#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "application.h"

class XlibApp : public BaseApp
{
public:
    XlibApp(const AppEntry& entry, const std::tstring& caption, uint32_t width, uint32_t height);
    virtual ~XlibApp();
    virtual void setWindowCaption(const std::tstring& caption) override;
    virtual void show() const override;
    virtual void run() override;
    virtual void onKeyUp(char key, int repeat, uint32_t flags) override {};
    virtual void onMouseMove(int x, int y) override;
    virtual void onMouseLButton(bool down, int x, int y) override;
    virtual void onMouseRButton(bool down, int x, int y) override {}
    virtual void onMouseMButton(bool down, int x, int y) override {}
    virtual void onMouseWheel(float delta) override {}

protected:
    Display *dpy = nullptr;
    Window window = 0;

private:
    virtual char translateKey(int code) const override;
    void handleEvent(const XEvent& event);

    Atom deleteWindow = 0;
    Atom protocols = 0;
    XPoint lastPos = {0, 0};
    XPoint currPos = {0, 0};
};
