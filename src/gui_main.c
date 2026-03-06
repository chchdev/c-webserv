#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "resource.h"

#define IDC_PORT_LABEL 101
#define IDC_PORT_EDIT 102
#define IDC_START_BUTTON 103
#define IDC_STOP_BUTTON 104
#define IDC_URL_LABEL 105
#define IDC_HINT_LABEL 106
#define STATUS_TIMER_ID 1
#define SHUTDOWN_TOKEN_SIZE 64

#define HEADER_HEIGHT 96
#define CARD_LEFT 22
#define CARD_TOP 116
#define CARD_HEIGHT 170

static PROCESS_INFORMATION g_server_process;
static int g_server_running = 0;
static uint16_t g_current_port = 0;
static char g_shutdown_token[SHUTDOWN_TOKEN_SIZE];
static char g_status_text[128] = "Stopped";

static HWND g_port_label = NULL;
static HWND g_port_edit = NULL;
static HWND g_start_button = NULL;
static HWND g_stop_button = NULL;
static HWND g_url_label = NULL;
static HWND g_hint_label = NULL;

static HFONT g_font_regular = NULL;
static HFONT g_font_heading = NULL;
static HFONT g_font_subheading = NULL;
static HFONT g_font_mono = NULL;

static HICON g_app_icon = NULL;
static HBRUSH g_brush_window = NULL;
static HBRUSH g_brush_header = NULL;
static HBRUSH g_brush_card = NULL;
static HBRUSH g_brush_badge_running = NULL;
static HBRUSH g_brush_badge_stopped = NULL;
static HPEN g_card_border_pen = NULL;

static const COLORREF COLOR_HEADER_TEXT = RGB(245, 249, 255);
static const COLORREF COLOR_SUBTITLE_TEXT = RGB(194, 210, 230);
static const COLORREF COLOR_BODY_TEXT = RGB(43, 58, 81);
static const COLORREF COLOR_MUTED_TEXT = RGB(106, 122, 146);
static const COLORREF COLOR_BADGE_TEXT = RGB(255, 255, 255);

static void apply_font(HWND control, HFONT font) {
    if (control != NULL && font != NULL) {
        SendMessageA(control, WM_SETFONT, (WPARAM)font, TRUE);
    }
}

static void initialize_fonts(void) {
    g_font_heading = CreateFontA(
        -30,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "Segoe UI");

    g_font_subheading = CreateFontA(
        -18,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "Segoe UI");

    g_font_regular = CreateFontA(
        -18,
        0,
        0,
        0,
        FW_SEMIBOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "Segoe UI");

    g_font_mono = CreateFontA(
        -18,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN,
        "Consolas");
}

static void initialize_theme(void) {
    g_brush_window = CreateSolidBrush(RGB(243, 247, 252));
    g_brush_header = CreateSolidBrush(RGB(26, 44, 72));
    g_brush_card = CreateSolidBrush(RGB(255, 255, 255));
    g_brush_badge_running = CreateSolidBrush(RGB(38, 154, 96));
    g_brush_badge_stopped = CreateSolidBrush(RGB(114, 132, 159));
    g_card_border_pen = CreatePen(PS_SOLID, 1, RGB(217, 226, 239));
}

static void cleanup_theme(void) {
    if (g_brush_window != NULL) {
        DeleteObject(g_brush_window);
        g_brush_window = NULL;
    }
    if (g_brush_header != NULL) {
        DeleteObject(g_brush_header);
        g_brush_header = NULL;
    }
    if (g_brush_card != NULL) {
        DeleteObject(g_brush_card);
        g_brush_card = NULL;
    }
    if (g_brush_badge_running != NULL) {
        DeleteObject(g_brush_badge_running);
        g_brush_badge_running = NULL;
    }
    if (g_brush_badge_stopped != NULL) {
        DeleteObject(g_brush_badge_stopped);
        g_brush_badge_stopped = NULL;
    }
    if (g_card_border_pen != NULL) {
        DeleteObject(g_card_border_pen);
        g_card_border_pen = NULL;
    }
}

static HICON load_app_icon(HINSTANCE instance, int size) {
    HICON icon = (HICON)LoadImageA(instance, MAKEINTRESOURCEA(IDI_APP_ICON), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
    if (icon != NULL) {
        return icon;
    }

    return LoadIconA(NULL, IDI_APPLICATION);
}

static int is_server_process_alive(void) {
    if (!g_server_running) {
        return 0;
    }

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(g_server_process.hProcess, &exit_code)) {
        return 0;
    }

    return exit_code == STILL_ACTIVE;
}

