#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "version.h"

#include <QMessageBox>

#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QTreeWidgetItem>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QInputDialog>
#include <QFileDialog>

#include "xmloutputparser.h"
#include "treewidgetitem.h"

#include <QCloseEvent>
#include <QSettings>

#include <QDebug>

#define STRING(b) (b?"true":"false")

#define XML_VERSION "0.0.1"

namespace Ui {
class MainWindow;
class TestEdit;
class ProjectEdit;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

    bool saveXML();
    void saveItem(QXmlStreamWriter &xml, QTreeWidgetItem *item);
    void parseXML(QString filepath);
    void readProject(QXmlStreamReader &xml, QTreeWidgetItem *item);
    void readTest(QXmlStreamReader &xml, QTreeWidgetItem *item);

    QTreeWidgetItem* createChildItem(QTreeWidgetItem *item);

    void startTests(QTreeWidgetItem *item);

    void clearStatus(QTreeWidgetItem *item);

    QMessageBox::StandardButton saveBeforeClose();

    void sort();

private:
    Ui::MainWindow *ui;

    int m_nbTest, m_passed, m_failed, m_skipped;

    QMap<QProcess*,QTreeWidgetItem*> m_mapProcess;

    QAction *m_startAction;

    QIcon m_projectIcon, m_testIcon;

    QString m_currentFilepath;
    QString m_currentFilename;
    bool m_modified;

    QTemporaryDir m_outputDir;

public slots:
    void showContextMenu(QPoint);

    void addProject();
    void addTest();

    void startTests();
    void finished(int errorCode, QProcess::ExitStatus status);

    bool pressNew();
    void pressOpen();
    bool pressSave();
    bool pressSaveAs();
    bool pressClose();
    void pressQuit();

    void pressEdit();
    void pressRename();
    void pressDelete();

    void documentModified();

    void displayItemInfo();

};

#endif // MAINWINDOW_H
