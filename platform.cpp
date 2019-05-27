#include "platform.h"

#include "synchapi.h"
#include <cassert>

#ifdef _WIN32

static Destroy_Callback g_window_destroy_callback;

constexpr LPCTSTR window_class_name = TEXT("VulkanPractice");
constexpr LPCTSTR window_name = TEXT("Vulkan Practice");
LRESULT CALLBACK window_proc_callback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        g_window_destroy_callback();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }

    return 0;
}

Window create_window(const Rect& window_rect, Destroy_Callback destroy_callback)
{
    g_window_destroy_callback = destroy_callback;

    Window window = {};
    window.h_instance = GetModuleHandle(nullptr);

    // Register the window class
    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.style = 0;
    window_class.lpfnWndProc = window_proc_callback;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = window.h_instance;
    window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszMenuName = nullptr;
    window_class.lpszClassName = window_class_name;
    window_class.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    ATOM window_class_atom = RegisterClassExW(&window_class);
    if (!window_class_atom) {
        assert(!"Unable to register window class");
    }

    // Create the window
    window.h_window = CreateWindowW(window_class_name, window_name, WS_TILEDWINDOW, 
                                    window_rect.x, window_rect.y, window_rect.width, window_rect.height, 
                                    nullptr, nullptr, window.h_instance, nullptr);
    if (!window.h_window) {
        assert(!"Unable to create window");
    }

    ShowWindow(window.h_window, SW_SHOW);
    UpdateWindow(window.h_window);

    return window;
}

int process_window_messages(Window const& window)
{
    int num_msgs = 0;
    MSG window_msg;
    while (PeekMessageA(&window_msg, window.h_window, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&window_msg);
        DispatchMessage(&window_msg); // call the window proc callback
        ++num_msgs;
    }

    return num_msgs;
}

static double g_platform_init = false;
static double g_sys_perf_freq_ms = 0.0;

void init_platform() {
    LARGE_INTEGER sys_perf_freq;
    if (!QueryPerformanceFrequency(&sys_perf_freq))
    {
        log_error("QueryPerformanceCounter failed\n");
        assert(!"QueryPerformanceCounter failed");
    }

    g_sys_perf_freq_ms = static_cast<double>(sys_perf_freq.QuadPart) / 1000.0;

    g_platform_init = true;
}

double get_perf_counter_ms() {
    assert(g_platform_init);
    LARGE_INTEGER sys_perf_counter;
    QueryPerformanceCounter(&sys_perf_counter);
    return static_cast<double>(sys_perf_counter.QuadPart) / g_sys_perf_freq_ms;
}

void sleep(double milliseconds) {
    Sleep(static_cast<DWORD>(milliseconds));
}

#endif // _WIN32