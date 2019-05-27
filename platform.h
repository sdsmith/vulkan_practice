#pragma once

struct Rect
{
    int x;
    int y;
    int width;
    int height;
};

#ifdef _WIN32
#define NOMINMAX 1
#include <Windows.h>
#include <WinBase.h>

struct Window
{
    HINSTANCE h_instance;
    HWND h_window;
};

template<typename... Args>
void log_error(char const* format, Args... args)
{
    // TODO: detect when building in VS IDE vs all other cases
    constexpr size_t buf_size = 1024;
    char buf[buf_size];
    snprintf(buf, buf_size, format, args...);
    OutputDebugStringA(buf);
}

LRESULT CALLBACK window_proc_callback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

using Destroy_Callback = void (*)();
Window create_window(const Rect& window_rect, Destroy_Callback destroy_callback);

int process_window_messages(Window const& window);

#endif // _WIN32