#include <cstring>
#include <cassert>
#include <xcb/xcb_icccm.h> // libxcb-icccm4-dev
#include "xcbApp.h"

XcbApp::XcbApp(const AppEntry& entry, String caption, uint32_t width, uint32_t height):
    BaseApp(caption, width, height)
{
    std::cout << "Platform: XCB" << std::endl;
    connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection)) 
        throw std::runtime_error("failed to open connection to X server");

    // Get the first screen
    const xcb_setup_t *setup = xcb_get_setup(connection);
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    screen = it.data;
    assert(screen);

    // Set window attributes
    const uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[32];
    values[0] = screen->black_pixel;
    values[1] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE;

    // Create window
    window = xcb_generate_id(connection);
    xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 
        0, 0, width, height, 0, // border width
        XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
        mask, values);

    // Set fixed window size
    xcb_size_hints_t hints = {};
    hints.flags = XCB_ICCCM_SIZE_HINT_P_MIN_SIZE | XCB_ICCCM_SIZE_HINT_P_MAX_SIZE;
    hints.min_width = hints.max_width = static_cast<int32_t>(width);
    hints.min_height = hints.max_height = static_cast<int32_t>(height);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS,
        sizeof(int32_t) * 8, sizeof(xcb_size_hints_t)/sizeof(int32_t), &hints);

    setWindowCaption(caption);

    // Catch window close
    xcb_intern_atom_reply_t *protocols = getAtom("WM_PROTOCOLS", true);
    deleteWindow = getAtom("WM_DELETE_WINDOW", false);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window,
        protocols->atom, XCB_ATOM_ATOM,
        sizeof(xcb_atom_t) * 8, 1, &deleteWindow->atom);
    free(protocols);
}

XcbApp::~XcbApp()
{
    free(deleteWindow);
    xcb_destroy_window(connection, window);
    xcb_disconnect(connection);
}

void XcbApp::setWindowCaption(String caption)
{
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window,
        XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
        sizeof(char) * 8, strlen(caption), caption);
}

void XcbApp::show() const
{
    uint32_t coords[2] = {0, 0};
    if (width < screen->width_in_pixels && 
        height < screen->height_in_pixels)
    {   // Place window in the center of the desktop
        coords[0] = (screen->width_in_pixels - width) / 2;
        coords[1] = (screen->height_in_pixels - height) / 2;
    }
    xcb_map_window(connection, window);
    xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);    
    xcb_flush(connection);
}

void XcbApp::run()
{
    xcb_flush(connection);
    while (!quit) 
    {
        xcb_generic_event_t *event = xcb_poll_for_event(connection);
        while (event) 
        {
            handleEvent(event);
            free(event);
            event = xcb_poll_for_event(connection);
        }
        onIdle();
    }
}

void XcbApp::onMouseMove(int x, int y)
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

void XcbApp::onMouseLButton(bool down, int x, int y)
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

char XcbApp::translateKey(int code) const
{
    switch (code)
    {
    case 0x09: return AppKey::Escape;
    case 0x17: return AppKey::Tab;
    case 0x24: return AppKey::Enter;
    case 0x41: return AppKey::Space;
    case 0x6E: return AppKey::Home;
    case 0x6F: return AppKey::Up;
    case 0x70: return AppKey::PgUp;
    case 0x71: return AppKey::Left;
    case 0x72: return AppKey::Right;
    case 0x73: return AppKey::End;
    case 0x74: return AppKey::Down;
    case 0x75: return AppKey::PgDn;
    default:
        // Map to ASCII 0..9
        if (code >= 0xA && code <= 0x12)
            return '1' + (code - 0xA);
        else if (0x13 == code)
            return '0';
    }
    return AppKey::Null;
}

void XcbApp::handleEvent(const xcb_generic_event_t *event)
{
    const uint8_t eventType = event->response_type & 0x7f;
    switch (eventType)
    {
    case XCB_KEY_PRESS:
        {
            const xcb_key_press_event_t *keyPress = reinterpret_cast<const xcb_key_press_event_t *>(event);
            const char key = translateKey(keyPress->detail);
            onKeyDown(key, 0, 0);
        }
        break;
    case XCB_KEY_RELEASE:
        {
            const xcb_key_release_event_t *keyRelease = reinterpret_cast<const xcb_key_release_event_t *>(event);
            const char key = translateKey(keyRelease->detail);
            onKeyUp(key, 0, 0);
        }
        break;
    case XCB_BUTTON_PRESS:
        {
            const xcb_button_press_event_t *btnPress = reinterpret_cast<const xcb_button_press_event_t *>(event);
            switch (btnPress->detail)
            {
            case XCB_BUTTON_INDEX_1: onMouseLButton(true, btnPress->event_x, btnPress->event_y); break;
            case XCB_BUTTON_INDEX_2: onMouseMButton(true, btnPress->event_x, btnPress->event_y); break;
            case XCB_BUTTON_INDEX_3: onMouseRButton(true, btnPress->event_x, btnPress->event_y); break;
            case XCB_BUTTON_INDEX_4: onMouseWheel(1.0f); break;
            case XCB_BUTTON_INDEX_5: onMouseWheel(-1.0f); break;
            }
        }
        break;
    case XCB_BUTTON_RELEASE:
        {
            const xcb_button_press_event_t *btnRelease = reinterpret_cast<const xcb_button_release_event_t *>(event);
            switch (btnRelease->detail)
            {
            case XCB_BUTTON_INDEX_1: onMouseLButton(false, btnRelease->event_x, btnRelease->event_y); break;
            case XCB_BUTTON_INDEX_2: onMouseMButton(false, btnRelease->event_x, btnRelease->event_y); break;
            case XCB_BUTTON_INDEX_3: onMouseRButton(false, btnRelease->event_x, btnRelease->event_y); break;
            }
        }
        break;
    case XCB_MOTION_NOTIFY:
        {
            const xcb_motion_notify_event_t *mouseMotion = reinterpret_cast<const xcb_motion_notify_event_t *>(event);
            onMouseMove(mouseMotion->event_x, mouseMotion->event_y);
        }
        break;
    case XCB_EXPOSE:
        {
            onPaint();
        }
        break;
    case XCB_CLIENT_MESSAGE:
        {
            const xcb_client_message_event_t *clientMsg = reinterpret_cast<const xcb_client_message_event_t *>(event);
            if (clientMsg->data.data32[0] == deleteWindow->atom)
                close();
        }
        break;
    }
}

xcb_intern_atom_reply_t *XcbApp::getAtom(const char *name, bool shouldExists) const
{
    const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, shouldExists ? 1 : 0,
        (uint16_t)strlen(name), name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, NULL);
    assert(reply);
    return reply;
}
