#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "webserver.h"

#define REQUEST_BUFFER_SIZE 4096
#define RESPONSE_BUFFER_SIZE 512
#define FILE_BUFFER_SIZE 8192
#define URL_PATH_SIZE 1024
#define FILE_PATH_SIZE 2048
#define HEADER_VALUE_SIZE 256

static int send_all(SOCKET socket, const char *data, int length) {
    int sent_total = 0;
    while (sent_total < length) {
        int sent = send(socket, data + sent_total, length - sent_total, 0);
        if (sent == SOCKET_ERROR) {
            return 1;
        }

        sent_total += sent;
    }

    return 0;
}

static int equals_ignore_case(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
            return 0;
        }
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static void send_text_response(SOCKET client_socket, int status_code, const char *status_text, const char *body) {
    char header[RESPONSE_BUFFER_SIZE];
    int body_len = (int)strlen(body);
    int header_len = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        status_code,
        status_text,
        body_len);

    if (header_len > 0) {
        send_all(client_socket, header, header_len);
    }
    send_all(client_socket, body, body_len);
}

static const char *get_content_type(const char *file_path) {
    const char *extension = strrchr(file_path, '.');
    if (extension == NULL) {
        return "application/octet-stream";
    }

    if (equals_ignore_case(extension, ".html") || equals_ignore_case(extension, ".htm")) {
        return "text/html; charset=utf-8";
    }
    if (equals_ignore_case(extension, ".css")) {
        return "text/css; charset=utf-8";
    }
    if (equals_ignore_case(extension, ".js")) {
        return "application/javascript; charset=utf-8";
    }
    if (equals_ignore_case(extension, ".json")) {
        return "application/json; charset=utf-8";
    }
    if (equals_ignore_case(extension, ".txt")) {
        return "text/plain; charset=utf-8";
    }
    if (equals_ignore_case(extension, ".png")) {
        return "image/png";
    }
    if (equals_ignore_case(extension, ".jpg") || equals_ignore_case(extension, ".jpeg")) {
        return "image/jpeg";
    }
    if (equals_ignore_case(extension, ".gif")) {
        return "image/gif";
    }
    if (equals_ignore_case(extension, ".svg")) {
        return "image/svg+xml";
    }

    return "application/octet-stream";
}

static int is_directory(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return 0;
    }

    return (info.st_mode & S_IFDIR) != 0;
}

static int file_exists(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return 0;
    }

    return (info.st_mode & S_IFREG) != 0;
}

static int contains_path_traversal(const char *path) {
    const char *cursor = path;
    while (*cursor != '\0') {
        if (cursor[0] == '.' && cursor[1] == '.' &&
            (cursor[2] == '\0' || cursor[2] == '/' || cursor[2] == '\\')) {
            return 1;
        }
        cursor++;
    }

    return 0;
}

static int resolve_www_path(const char *raw_target, char *resolved_path, size_t resolved_path_size) {
    char cleaned_target[URL_PATH_SIZE];
    size_t i = 0;

    while (raw_target[i] != '\0' && raw_target[i] != '?' && raw_target[i] != '#' && i < sizeof(cleaned_target) - 1) {
        cleaned_target[i] = raw_target[i];
        i++;
    }
    cleaned_target[i] = '\0';

    if (cleaned_target[0] != '/') {
        return 1;
    }

    if (contains_path_traversal(cleaned_target)) {
        return 1;
    }

    for (i = 0; cleaned_target[i] != '\0'; i++) {
        if (cleaned_target[i] == '/') {
            cleaned_target[i] = '\\';
        }
    }

    if (strcmp(cleaned_target, "\\") == 0) {
        return snprintf(resolved_path, resolved_path_size, "www\\index.html") < 0;
    }

    if (snprintf(resolved_path, resolved_path_size, "www%s", cleaned_target) < 0) {
        return 1;
    }

    if (is_directory(resolved_path)) {
        char index_path[FILE_PATH_SIZE];
        if (snprintf(index_path, sizeof(index_path), "%s\\index.html", resolved_path) < 0) {
            return 1;
        }

        if (file_exists(index_path)) {
            return snprintf(resolved_path, resolved_path_size, "%s", index_path) < 0;
        }
    }

    return 0;
}

static void extract_request_path(const char *raw_target, char *path_out, size_t path_out_size) {
    size_t i = 0;
    while (raw_target[i] != '\0' && raw_target[i] != '?' && raw_target[i] != '#' && i < path_out_size - 1) {
        path_out[i] = raw_target[i];
        i++;
    }
    path_out[i] = '\0';
}

