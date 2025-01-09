TEMPLATE = subdirs

SUBDIRS += \
    third-party/magma/projects/qt/magma.pro \
    framework \
    01-clear

framework.depends = third-party/magma/projects/qt/magma.pro

01-clear.depends += third-party/magma/projects/qt/magma.pro
01-clear.depends += framework
