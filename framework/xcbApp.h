#pragma once
#include <xcb/xcb.h>
#include "application.h"

class XcbApp : public BaseApp
{
public:
    XcbApp(const AppEntry& entry, String caption, uint32_t width, uint32_t height);
    virtual ~XcbApp();
    virtual void setWindowCaption(String caption) override;
    virtual void show() const override;
    virtual void run() override;
    virtual void onKeyUp(char key, int repeat, uint32_t flags) override {};
    virtual void onMouseMove(int x, int y) override;
    virtual void onMouseLButton(bool down, int x, int y) override;
    virtual void onMouseRButton(bool down, int x, int y) override {}
    virtual void onMouseMButton(bool down, int x, int y) override {}
    virtual void onMouseWheel(float delta) override {}

protected:
    xcb_connection_t *connection = nullptr;
    xcb_window_t window = 0;
    xcb_screen_t *screen = nullptr;

private:
    virtual char translateKey(int code) const override;
    void handleEvent(const xcb_generic_event_t *event);    
    xcb_intern_atom_reply_t *getAtom(const char *name, bool shouldExists) const;

    xcb_intern_atom_reply_t *deleteWindow = nullptr;
    xcb_point_t lastPos = {0, 0};
    xcb_point_t currPos = {0, 0};
};
