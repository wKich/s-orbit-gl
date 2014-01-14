QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test-2d-orbit-opengl
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp

QMAKE_CXXFLAGS += -std=c++0x

HEADERS += \
    mainwindow.h

RESOURCES += \
    main.qrc

OTHER_FILES += \
    main.vert \
    main.frag
