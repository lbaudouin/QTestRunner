QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QTestRunner
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    xmloutputparser.cpp

HEADERS  += mainwindow.h \
    version.h \
    xmloutputparser.h \
    treewidgetitem.h \
    testeditdialog.h \
    projecteditdialog.h

FORMS    += mainwindow.ui \
    testedit.ui \
    projectedit.ui

RESOURCES += \
    ressources.qrc
