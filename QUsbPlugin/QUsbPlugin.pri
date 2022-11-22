QMAKE_CXXFLAGS += -std=c++11

include($$PWD/../UsbPlugin.pri)

HEADERS += \
    $$PWD/QUsbPlugin.h

SOURCES += \
    $$PWD/QUsbPlugin.cpp

INCLUDEPATH += $$PWD
