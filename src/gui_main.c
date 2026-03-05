#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#define IDC_PORT_LABEL 101
#define IDC_PORT_EDIT 102
#define IDC_START_BUTTON 103
#define IDC_STOP_BUTTON 104
#define IDC_STATUS_LABEL 105
#define STATUS_TIMER_ID 1

static PROCESS_INFORMATION g_server_process;
static int g_server_running = 0;

static HWND g_port_edit = NULL;
static HWND g_start_button = NULL;
static HWND g_stop_button = NULL;
static HWND g_status_label = NULL;

static void set_status_text(const char *text) {
    if (g_status_label != NULL) {
        SetWindowTextA(g_status_label, text);
    }
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

static int parse_port_input(uint16_t *port_out) {
    char buffer[16];
    if (GetWindowTextA(g_port_edit, buffer, (int)sizeof(buffer)) <= 0) {
        return 1;
    }

    char *end_ptr = NULL;
    long value = strtol(buffer, &end_ptr, 10);
    if (end_ptr == buffer || *end_ptr != '\0' || value <= 0 || value > 65535) {
        return 1;
    }

    *port_out = (uint16_t)value;
    return 0;
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

static void stop_server_process(void) {
    if (!g_server_running) {
        return;
    }

    TerminateProcess(g_server_process.hProcess, 0);
    WaitForSingleObject(g_server_process.hProcess, 3000);

    cleanup_server_process_handles();
    g_server_running = 0;
    set_status_text("Stopped");
    update_button_state();
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

    char command_line[MAX_PATH + 32];
    if (snprintf(command_line, sizeof(command_line), "\"%s\" %u", server_path, port) < 0) {
        MessageBoxA(NULL, "Failed to create server command line.", "Start Error", MB_OK | MB_ICONERROR);
        return;
    }

    STARTUPINFOA startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info;
    ZeroMemory(&process_info, sizeof(process_info));

    if (!CreateProcessA(
            NULL,
            command_line,
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_PROCESS_GROUP,
            NULL,
            bin_dir,
            &startup_info,
            &process_info)) {
        MessageBoxA(NULL, "Failed to start webserv.exe.", "Start Error", MB_OK | MB_ICONERROR);
        return;
    }

    g_server_process = process_info;
    g_server_running = 1;

    char status_text[64];
    snprintf(status_text, sizeof(status_text), "Running on port %u", port);
    set_status_text(status_text);
    update_button_state();
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    (void)l_param;

    switch (msg) {
    case WM_CREATE:
        CreateWindowA("STATIC", "Port:", WS_VISIBLE | WS_CHILD, 20, 20, 40, 24, hwnd, (HMENU)IDC_PORT_LABEL, NULL, NULL);

        g_port_edit = CreateWindowA("EDIT", "8080", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT, 70, 20, 110, 24, hwnd, (HMENU)IDC_PORT_EDIT, NULL, NULL);

        g_start_button = CreateWindowA("BUTTON", "Start", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 200, 20, 80, 24, hwnd, (HMENU)IDC_START_BUTTON, NULL, NULL);

        g_stop_button = CreateWindowA("BUTTON", "Stop", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 290, 20, 80, 24, hwnd, (HMENU)IDC_STOP_BUTTON, NULL, NULL);

        g_status_label = CreateWindowA("STATIC", "Stopped", WS_VISIBLE | WS_CHILD, 20, 65, 350, 24, hwnd, (HMENU)IDC_STATUS_LABEL, NULL, NULL);

        SetTimer(hwnd, STATUS_TIMER_ID, 1000, NULL);
        update_button_state();
        return 0;

    case WM_TIMER:
        if (w_param == STATUS_TIMER_ID && g_server_running && !is_server_process_alive()) {
            cleanup_server_process_handles();
            g_server_running = 0;
            set_status_text("Stopped (process exited)");
            update_button_state();
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
        default:
            break;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, STATUS_TIMER_ID);
        stop_server_process();
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
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&window_class)) {
        MessageBoxA(NULL, "Failed to register GUI window class.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExA(
        0,
        class_name,
        "c-webserv Control Panel",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        410,
        145,
        NULL,
        NULL,
        instance,
        NULL);

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Failed to create GUI window.", "Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, cmd_show);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