static int read_header_value(const char *request, const char *header_name, char *value_out, size_t value_out_size) {
    char key[64];
    if (snprintf(key, sizeof(key), "\r\n%s:", header_name) < 0) {
        return 1;
    }

    const char *header_start = strstr(request, key);
    if (header_start == NULL) {
        return 1;
    }

    const char *value_start = header_start + strlen(key);
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++;
    }

    size_t i = 0;
    while (value_start[i] != '\0' && value_start[i] != '\r' && value_start[i] != '\n' && i < value_out_size - 1) {
        value_out[i] = value_start[i];
        i++;
    }
    value_out[i] = '\0';

    return i == 0 ? 1 : 0;
}

static void send_file_response(SOCKET client_socket, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        send_text_response(client_socket, 500, "Internal Server Error", "500 Internal Server Error\n");
        return;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        send_text_response(client_socket, 500, "Internal Server Error", "500 Internal Server Error\n");
        return;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        send_text_response(client_socket, 500, "Internal Server Error", "500 Internal Server Error\n");
        return;
    }

    rewind(file);

    const char *content_type = get_content_type(file_path);
    char header[RESPONSE_BUFFER_SIZE];
    int header_len = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        content_type,
        file_size);

    if (header_len > 0 && send_all(client_socket, header, header_len) != 0) {
        fclose(file);
        return;
    }

    char buffer[FILE_BUFFER_SIZE];
    while (!feof(file)) {
        size_t read_count = fread(buffer, 1, sizeof(buffer), file);
        if (read_count > 0) {
            if (send_all(client_socket, buffer, (int)read_count) != 0) {
                break;
            }
        }

        if (ferror(file)) {
            break;
        }
    }

    fclose(file);
}

static int init_winsock(void) {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", result);
        return 1;
    }

    return 0;
}

static SOCKET create_listening_socket(uint16_t port) {
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        fprintf(stderr, "socket failed with error: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    int opt = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == SOCKET_ERROR) {
        fprintf(stderr, "setsockopt failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }

    return listen_socket;
}

static int handle_client(SOCKET client_socket, const char *shutdown_token) {
    char request_buffer[REQUEST_BUFFER_SIZE];
    int recv_result = recv(client_socket, request_buffer, (int)sizeof(request_buffer) - 1, 0);

    if (recv_result <= 0) {
        closesocket(client_socket);
        return 1;
    }

    request_buffer[recv_result] = '\0';
    printf("Received request:\n%.*s\n", recv_result, request_buffer);

    char method[16];
    char target[URL_PATH_SIZE];
    if (sscanf(request_buffer, "%15s %1023s", method, target) != 2) {
        send_text_response(client_socket, 400, "Bad Request", "400 Bad Request\n");
        closesocket(client_socket);
        return 1;
    }

    if (!equals_ignore_case(method, "GET")) {
        send_text_response(client_socket, 405, "Method Not Allowed", "405 Method Not Allowed\n");
        closesocket(client_socket);
        return 1;
    }

    char request_path[URL_PATH_SIZE];
    extract_request_path(target, request_path, sizeof(request_path));

    if (strcmp(request_path, "/__shutdown") == 0) {
        if (shutdown_token == NULL || shutdown_token[0] == '\0') {
            send_text_response(client_socket, 403, "Forbidden", "403 Forbidden\n");
            closesocket(client_socket);
            return 1;
        }

        char provided_token[HEADER_VALUE_SIZE];
        if (read_header_value(request_buffer, "X-Shutdown-Token", provided_token, sizeof(provided_token)) != 0 ||
            strcmp(provided_token, shutdown_token) != 0) {
            send_text_response(client_socket, 403, "Forbidden", "403 Forbidden\n");
            closesocket(client_socket);
            return 1;
        }

        send_text_response(client_socket, 200, "OK", "Shutting down\n");
        closesocket(client_socket);
        return 0;
    }

    char file_path[FILE_PATH_SIZE];
    if (resolve_www_path(target, file_path, sizeof(file_path)) != 0) {
        send_text_response(client_socket, 400, "Bad Request", "400 Bad Request\n");
        closesocket(client_socket);
        return 1;
    }

    if (!file_exists(file_path)) {
        send_text_response(client_socket, 404, "Not Found", "404 Not Found\n");
        closesocket(client_socket);
        return 1;
    }

    send_file_response(client_socket, file_path);
    closesocket(client_socket);
    return 1;
}

int run_server(uint16_t port, const char *shutdown_token) {
    if (init_winsock() != 0) {
        return 1;
    }

    SOCKET listen_socket = create_listening_socket(port);
    if (listen_socket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    printf("c-webserv listening on http://localhost:%u\n", port);
    printf("Press Ctrl+C to stop the server.\n");
    if (shutdown_token != NULL && shutdown_token[0] != '\0') {
        printf("Graceful shutdown endpoint enabled at /__shutdown\n");
    }

    for (;;) {
        SOCKET client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
            break;
        }

        if (!handle_client(client_socket, shutdown_token)) {
            break;
        }
    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
