QT += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = ChatClient
TEMPLATE = app

SOURCES += \
    main.cpp \
    loginwindow.cpp \
    mainwindow.cpp \
    chatwindow.cpp \
    chatclient.cpp \
    database.cpp

HEADERS += \
    loginwindow.h \
    mainwindow.h \
    chatwindow.h \
    chatclient.h \
    database.h

FORMS += \
    loginwindow.ui \
    mainwindow.ui \
    chatwindow.ui

RESOURCES += \
    resources.qrc
