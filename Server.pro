QT += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = ChatServer
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    chatserver.cpp \
    serverworker.cpp \
    database.cpp

HEADERS += \
    mainwindow.h \
    chatserver.h \
    serverworker.h \
    database.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
