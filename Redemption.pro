QT += core gui widgets dbus network concurrent

TARGET = Redemption
TEMPLATE = app

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    playbackengine.cpp \
    metadatareader.cpp \
    filetreemodel.cpp \
    mprisplayer.cpp

HEADERS += \
    mainwindow.h \
    playbackengine.h \
    metadatareader.h \
    filetreemodel.h \
    seekslider.h \
    coverartdialog.h \
    settingsdialog.h \
    searchdialog.h \
    mprisplayer.h \
    toastnotification.h \
    metadatadialog.h \
    scrollinglabel.h \
    ambientbar.h \
    playlistview.h \
    nowplayingpanel.h

# mpv
LIBS += -lmpv

# TagLib
CONFIG += link_pkgconfig
PKGCONFIG += taglib

OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui

target.path = /usr/bin
desktop.path = /usr/share/applications
desktop.files = redemption.desktop
icon.path = /usr/share/icons/hicolor/scalable/apps
icon.files = redemption.svg

INSTALLS += target desktop icon