static void cleanup_server_process_handles(void) {
    if (g_server_process.hThread != NULL) {
        CloseHandle(g_server_process.hThread);
    }
    if (g_server_process.hProcess != NULL) {
        CloseHandle(g_server_process.hProcess);
    }

    ZeroMemory(&g_server_process, sizeof(g_server_process));
}

static void update_button_state(void) {
    EnableWindow(g_start_button, g_server_running ? FALSE : TRUE);
    EnableWindow(g_stop_button, g_server_running ? TRUE : FALSE);
    EnableWindow(g_port_edit, g_server_running ? FALSE : TRUE);
}

static uint16_t get_port_for_preview(void) {
    char buffer[16];
    if (GetWindowTextA(g_port_edit, buffer, (int)sizeof(buffer)) <= 0) {
        return 8080;
    }

    char *end_ptr = NULL;
    long value = strtol(buffer, &end_ptr, 10);
    if (end_ptr == buffer || *end_ptr != '\0' || value <= 0 || value > 65535) {
        return 8080;
    }

    return (uint16_t)value;
}

static int parse_port_input(uint16_t *port_out) {
    uint16_t parsed = get_port_for_preview();
    char buffer[16];
    GetWindowTextA(g_port_edit, buffer, (int)sizeof(buffer));

    char *end_ptr = NULL;
    long original = strtol(buffer, &end_ptr, 10);
    if (end_ptr == buffer || *end_ptr != '\0' || original <= 0 || original > 65535) {
        return 1;
    }

    *port_out = parsed;
    return 0;
}

static void update_url_preview(void) {
    char url_text[128];
    uint16_t preview_port = g_server_running ? g_current_port : get_port_for_preview();
    snprintf(url_text, sizeof(url_text), "Server URL: http://localhost:%u", preview_port);
    SetWindowTextA(g_url_label, url_text);
}

static void set_status_text(const char *text) {
    snprintf(g_status_text, sizeof(g_status_text), "%s", text);
    InvalidateRect(GetParent(g_port_edit), NULL, TRUE);
}

static int get_bin_directory(char *out_dir, size_t out_dir_size) {
    if (GetModuleFileNameA(NULL, out_dir, (DWORD)out_dir_size) == 0) {
        return 1;
    }

    char *last_separator = strrchr(out_dir, '\\');
    if (last_separator == NULL) {
        return 1;
    }

    *last_separator = '\0';
    return 0;
}

static int build_server_executable_path(char *out_path, size_t out_path_size, char *bin_dir, size_t bin_dir_size) {
    if (get_bin_directory(bin_dir, bin_dir_size) != 0) {
        return 1;
    }

    if (snprintf(out_path, out_path_size, "%s\\webserv.exe", bin_dir) < 0) {
        return 1;
    }

    DWORD attrs = GetFileAttributesA(out_path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return 1;
    }

    return 0;
}

static void generate_shutdown_token(char *token_out, size_t token_out_size) {
    DWORD pid = GetCurrentProcessId();
    DWORD ticks = GetTickCount();
    snprintf(token_out, token_out_size, "%lu-%lu", (unsigned long)pid, (unsigned long)ticks);
}

