#-------------------------------------------------
#
# Project created by QtCreator 2015-11-20T22:52:05
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = TodakServer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    clienthandler.cpp \
    webserver.cpp

HEADERS += \
    webserver.h \
    clienthandler.h \
    structure.h

DISTFILES += \
    README.txt
