QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QTestRunner
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    xmloutputparser.cpp

HEADERS  += mainwindow.h \
    version.h \
    xmloutputparser.h

FORMS    += mainwindow.ui

RESOURCES += \
    ressources.qrc
