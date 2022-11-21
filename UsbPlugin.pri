# DEFINES += OS_DARWIN
# DEFINES += OS_HAIKU
# DEFINES += OS_LINUX
# DEFINES += OS_EMSCRIPTEN
# DEFINES += OS_NETBSD
# DEFINES += OS_NULL
# DEFINES += OS_OPENBSD
# DEFINES += OS_SUNOS
 DEFINES += OS_WINDOWS

# DEFINES += PLATFORM_POSIX

# DEFINES += USE_UDEV

# DEFINES += IS_ANDROID

# DEFINES += IS_MINGW

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/libusb
INCLUDEPATH += $$PWD/libusb/os

HEADERS += \
    $$PWD/libusb/libusb.h \
    $$PWD/libusb/libusbi.h \
    $$PWD/libusb/os/sunos_usb.h \
    $$PWD/libusb/version.h \
    $$PWD/libusb/version_nano.h

SOURCES += \
    $$PWD/libusb/core.c \
    $$PWD/libusb/descriptor.c \
    $$PWD/libusb/hotplug.c \
    $$PWD/libusb/io.c \
    $$PWD/libusb/strerror.c \
    $$PWD/libusb/sync.c

if(contains(DEFINES,OS_DARWIN)) {

HEADERS += \
    $$PWD/libusb/os/darwin_usb.h \

SOURCES += \
    $$PWD/libusb/os/darwin_usb.c \

}

if(contains(DEFINES,OS_HAIKU)) {

HEADERS += \
    $$PWD/libusb/os/haiku_usb.h \
    $$PWD/libusb/os/haiku_usb_raw.h \

SOURCES += \
    $$PWD/libusb/os/haiku_pollfs.cpp \
    $$PWD/libusb/os/haiku_usb_backend.cpp \
    $$PWD/libusb/os/haiku_usb_raw.cpp \

}


if(contains(DEFINES,OS_LINUX)) {

HEADERS += \
    $$PWD/libusb/os/linux_usbfs.h \

SOURCES += \
    $$PWD/libusb/os/linux_usbfs.c \

if(contains(DEFINES,USE_UDEV)) {

SOURCES += \
    $$PWD/libusb/os/linux_udev.c \

} else {

SOURCES += \
    $$PWD/libusb/os/linux_netlink.c \

}

}


if(contains(DEFINES,OS_EMSCRIPTEN)) {

#HEADERS += \

SOURCES += \
    $$PWD/libusb/os/emscripten_webusb.cpp \

include($$PWD/emscripten/config.pri)

}


if(contains(DEFINES,OS_NETBSD)) {

# HEADERS += \

SOURCES += \
    $$PWD/libusb/os/netbsd_usb.c \

}


if(contains(DEFINES,OS_NULL)) {

# HEADERS += \

SOURCES += \
    $$PWD/libusb/os/null_usb.c \

}

if(contains(DEFINES,OS_OPENBSD)) {

# HEADERS += \

SOURCES += \
    $$PWD/libusb/os/openbsd_usb.c \

}

if(contains(DEFINES,OS_SUNOS)) {

# HEADERS += \

SOURCES += \
    $$PWD/libusb/os/sunos_usb.c \

}

if(contains(DEFINES,OS_WINDOWS)) {

HEADERS += \
    $$PWD/libusb/os/windows_common.h \
    $$PWD/libusb/os/windows_usbdk.h \
    $$PWD/libusb/os/windows_winusb.h \

SOURCES += \
    $$PWD/libusb/os/windows_common.c \
    $$PWD/libusb/os/windows_usbdk.c \
    $$PWD/libusb/os/windows_winusb.c \

}

if(contains(DEFINES,PLATFORM_POSIX)) {

HEADERS += \
    $$PWD/libusb/os/events_posix.h \
    $$PWD/libusb/os/threads_posix.h \

SOURCES += \
    $$PWD/libusb/os/events_posix.c \
    $$PWD/libusb/os/threads_posix.c \

} else {

DEFINES += HAVE_STRING_H

HEADERS += \
    $$PWD/libusb/os/events_windows.h \
    $$PWD/libusb/os/threads_windows.h \

SOURCES += \
    $$PWD/libusb/os/events_windows.c \
    $$PWD/libusb/os/threads_windows.c \

}



macx {
# mac only
include($$PWD/Xcode/config.pri)
}
unix:!macx{
# linux only
if(contains(DEFINES,IS_ANDROID)) {
    include($$PWD/android/config.pri)
} else {

}

}
win32 {
# windows only
HEADERS += \
    $$PWD/libusb/libusb-1.0.rc \

DISTFILES += \
    $$PWD/libusb/libusb-1.0.def

if(contains(DEFINES,PLATFORM_POSIX)) {

    include($$PWD/default/config.pri)

} else {

if(contains(DEFINES, IS_MINGW)) {
    include($$PWD/mingw/config.pri)
} else {
    include($$PWD/msvc/config.pri)
}

}

}



