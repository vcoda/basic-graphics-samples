#include "winApp.h"

Win32App *Win32App::self;
DebugOutputStream Win32App::dostream;

Win32App::Win32App(const AppEntry& entry, const std::tstring& caption, uint32_t width, uint32_t height):
    BaseApp(caption, width, height),
    hInstance(entry.hInstance),
    hWnd(NULL)
{
    Win32App::self = this;
    HICON icon = (HICON)LoadImage(NULL, TEXT("..\\framework\\resources\\vulkan.ico"),
        IMAGE_ICON, 64, 64, LR_LOADFROMFILE);

    // Register window class
    const WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        Win32App::wndProc,
        0, 0,
        entry.hInstance,
        icon,
        LoadCursor(NULL, IDC_ARROW),
        NULL, NULL, TEXT("demo"),
        icon
    };
    RegisterClassEx(&wc);

    // Create window
    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    hWnd = CreateWindow(wc.lpszClassName, caption.c_str(), style,
        0, 0, width, height,
        NULL, NULL, wc.hInstance, NULL);

    // Adjust window size according to style
    RECT rc = {0L, 0L, (LONG)width, (LONG)height};
    AdjustWindowRect(&rc, style, FALSE);
    SetWindowPos(hWnd, HWND_TOP, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_HIDEWINDOW);

    setWindowCaption(caption);
}

Win32App::~Win32App()
{
    DestroyWindow(hWnd);
    UnregisterClass(TEXT("demo"), hInstance);
}

void Win32App::setWindowCaption(const std::tstring& caption)
{
    SetWindowText(hWnd, caption.c_str());
}

void Win32App::show()
{
    // Get desktop resolution
    const HWND hDesktopWnd = GetDesktopWindow();
    RECT desktopRect;
    GetWindowRect(hDesktopWnd, &desktopRect);
    // Get window size
    RECT rc;
    GetWindowRect(hWnd, &rc);
    const int cx = static_cast<int>(rc.right - rc.left);
    const int cy = static_cast<int>(rc.bottom - rc.top);
    int x = 0, y = 0;
    if (cx < desktopRect.right && cy < desktopRect.bottom)
    {   // Place window in the center of the desktop
        x = (desktopRect.right - cx) / 2;
        y = (desktopRect.bottom - cy) / 2;
    }
    SetWindowPos(hWnd, HWND_TOP, x, y, cx, cy, SWP_SHOWWINDOW);
}

void Win32App::run()
{
    while (!quit)
    {
        MSG msg;
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            if (!IsIconic(hWnd))
                onIdle();
        }
    }
}

void Win32App::onMouseMove(int x, int y)
{
    currPos.x = x;
    currPos.y = y;
    if (mousing)
    {
        spinX += (currPos.x - lastPos.x);
        spinY += (currPos.y - lastPos.y);
    }
    lastPos = currPos;
}

void Win32App::onMouseLButton(bool down, int x, int y)
{
    if (down)
    {
        lastPos.x = currPos.x = x;
        lastPos.y = currPos.y = y;
        mousing = true;
    }
    else
    {
        mousing = false;
    }
}

char Win32App::translateKey(int code) const
{
    switch (code)
    {
    case VK_TAB: return AppKey::Tab;
    case VK_RETURN: return AppKey::Enter;
    case VK_ESCAPE: return AppKey::Escape;
    case VK_LEFT: return AppKey::Left;
    case VK_RIGHT: return AppKey::Right;
    case VK_UP: return AppKey::Up;
    case VK_DOWN: return AppKey::Down;
    case VK_HOME: return AppKey::Home;
    case VK_END: return AppKey::End;
    case VK_PRIOR: return AppKey::PgUp;
    case VK_NEXT: return AppKey::PgDn;
    }
    return (char)code;
}

LRESULT WINAPI Win32App::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        {
            const char key = self->translateKey((int)wParam);
            self->onKeyDown(key, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam));
        }
        break;
    case WM_KEYUP:
        {
            const char key = self->translateKey((int)wParam);
            self->onKeyUp(key, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam));
        }
        break;
    case WM_MOUSEMOVE:
        self->onMouseMove((int)LOWORD(lParam), (int)HIWORD(lParam));
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        self->onMouseLButton((WM_LBUTTONDOWN == msg), (int)LOWORD(lParam), (int)HIWORD(lParam));
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        self->onMouseRButton((WM_RBUTTONDOWN == msg), (int)LOWORD(lParam), (int)HIWORD(lParam));
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        self->onMouseMButton((WM_MBUTTONDOWN == msg), (int)LOWORD(lParam), (int)HIWORD(lParam));
        break;
    case WM_MOUSEWHEEL:
        {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            self->onMouseWheel(delta/(float)WHEEL_DELTA);
        }
        break;
#ifndef _DEBUG
    case WM_PAINT:
        Win32App::self->onPaint();
        break;
#endif
    case WM_CLOSE:
        self->close();
        break;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
