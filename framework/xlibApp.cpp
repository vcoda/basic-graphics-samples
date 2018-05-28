#include "xlibApp.h"

XlibApp::XlibApp(const AppEntry& entry, const std::tstring& caption, uint32_t width, uint32_t height):
    BaseApp(caption, width, height)
{
    std::cout << "Platform: Xlib" << std::endl;
    XInitThreads();
    dpy = XOpenDisplay(NULL);
    if (!dpy)
        throw std::runtime_error("failed to open connection to X server");

    // Select visual
    XVisualInfo infoTemplate = {};
    infoTemplate.screen = XDefaultScreen(dpy);
    int visualCount = 0;
    XVisualInfo *visual = XGetVisualInfo(dpy, VisualScreenMask, &infoTemplate, &visualCount);

    // Create colormap
    const Window rootWindow = RootWindow(dpy, infoTemplate.screen);
    const Colormap cm = XCreateColormap(dpy, rootWindow, visual->visual, AllocNone);

    // Set window attributes
    XSetWindowAttributes windowAttribs = {};
    windowAttribs.background_pixel = 0xFFFFFFFF;
    windowAttribs.border_pixel = 0;
    windowAttribs.event_mask = KeyPressMask | KeyReleaseMask |
                               ButtonPressMask | ButtonReleaseMask |
                               PointerMotionMask | ExposureMask;
    windowAttribs.colormap = cm;

    // Create window
    window = XCreateWindow(dpy,
        rootWindow,
        0, 0,
        width, height,
        0,
        visual->depth,
        InputOutput,
        visual->visual,
        CWBackPixel | CWBorderPixel | CWEventMask | CWColormap,
        &windowAttribs);
    if (!window)
        throw std::runtime_error("failed to create X window");

    // Set fixed window size
    XSizeHints hints = {};
    hints.flags = PMinSize | PMaxSize;
    hints.min_width = hints.max_width = width;
    hints.min_height = hints.max_height = height;

    XSetStandardProperties(dpy, window,
        caption.c_str(), caption.c_str(), None,
        entry.argv, entry.argc,
        &hints);

    // Catch window close
    deleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    protocols = XInternAtom(dpy, "WM_PROTOCOLS", True);
    XSetWMProtocols(dpy, window, &deleteWindow, 1);
}

XlibApp::~XlibApp()
{
    XDestroyWindow(dpy, window);
    XCloseDisplay(dpy);
}

void XlibApp::setWindowCaption(const std::tstring& caption)
{
    XStoreName(dpy, window, caption.c_str());
}

void XlibApp::show() const
{
    const Screen *screen = DefaultScreenOfDisplay(dpy);
    XWindowChanges changes = {};
    changes.x = 0;
    changes.y = 0;
    if (width < screen->width && height < screen->height)
    {   // Place window in the center of the desktop
        changes.x = (screen->width - width) / 2;
        changes.y = (screen->height - height) / 2;
    }
    XMapRaised(dpy, window);
    XConfigureWindow(dpy, window, CWX | CWY, &changes);
    XFlush(dpy);
}

void XlibApp::run()
{
    while (!quit)
    {
        while (XPending(dpy))
        {
            XEvent event;
            XNextEvent(dpy, &event);
            handleEvent(event);
        }
        onIdle();
    }
}

void XlibApp::onMouseMove(int x, int y)
{
    currPos.x = static_cast<short>(x);
    currPos.y = static_cast<short>(y);
    if (mousing)
    {
        spinX += (currPos.x - lastPos.x);
        spinY += (currPos.y - lastPos.y);
    }
    lastPos = currPos;
}

void XlibApp::onMouseLButton(bool down, int x, int y)
{
    if (down)
    {
        lastPos.x = currPos.x = static_cast<short>(x);
        lastPos.y = currPos.y = static_cast<short>(y);
        mousing = true;
    }
    else
    {
        mousing = false;
    }
}

char XlibApp::translateKey(int code) const
{
    switch (code)
    {    
    case XK_Tab: return AppKey::Tab;
    case XK_Return: return AppKey::Enter;
    case XK_Escape: return AppKey::Escape;
    case XK_Left: return AppKey::Left;
    case XK_Right: return AppKey::Right;
    case XK_Up: return AppKey::Up;
    case XK_Down: return AppKey::Down;
    case XK_Home: return AppKey::Home;
    case XK_End: return AppKey::End;
    case XK_Prior: return AppKey::PgUp;
    case XK_Next: return AppKey::PgDn;
    }
    return (char)code;
}

void XlibApp::handleEvent(const XEvent& event)
{
    switch (event.type)
    {
    case KeyPress:
        {
            const KeySym sym = XKeycodeToKeysym(dpy, event.xkey.keycode, 0);
            const char key = translateKey(static_cast<int>(sym));
            onKeyDown(key, 0, 0);
        }
        break;
    case KeyRelease:
        {
            const KeySym sym = XKeycodeToKeysym(dpy, event.xkey.keycode, 0);
            const char key = translateKey(static_cast<int>(sym));
            onKeyUp(key, 0, 0);
        }
        break;
    case ButtonPress:
        switch (event.xbutton.button)
        {
        case 1: onMouseLButton(true, event.xbutton.x, event.xbutton.y); break;
        case 2: onMouseMButton(true, event.xbutton.x, event.xbutton.y); break;
        case 3: onMouseRButton(true, event.xbutton.x, event.xbutton.y); break;
        case 4: onMouseWheel(1.0f); break;
        case 5: onMouseWheel(-1.0f); break;
        }
        break;
    case ButtonRelease:
        switch (event.xbutton.button)
        {
        case 1: onMouseLButton(false, event.xbutton.x, event.xbutton.y); break;
        case 2: onMouseMButton(false, event.xbutton.x, event.xbutton.y); break;
        case 3: onMouseRButton(false, event.xbutton.x, event.xbutton.y); break;
        }
        break;
    case MotionNotify:
        {
            onMouseMove(event.xmotion.x, event.xmotion.y);
        }
        break;
    case Expose:
        {
            onPaint();
        }
        break;
    case ClientMessage:
        if (event.xclient.message_type == protocols)
        {
            if (event.xclient.data.l[0] == (long)deleteWindow)
                close();
        }
        break;
    default:
        break;
    }
}