static int send_shutdown_request(uint16_t port, const char *token) {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char request[512];
    int request_len = snprintf(
        request,
        sizeof(request),
        "GET /__shutdown HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "X-Shutdown-Token: %s\r\n"
        "\r\n",
        token);

    if (request_len <= 0 || send(sock, request, request_len, 0) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char response[128];
    int recv_result = recv(sock, response, (int)sizeof(response), 0);

    closesocket(sock);
    WSACleanup();

    return recv_result <= 0 ? 1 : 0;
}

static void stop_server_process(void) {
    if (!g_server_running) {
        return;
    }

    int requested_shutdown = send_shutdown_request(g_current_port, g_shutdown_token) == 0;
    DWORD wait_result = WaitForSingleObject(g_server_process.hProcess, 3000);

    if (wait_result != WAIT_OBJECT_0) {
        TerminateProcess(g_server_process.hProcess, 0);
        WaitForSingleObject(g_server_process.hProcess, 3000);
        set_status_text("Stopped (forced)");
    } else if (requested_shutdown) {
        set_status_text("Stopped");
    } else {
        set_status_text("Stopped");
    }

    cleanup_server_process_handles();
    g_server_running = 0;
    g_current_port = 0;
    g_shutdown_token[0] = '\0';
    update_button_state();
    update_url_preview();
}

static void start_server_process(void) {
    if (g_server_running && is_server_process_alive()) {
        set_status_text("Already running");
        return;
    }

    if (g_server_running) {
        cleanup_server_process_handles();
        g_server_running = 0;
    }

    uint16_t port = 0;
    if (parse_port_input(&port) != 0) {
        MessageBoxA(NULL, "Please enter a valid port (1-65535).", "Invalid Port", MB_OK | MB_ICONERROR);
        return;
    }

    char bin_dir[MAX_PATH];
    char server_path[MAX_PATH];
    if (build_server_executable_path(server_path, sizeof(server_path), bin_dir, sizeof(bin_dir)) != 0) {
        MessageBoxA(NULL, "Could not locate bin\\webserv.exe next to this GUI executable.", "Missing Server Binary", MB_OK | MB_ICONERROR);
        return;
    }

    generate_shutdown_token(g_shutdown_token, sizeof(g_shutdown_token));

    char command_line[MAX_PATH + 128];
    if (snprintf(command_line, sizeof(command_line), "\"%s\" %u --shutdown-token %s", server_path, port, g_shutdown_token) < 0) {
        MessageBoxA(NULL, "Failed to create server command line.", "Start Error", MB_OK | MB_ICONERROR);
        return;
    }

    STARTUPINFOA startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION process_info;
    ZeroMemory(&process_info, sizeof(process_info));

    if (!CreateProcessA(
            NULL,
            command_line,
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW,
            NULL,
            bin_dir,
            &startup_info,
            &process_info)) {
        MessageBoxA(NULL, "Failed to start webserv.exe.", "Start Error", MB_OK | MB_ICONERROR);
        return;
    }

    g_server_process = process_info;
    g_server_running = 1;
    g_current_port = port;

    char status_text[64];
    snprintf(status_text, sizeof(status_text), "Running on port %u", port);
    set_status_text(status_text);

    update_button_state();
    update_url_preview();
}

static void draw_badge(HDC hdc, RECT *badge_rect) {
    HBRUSH badge_brush = g_server_running ? g_brush_badge_running : g_brush_badge_stopped;
    FillRect(hdc, badge_rect, badge_brush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_BADGE_TEXT);
    SelectObject(hdc, g_font_regular);

    char badge_text[80];
    snprintf(badge_text, sizeof(badge_text), "Status: %s", g_status_text);
    DrawTextA(hdc, badge_text, -1, badge_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

static void draw_shell(HWND hwnd, HDC hdc) {
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);

    FillRect(hdc, &client_rect, g_brush_window);

    RECT header_rect = {0, 0, client_rect.right, HEADER_HEIGHT};
    FillRect(hdc, &header_rect, g_brush_header);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_HEADER_TEXT);
    SelectObject(hdc, g_font_heading);

    RECT title_rect = {24, 16, client_rect.right - 24, 52};
    DrawTextA(hdc, "c-webserv", -1, &title_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, COLOR_SUBTITLE_TEXT);
    SelectObject(hdc, g_font_subheading);
    RECT subtitle_rect = {26, 54, client_rect.right - 24, 84};
    DrawTextA(hdc, "Manage server runtime, ports, and graceful lifecycle operations", -1, &subtitle_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT card_rect = {CARD_LEFT, CARD_TOP, client_rect.right - 22, CARD_TOP + CARD_HEIGHT};
    FillRect(hdc, &card_rect, g_brush_card);
    SelectObject(hdc, g_card_border_pen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, card_rect.left, card_rect.top, card_rect.right, card_rect.bottom);

    SetTextColor(hdc, COLOR_MUTED_TEXT);
    SelectObject(hdc, g_font_subheading);
    RECT label_rect = {card_rect.left + 18, card_rect.top + 8, card_rect.right - 18, card_rect.top + 34};
    DrawTextA(hdc, "SERVER CONFIGURATION", -1, &label_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT badge_rect = {CARD_LEFT, CARD_TOP + CARD_HEIGHT + 16, client_rect.right - 22, CARD_TOP + CARD_HEIGHT + 56};
    draw_badge(hdc, &badge_rect);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
    case WM_CREATE:
        initialize_fonts();
        initialize_theme();

        g_port_label = CreateWindowA("STATIC", "Port", WS_VISIBLE | WS_CHILD, 46, 154, 70, 24, hwnd, (HMENU)IDC_PORT_LABEL, NULL, NULL);

        g_port_edit = CreateWindowA("EDIT", "8080", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 46, 180, 150, 30, hwnd, (HMENU)IDC_PORT_EDIT, NULL, NULL);

        g_start_button = CreateWindowA("BUTTON", "Start Server", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 216, 178, 144, 34, hwnd, (HMENU)IDC_START_BUTTON, NULL, NULL);

        g_stop_button = CreateWindowA("BUTTON", "Stop Server", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 372, 178, 144, 34, hwnd, (HMENU)IDC_STOP_BUTTON, NULL, NULL);

        g_url_label = CreateWindowA("STATIC", "Server URL: http://localhost:8080", WS_VISIBLE | WS_CHILD, 46, 228, 470, 26, hwnd, (HMENU)IDC_URL_LABEL, NULL, NULL);

        g_hint_label = CreateWindowA("STATIC", "Tip: Keep this console open while serving files from www/", WS_VISIBLE | WS_CHILD, 46, 252, 470, 22, hwnd, (HMENU)IDC_HINT_LABEL, NULL, NULL);

        apply_font(g_port_label, g_font_regular);
        apply_font(g_port_edit, g_font_regular);
        apply_font(g_start_button, g_font_regular);
        apply_font(g_stop_button, g_font_regular);
        apply_font(g_url_label, g_font_mono);
        apply_font(g_hint_label, g_font_subheading);

        SetTimer(hwnd, STATUS_TIMER_ID, 1000, NULL);
        update_button_state();
        update_url_preview();
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT paint_struct;
        HDC hdc = BeginPaint(hwnd, &paint_struct);
        draw_shell(hwnd, hdc);
        EndPaint(hwnd, &paint_struct);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)w_param;
        HWND control = (HWND)l_param;

        SetBkMode(hdc, TRANSPARENT);
        if (control == g_hint_label) {
            SetTextColor(hdc, COLOR_MUTED_TEXT);
        } else {
            SetTextColor(hdc, COLOR_BODY_TEXT);
        }
        return (LRESULT)g_brush_card;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)w_param;
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, COLOR_BODY_TEXT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_TIMER:
        if (w_param == STATUS_TIMER_ID && g_server_running && !is_server_process_alive()) {
            cleanup_server_process_handles();
            g_server_running = 0;
            g_current_port = 0;
            g_shutdown_token[0] = '\0';
            set_status_text("Stopped (process exited)");
            update_button_state();
            update_url_preview();
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(w_param)) {
        case IDC_START_BUTTON:
            start_server_process();
            return 0;

        case IDC_STOP_BUTTON:
            stop_server_process();
            return 0;

        case IDC_PORT_EDIT:
            if (HIWORD(w_param) == EN_CHANGE && !g_server_running) {
                update_url_preview();
            }
            return 0;

        default:
            break;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, STATUS_TIMER_ID);
        stop_server_process();

        if (g_font_heading != NULL) {
            DeleteObject(g_font_heading);
            g_font_heading = NULL;
        }
        if (g_font_subheading != NULL) {
            DeleteObject(g_font_subheading);
            g_font_subheading = NULL;
        }
        if (g_font_regular != NULL) {
            DeleteObject(g_font_regular);
            g_font_regular = NULL;
        }
        if (g_font_mono != NULL) {
            DeleteObject(g_font_mono);
            g_font_mono = NULL;
        }

        cleanup_theme();

        if (g_app_icon != NULL) {
            DestroyIcon(g_app_icon);
            g_app_icon = NULL;
        }

        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show) {
    (void)prev_instance;
    (void)cmd_line;

    const char *class_name = "CWebservGuiWindow";

    WNDCLASSA window_class;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hIcon = load_app_icon(instance, 64);

    if (!RegisterClassA(&window_class)) {
        MessageBoxA(NULL, "Failed to register GUI window class.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExA(
        0,
        class_name,
        "c-webserv Control Panel",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        572,
        404,
        NULL,
        NULL,
        instance,
        NULL);

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Failed to create GUI window.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_app_icon = load_app_icon(instance, 64);
    if (g_app_icon != NULL) {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_app_icon);
    }

    HICON small_icon = load_app_icon(instance, 32);
    if (small_icon != NULL) {
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)small_icon);
        if (small_icon != g_app_icon) {
            DestroyIcon(small_icon);
        }
    }

    ShowWindow(hwnd, cmd_show);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
