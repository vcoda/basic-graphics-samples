#pragma once
#include "application.h"

class QApplication;
class QWindow;

class QtApp : public BaseApp
{
public:
    QtApp(AppEntry entry, const std::tstring& caption, uint32_t width, uint32_t height);
    ~QtApp();
    virtual void setWindowCaption(const std::tstring& caption) override;
    virtual void show() override;
    virtual void run() override;
    virtual void close() override;
    virtual void onKeyDown(char key, int repeat, uint32_t flags) override;
    virtual void onKeyUp(char /* key */, int /* repeat */, uint32_t /* flags */) override {}
    virtual void onMouseMove(int x, int y) override;
    virtual void onMouseLButton(bool down, int x, int y) override;
    virtual void onMouseRButton(bool /* down */, int /* x */, int /* y */) override  {}
    virtual void onMouseMButton(bool /* down */, int /* x */, int /* y */) override  {}
    virtual void onMouseWheel(float /* delta */) override  {}
    virtual char translateKey(int code) const override;

protected:
    std::unique_ptr<QApplication> app;
    std::unique_ptr<QWindow> window;

    struct Pos
    {
        int x = 0;
        int y = 0;
    } currPos, lastPos;
};
