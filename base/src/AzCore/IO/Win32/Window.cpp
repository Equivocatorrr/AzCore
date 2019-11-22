/*
    File: Win32/Window.cpp
    Author: Philip Haynes
*/

#include "../../io.hpp"
#ifdef IO_FOR_VULKAN
#include "../../vk.hpp"
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dinput.h>

#define WS_FULLSCREEN (WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE)
#define WS_WINDOWED (WS_OVERLAPPEDWINDOW | WS_VISIBLE)

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#include "../RawInput.hpp"
#include "../../io.hpp"
#include "../../basictypes.hpp"
#include "../../keycodes.hpp"
#include "../../Memory/String.hpp"

namespace AzCore {

namespace io {

u32 windowClassNum = 0;

String winGetInputName(u8 hid)
{
    if (hid == 255)
    {
        return "Null";
    }
    // First make sure we're not anything that doesn't move
    if (hid < 0x04 || (hid >= 0x28 && hid <= 0x2c) || (hid >= 0x39 && hid <= 0x58) || hid >= 0x64)
    {
        return KeyCodeName(hid);
    }
    // Check if we even have a mapping at all
    u8 keyCode = KeyCodeToWinScan(hid);
    if (keyCode == 255)
    {
        return "None";
    }
    // If layout-dependent, update the label based on the layout
    char label[2] = {
        (char)MapVirtualKey(MapVirtualKey((u32)keyCode, MAPVK_VSC_TO_VK), MAPVK_VK_TO_CHAR),
        0};
    return String(label);
}

Window *focusedWindow = nullptr;

struct WindowData
{
    HINSTANCE instance;
    HWND window;
    WNDCLASSEX windowClass;
    HICON windowIcon, windowIconSmall;
    String windowClassName;
};

Window::Window()
{
    data = new WindowData;
}

Window::~Window()
{
    if (open)
    {
        Close();
    }
    delete data;
}

#ifdef IO_FOR_VULKAN
bool Window::CreateVkSurface(vk::Instance *instance, VkSurfaceKHR *surface)
{
    if (!open)
    {
        error = "CreateVkSurface was called before the window was created!";
        return false;
    }
    VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    createInfo.hinstance = data->instance;
    createInfo.hwnd = data->window;
    VkResult result = vkCreateWin32SurfaceKHR(instance->data.instance, &createInfo, nullptr, surface);
    if (result != VK_SUCCESS)
    {
        error = "Failed to create Win32 Surface!";
        return false;
    }
    return true;
}
#endif

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window *thisWindow = focusedWindow;
    if (focusedWindow == nullptr)
    {
        PostQuitMessage(0);
        return 0;
    }
    u8 keyCode = 0;
    char character = '\0';
    bool press = false, release = false;
    switch (uMsg)
    {
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    // Dealing with the close button
    case WM_CLOSE:
    {
        focusedWindow->quit = true;
        return 0;
    }
    case WM_DESTROY:
    {
        return 0;
    }
    // Keyboard Controls
    case WM_KEYDOWN:
    {
        // keyCode = KeyCodeFromWinVK((u8)wParam);
        keyCode = KeyCodeFromWinScan((u8)(lParam >> 16));
        if (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9)
        {
            keyCode = KC_KEY_KP1 + wParam - VK_NUMPAD1;
        }
        else if (wParam == VK_NUMPAD0)
        {
            keyCode = KC_KEY_KP0;
        }
        else if (wParam == VK_NUMLOCK)
        {
            keyCode = KC_KEY_NUMLOCK;
        }
        else if (wParam == VK_DECIMAL)
        {
            keyCode = KC_KEY_KPDOT;
        }
        else if (wParam == VK_MULTIPLY)
        {
            keyCode = KC_KEY_KPASTERISK;
        }
        else if (wParam == VK_DIVIDE)
        {
            keyCode = KC_KEY_KPSLASH;
        }
        else if (wParam == VK_MULTIPLY)
        {
            keyCode = KC_KEY_KPASTERISK;
        }
        // cout << "KeyCode down: " << KeyCodeName(keyCode) << std::endl;
        // cout << "WM_KEYDOWN scancode: 0x" << std::hex << (u32)((u8)(lParam>>16)) << " vk_code: 0x" << (u32)((u8)wParam) << std::endl;
        character = (char)MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
        press = true;
        break;
    }
    case WM_KEYUP:
    {
        // keyCode = KeyCodeFromWinVK((u8)wParam);
        keyCode = KeyCodeFromWinScan((u8)(lParam >> 16));
        if (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9)
        {
            keyCode = KC_KEY_KP1 + wParam - VK_NUMPAD1;
        }
        else if (wParam == VK_NUMPAD0)
        {
            keyCode = KC_KEY_KP0;
        }
        else if (wParam == VK_NUMLOCK)
        {
            keyCode = KC_KEY_NUMLOCK;
        }
        else if (wParam == VK_DECIMAL)
        {
            keyCode = KC_KEY_KPDOT;
        }
        else if (wParam == VK_MULTIPLY)
        {
            keyCode = KC_KEY_KPASTERISK;
        }
        else if (wParam == VK_DIVIDE)
        {
            keyCode = KC_KEY_KPSLASH;
        }
        else if (wParam == VK_MULTIPLY)
        {
            keyCode = KC_KEY_KPASTERISK;
        }
        // cout << "KeyCode up: " << KeyCodeName(keyCode) << std::endl;
        // cout << "WM_KEYUP scancode: 0x" << std::hex << (u32)((u8)(lParam>>16)) << " vk_code: 0x" << (u32)((u8)wParam) << std::endl;
        // character = (char)MapVirtualKey(wParam, MAPVK_VK_TO_CHAR);
        release = true;
        break;
    }
    // Mouse Controls
    case WM_MOUSEMOVE:
    {
        if (thisWindow->input != nullptr)
        {
            thisWindow->input->cursor.x = i32(i16(lParam));
            thisWindow->input->cursor.y = i32(lParam >> 16);
        }
        break;
    }
    case WM_MOUSEWHEEL:
    {
        if (thisWindow->input != nullptr)
        {
            thisWindow->input->scroll.y = f32(HIWORD(wParam)) / 120.0;
            if (thisWindow->input->scroll.y > 0)
                keyCode = KC_MOUSE_SCROLLUP;
            else
                keyCode = KC_MOUSE_SCROLLDOWN;
            press = true;
        }
        break;
    }
    case WM_MOUSEHWHEEL:
    {
        if (thisWindow->input != nullptr)
        {
            thisWindow->input->scroll.x = f32(HIWORD(wParam)) / 120.0;
            if (thisWindow->input->scroll.x > 0)
                keyCode = KC_MOUSE_SCROLLRIGHT;
            else
                keyCode = KC_MOUSE_SCROLLLEFT;
            press = true;
        }
        break;
    }
    case WM_LBUTTONDOWN:
    {
        keyCode = KC_MOUSE_LEFT;
        press = true;
        break;
    }
    case WM_LBUTTONUP:
    {
        keyCode = KC_MOUSE_LEFT;
        release = true;
        break;
    }
    case WM_MBUTTONDOWN:
    {
        keyCode = KC_MOUSE_MIDDLE;
        press = true;
        break;
    }
    case WM_MBUTTONUP:
    {
        keyCode = KC_MOUSE_MIDDLE;
        release = true;
        break;
    }
    case WM_RBUTTONDOWN:
    {
        keyCode = KC_MOUSE_RIGHT;
        press = true;
        break;
    }
    case WM_RBUTTONUP:
    {
        keyCode = KC_MOUSE_RIGHT;
        release = true;
        break;
    }
    case WM_XBUTTONDOWN:
    {
        i16 i = HIWORD(wParam);
        if (i == XBUTTON1)
            keyCode = KC_MOUSE_XONE;
        else
            keyCode = KC_MOUSE_XTWO;
        press = true;
        break;
    }
    case WM_XBUTTONUP:
    {
        i16 i = HIWORD(wParam); // XBUTTON1 = 1, XBUTTON2 = 2
        if (i == XBUTTON1)
            keyCode = KC_MOUSE_XONE;
        else
            keyCode = KC_MOUSE_XTWO;
        release = true;
        break;
    }
    case WM_CHAR:
    {
        if (thisWindow->input != nullptr)
        {
            thisWindow->input->typingString += (char)wParam;
        }
        // handleCharInput((char)wParam);
        break;
    }
    case WM_MOVE:
    {
        if (!thisWindow->resized)
        {
            if (!thisWindow->fullscreen)
            {
                thisWindow->windowedX = LOWORD(lParam);
                thisWindow->windowedY = HIWORD(lParam);
            }
            thisWindow->x = LOWORD(lParam);
            thisWindow->y = HIWORD(lParam);
        }
        break;
    }
    case WM_SIZE:
    {
        if (!thisWindow->resized)
        {
            // Workaround because Windows sucks
            thisWindow->width = LOWORD(lParam);
            thisWindow->height = HIWORD(lParam);
            if (!thisWindow->fullscreen)
            {
                thisWindow->windowedWidth = LOWORD(lParam);
                thisWindow->windowedHeight = HIWORD(lParam);
            }
        }
        else
        {
            thisWindow->resized = false;
        }
        break;
    }
    case WM_SETFOCUS:
    {
        thisWindow->focused = true;
        break;
    }
    case WM_KILLFOCUS:
    {
        thisWindow->focused = false;
        thisWindow->input->ReleaseAll();
        break;
    }
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (thisWindow->input != nullptr && thisWindow->focused)
    {
        if (press)
        {

            if (keyCode != 0)
            {
                thisWindow->input->Press(keyCode);
            }
            if (character != '\0')
            {
                thisWindow->input->PressChar(character);
            }
        }
        if (release)
        {
            if (keyCode != 0)
            {
                thisWindow->input->Release(keyCode);
            }
            if (character != '\0')
            {
                thisWindow->input->ReleaseChar(character);
            }
        }
    }

    if (keyCode == KC_MOUSE_XTWO || keyCode == KC_MOUSE_XONE)
        return TRUE;

    return 0;
}

bool Window::Open()
{
    data->instance = GetModuleHandle(NULL);
    data->windowIcon = LoadIcon(data->instance, "icon.ico");
    data->windowIconSmall = data->windowIcon;
    data->windowClass.cbSize = sizeof(WNDCLASSEX);
    data->windowClass.style = CS_OWNDC;
    data->windowClass.lpfnWndProc = WindowProcedure;
    data->windowClass.cbClsExtra = 0;
    data->windowClass.cbWndExtra = 0;
    data->windowClass.hInstance = data->instance;
    data->windowClass.hIcon = data->windowIcon;
    data->windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    data->windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    data->windowClass.lpszMenuName = NULL;

    data->windowClassName = "AzCore";
    data->windowClassName += ToString(windowClassNum++);
    data->windowClass.lpszClassName = data->windowClassName.data;
    data->windowClass.hIconSm = data->windowIconSmall;
    if (!RegisterClassEx(&data->windowClass))
    {
        error = "Failed to register window class: ";
        error += ToString((u32)GetLastError());
        return false;
    }

    RECT rect;
    rect.left = 0;
    rect.right = width;
    rect.top = 0;
    rect.bottom = height;
    AdjustWindowRect(&rect, WS_WINDOWED, FALSE);
    focusedWindow = this;
    data->window = CreateWindowEx(0, data->windowClassName.data, name.data, WS_WINDOWED, CW_USEDEFAULT, CW_USEDEFAULT,
                                  rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, data->instance, 0);
    if (data->window == NULL)
    {
        error = "Failed to create window: ";
        error += ToString((u32)GetLastError());
        return false;
    }
    open = true;
    return true;
}

bool Window::Show()
{
    if (!open)
    {
        error = "Window hasn't been created yet";
        return false;
    }
    ShowWindow(data->window, SW_SHOWNORMAL);

    return true;
}

bool Window::Close()
{
    if (!open)
    {
        error = "Window hasn't been created yet";
        return false;
    }
    DestroyWindow(data->window);
    UnregisterClass(data->windowClass.lpszClassName, data->instance);

    open = false;
    return true;
}

bool Window::Fullscreen(bool fs)
{
    if (!open)
    {
        error = "Window hasn't been created yet";
        return false;
    }
    if (fullscreen == fs)
        return true;

    fullscreen = fs;
    resized = true; // Prevent WM_SIZE from lying to us, the ungrateful bastard

    if (fullscreen)
    {
        HMONITOR monitor = MonitorFromWindow(data->window, MONITOR_DEFAULTTONEAREST);
        if (monitor != NULL)
        {
            MONITORINFO minfo;
            minfo.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &minfo);
            width = minfo.rcMonitor.right - minfo.rcMonitor.left;
            height = minfo.rcMonitor.bottom - minfo.rcMonitor.top;
            x = minfo.rcMonitor.left;
            y = minfo.rcMonitor.top;
            // if(minfo.dwFlags==MONITORINFOF_PRIMARY)
            //     Sys::cout << "Fullscreened on the primary monitor." << std::endl;
            // else
            //     Sys::cout << "Fullscreened on a non-primary monitor." << std::endl;
        }
        SetWindowLongPtr(data->window, GWL_STYLE, WS_FULLSCREEN);
        MoveWindow(data->window, x, y, width, height, TRUE);
    }
    else
    {
        width = windowedWidth;
        height = windowedHeight;
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = width;
        rect.bottom = height;
        SetWindowLongPtr(data->window, GWL_STYLE, WS_WINDOWED);
        AdjustWindowRect(&rect, WS_WINDOWED, FALSE);
        MoveWindow(data->window, windowedX, windowedY, rect.right - rect.left, rect.bottom - rect.top, TRUE);
        x = windowedX;
        y = windowedY;
    }

    return true;
}

bool Window::Resize(u32 w, u32 h)
{
    if (!open)
    {
        error = "Window hasn't been created yet";
        return false;
    }
    if (fullscreen)
    {
        error = "Fullscreen windows can't be resized";
        return false;
    }
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = w;
    rect.bottom = h;
    AdjustWindowRect(&rect, WS_WINDOWED, FALSE);
    SetWindowPos(data->window, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    return true;
}

bool Window::Update()
{
    MSG msg;
    while (PeekMessage(&msg, data->window, 0, 0, PM_REMOVE))
    {
        if (quit || msg.message == WM_QUIT)
        {
            return false;
        }
        else
        {
            if (msg.message == WM_SETFOCUS)
            {
                focusedWindow = this;
                focused = true;
            }
            if (msg.message == WM_KEYDOWN)
            {
                if (KC_KEY_F11 == KeyCodeFromWinScan((u8)(msg.lParam >> 16)))
                    Fullscreen(!fullscreen);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return true;
}

String Window::InputName(u8 keyCode) const
{
    return winGetInputName(keyCode);
}

} // namespace io

} // namespace AzCore