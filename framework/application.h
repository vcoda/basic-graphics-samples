#pragma once
#include <cstdint>
#include <iostream>
#include <memory>
#ifdef VK_USE_PLATFORM_WIN32_KHR
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGID
    #define NOGDI
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>
#endif
#include "platform.h"
#include "magma/src/core/pch.h"

class IApplication : public AlignAs<16>,
    public magma::IClass
{
public:
    virtual void setWindowCaption(const std::tstring& caption) = 0;
    virtual void show() = 0;
    virtual void run() = 0;
    virtual void close() = 0;
    virtual void onIdle() = 0;
    virtual void onPaint() = 0;
    virtual void onKeyDown(char key, int repeat, uint32_t flags) = 0;
    virtual void onKeyUp(char key, int repeat, uint32_t flags) = 0;
    virtual void onMouseMove(int x, int y) = 0;
    virtual void onMouseLButton(bool down, int x, int y) = 0;
    virtual void onMouseRButton(bool down, int x, int y) = 0;
    virtual void onMouseMButton(bool down, int x, int y) = 0;
    virtual void onMouseWheel(float distance) = 0;

protected:
    virtual char translateKey(int code) const = 0;
};

enum AppKey { Null = '\0', Tab = '\t', Enter = '\r', Escape = '\033', Space = ' ',
    Left = 0xA, Right = 0xB, Up = 0xC, Down = 0xD,
    Home = 0x11, End = 0x12, PgUp = 0x13, PgDn = 0x14
};

struct AppEntry
{
#if defined(_WIN32) && !defined(QT_CORE_LIB)
    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    LPSTR lpCmdLine;
    int nCmdShow;
#else
    int argc;
    char **argv;
#endif // _WIN32 || !QT_CORE_LIB
};

class BaseApp : public IApplication
{
public:
    virtual void onKeyDown(char key, int /* repeat */, uint32_t /* flags */) override
    {
        if (AppKey::Escape == key)
            close();
    }

protected:
    BaseApp(const std::tstring& caption, uint32_t width, uint32_t height):
        caption(caption), width(width), height(height) {}
    virtual void close() override { quit = true; }

protected:
    std::tstring caption;
    uint32_t width, height;
    bool mousing = false;
    float spinX = 0.f;
    float spinY = 0.f;
    bool quit = false;
};

std::unique_ptr<IApplication> appFactory(const AppEntry&);
