#include <stdio.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "webserver.h"

#define REQUEST_BUFFER_SIZE 4096

static const char *HTTP_RESPONSE =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Connection: close\r\n"
    "Content-Length: 34\r\n"
    "\r\n"
    "Hello from c-webserv on Windows!\n";

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

static void handle_client(SOCKET client_socket) {
    char request_buffer[REQUEST_BUFFER_SIZE];
    int recv_result = recv(client_socket, request_buffer, (int)sizeof(request_buffer) - 1, 0);

    if (recv_result > 0) {
        request_buffer[recv_result] = '\0';
        printf("Received request:\n%.*s\n", recv_result, request_buffer);
    }

    send(client_socket, HTTP_RESPONSE, (int)strlen(HTTP_RESPONSE), 0);
    closesocket(client_socket);
}

int run_server(uint16_t port) {
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

    for (;;) {
        SOCKET client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
            break;
        }

        handle_client(client_socket);
    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
