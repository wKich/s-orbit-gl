QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = s-orbit-gl
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp

QMAKE_CXXFLAGS += -std=c++0x

HEADERS += \
    mainwindow.h \
    pointdouble2d.h

RESOURCES += \
    main.qrc

OTHER_FILES += \
    main.vert \
    main.frag
