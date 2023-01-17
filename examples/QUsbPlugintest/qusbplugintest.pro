QT += core
TARGET = qusbplugintest

Release:DESTDIR = release
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc

include($$PWD/../../QUsbPlugin/QUsbPlugin.pri)



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

SOURCES += \
    qusbplugintest.cpp
