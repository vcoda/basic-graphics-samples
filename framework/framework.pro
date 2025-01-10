QT += widgets

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++17

SOURCES += \
    graphicsPipeline.cpp \
    main.cpp \
    qtApp.cpp \
    utilities.cpp \
    vulkanApp.cpp

HEADERS += \
    alignedAllocator.h \
    application.h \
    debugOutputStream.h \
    graphicsPipeline.h \
    platform.h \
    qtApp.h \
    shaderReflectionFactory.h \
    timer.h \
    utilities.h \
    vulkanApp.h

INCLUDEPATH += $(VULKAN_SDK)/include
INCLUDEPATH += ../third-party/

QMAKE_CXXFLAGS += -std=c++17 -msse4 -ftemplate-depth=2048 -fconstexpr-depth=2048
QMAKE_CXXFLAGS += -Wno-unknown-pragmas
QMAKE_CXXFLAGS += -Wno-deprecated-copy
QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
QMAKE_CXXFLAGS += -Wno-missing-field-initializers

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

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target
