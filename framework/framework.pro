QT += widgets

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++17

INCLUDEPATH += $(VULKAN_SDK)/include
INCLUDEPATH += ../third-party/

QMAKE_CXXFLAGS += -std=c++17 -msse4 -ftemplate-depth=2048 -fconstexpr-depth=2048

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
QMAKE_CXXFLAGS += -Wno-unknown-pragmas
QMAKE_CXXFLAGS += -Wno-deprecated-copy

QMAKE_CXXFLAGS += -DQT_NO_FOREACH

win32 {
    QMAKE_CXXFLAGS += -DVK_USE_PLATFORM_WIN32_KHR
}

unix {
    QMAKE_CXXFLAGS += -VK_USE_PLATFORM_XCB_KHR
}

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

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target
