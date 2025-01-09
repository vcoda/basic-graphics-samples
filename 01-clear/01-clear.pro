QT += widgets

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle

SOURCES += \
        01-clear.cpp

INCLUDEPATH += $(VULKAN_SDK)/include
INCLUDEPATH += ../third-party/

QMAKE_CXXFLAGS += -std=c++17 -msse4 -ftemplate-depth=2048 -fconstexpr-depth=2048

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
QMAKE_CXXFLAGS += -Wno-unknown-pragmas
QMAKE_CXXFLAGS += -Wno-deprecated-copy
QMAKE_CXXFLAGS += -Wno-strict-aliasing

QMAKE_CXXFLAGS += -DQT_NO_FOREACH

win32 {
    QMAKE_CXXFLAGS += -DVK_USE_PLATFORM_WIN32_KHR
} else:macx {
    QMAKE_CXXFLAGS += -DVK_USE_PLATFORM_MACOS_MVK
} else:unix {
    QMAKE_CXXFLAGS += -DVK_USE_PLATFORM_XCB_KHR
} else:android {
    QMAKE_CXXFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
} else:ios {
    QMAKE_CXXFLAGS += -DVK_USE_PLATFORM_IOS_MVK
}

LIBS += -L$(VULKAN_SDK)/Lib
CONFIG(debug, debug|release) {
    LIBS += -L../framework/debug
    LIBS += -L../third-party/magma/projects/qt/debug
} else {
    LIBS += -L../framework/release
    LIBS += -L../third-party/magma/projects/qt/release
}
LIBS += -lframework -lmagma -lvulkan-1
