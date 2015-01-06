#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_testedit.h"
#include "ui_projectedit.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_projectIcon =  QIcon(":images/project");
    m_testIcon =  QIcon(":images/test");
    m_currentFilepath.clear();
    m_currentFilename.clear();
    m_modified = false;

    QStringList labels;
    labels << tr("Name") << tr("Status");

    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
    ui->treeWidget->setHeaderLabels(labels);

    ui->mainToolBar->addAction( m_projectIcon , tr("Add project"), this, SLOT(addProject()) );
    ui->mainToolBar->addAction( m_testIcon , tr("Add test"), this, SLOT(addTest()) );
    ui->mainToolBar->addSeparator();
    m_startAction = ui->mainToolBar->addAction(this->style()->standardIcon( QStyle::SP_ArrowRight ), tr("Start tests"), this, SLOT(startTests()) );

    //Load settings
    QSettings settings(qAppName(),qAppName());
    this->restoreGeometry(settings.value("ui/geometry").toByteArray());
    ui->splitter->restoreGeometry(settings.value("ui/splitter").toByteArray());

    //Connect menu actions
    connect(ui->action_New,SIGNAL(triggered()),this,SLOT(pressNew()));
    connect(ui->action_Open,SIGNAL(triggered()),this,SLOT(pressOpen()));
    connect(ui->action_Save,SIGNAL(triggered()),this,SLOT(pressSave()));
    connect(ui->actionSave_As,SIGNAL(triggered()),this,SLOT(pressSaveAs()));
    connect(ui->action_Close,SIGNAL(triggered()),this,SLOT(pressClose()));
    connect(ui->action_Quit,SIGNAL(triggered()),this,SLOT(pressQuit()));

    connect(ui->action_Edit,SIGNAL(triggered()),this,SLOT(pressEdit()));
    connect(ui->action_Rename,SIGNAL(triggered()),this,SLOT(pressRename()));
    connect(ui->action_Delete,SIGNAL(triggered()),this,SLOT(pressDelete()));

    connect(ui->treeWidget,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));

    connect(ui->treeWidget,SIGNAL(itemSelectionChanged()),this,SLOT(displayItemInfo()));

    //Test
    parseXML("test.xml");
}

MainWindow::~MainWindow()
{
    QList<QProcess*> processList = m_mapProcess.keys();
    foreach(QProcess *p, processList){
        m_mapProcess.remove(p);
        p->close();
    }

    QSettings settings(qAppName(),qAppName());
    settings.setValue("ui/geometry",this->saveGeometry());
    settings.setValue("ui/splitter",ui->splitter->saveGeometry());
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(saveBeforeClose()==QMessageBox::Cancel)
        event->ignore();
    else
        QMainWindow::closeEvent(event);
}

void MainWindow::documentModified()
{
    m_modified = true;
    this->setWindowTitle(QString("%1 - %2*").arg(tr("QTestRunner"),m_currentFilename));
}

void MainWindow::sort()
{
    ui->treeWidget->sortItems(0,Qt::AscendingOrder);
}

void MainWindow::addProject()
{
    QTreeWidgetItem *parentItem = ui->treeWidget->invisibleRootItem();

    if(ui->treeWidget->invisibleRootItem()->childCount()>0){
        QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
        if(selected.size()!=1 || selected.at(0)->data(0,Qt::UserRole).toString()!="project"){
            QMessageBox::warning(this,tr("Warning"),tr("Select one project"));
            return;
        }
        parentItem = selected.at(0);
    }

    QString name = QInputDialog::getText(this,tr("Add project"),tr("Project name:"),QLineEdit::Normal,tr("<empty_project>"));
    if(name.isEmpty())
        return;

    QTreeWidgetItem *projectItem = createChildItem(parentItem);
    projectItem->setIcon(0, m_projectIcon );
    projectItem->setData(0, Qt::UserRole, QString("project") );
    projectItem->setText(0, name);
    projectItem->setText(1,tr("New project"));
    projectItem->setFlags(projectItem->flags() | Qt::ItemIsTristate);
    projectItem->setExpanded(true);
    projectItem->setCheckState(0,Qt::Checked);

    documentModified();

    sort();
}

void MainWindow::addTest()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.size()!=1 || selected.at(0)->data(0,Qt::UserRole).toString()!="project"){
        QMessageBox::warning(this,tr("Warning"),tr("Select one project"));
        return;
    }

    QString name = QInputDialog::getText(this,tr("Add test"),tr("Test name:"),QLineEdit::Normal,tr("<empty_test>"));
    if(name.isEmpty())
        return;

    QTreeWidgetItem *testItem = createChildItem(selected.at(0));
    testItem->setIcon(0, m_testIcon );
    testItem->setData(0, Qt::UserRole, QString("test"));
    testItem->setText(0, name);
    testItem->setText(1, tr("New test"));
    testItem->setFlags(testItem->flags() ^ Qt::ItemIsTristate);
    testItem->setExpanded(true);
    testItem->setCheckState(0,Qt::Checked);

    documentModified();

    sort();
}

void MainWindow::showContextMenu(QPoint pt)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(pt);
    if(!item)
        return;

    QString type = item->data(0,Qt::UserRole).toString();

    if(type!="test" && type!="project")
        return;

    QMenu *menu = new QMenu(this);

    if(type=="project"){
        menu->addAction(tr("Add project"),this,SLOT(addProject()));
        menu->addAction(tr("Add test"),this,SLOT(addTest()));
        menu->addSeparator();
    }

    QAction *editAction = menu->addAction(tr("Edit"));
    editAction->setShortcut(tr("F3","edit"));
    QAction *renameAction = menu->addAction(tr("Rename"));
    renameAction->setShortcut(tr("F2","rename"));
    QAction *deleteAction = menu->addAction(tr("Delete"));
    deleteAction->setShortcut(Qt::Key_Delete);


    menu->move( ui->treeWidget->mapToGlobal(pt) );

    QAction *selectedAction = menu->exec();
    if(selectedAction){
        if(selectedAction==renameAction){
            QString name = QInputDialog::getText(this,tr("Rename"),tr("New name:"),QLineEdit::Normal,item->text(0));
            if(!name.isEmpty()){
                item->setText(0,name);
                documentModified();
                sort();
            }
        }
        if(selectedAction==deleteAction){
            delete item;
            documentModified();
        }
        if(selectedAction==editAction){
            pressEdit();
        }
    }
}

void MainWindow::clearStatus(QTreeWidgetItem *item)
{
    item->setText(1,QString());
    for(int i=0;i<item->childCount();i++){
        QTreeWidgetItem *child = item->child(i);
        clearStatus(child);
    }
}

void MainWindow::startTests()
{
    m_nbTest = m_passed = m_failed = m_skipped = 0;

    m_startAction->setDisabled(true);

    QTreeWidgetItem *root = ui->treeWidget->invisibleRootItem();

    root->setCheckState(0,Qt::Checked);
    clearStatus(root);
    startTests(root);

    if(m_nbTest==0){
        m_startAction->setEnabled(true);
        ui->statusLabel->setText(tr("No test"));
    }else{
        ui->statusLabel->setText(tr("Pocessing"));

        if(!m_mapProcess.isEmpty()){
            m_mapProcess.firstKey()->start();
        }
    }

    ui->detailsLabel->setText(QString("%1\n%2\n%3").arg(tr("%n passed","",m_passed)).arg(tr("%n failed","",m_failed)).arg(tr("%n skipped","",m_skipped)));
 }

void MainWindow::startTests(QTreeWidgetItem *item)
{
    if(item->checkState(0)==Qt::Unchecked){
        item->setText(1,tr("Skipped"));
        return;
    }else{
        item->setText(1,tr("Processing"));
    }

    int nbTest = m_nbTest;

    if(item->columnCount()>0){
        if(item->data(0,Qt::UserRole).toString()=="test"){
            m_nbTest++;
            item->setText(1,tr("Processing"));

            QString exec = item->data(0,Qt::UserRole+1).toString();

            if(!QFile::exists(exec)){
                item->setText(1,tr("File not found"));
                m_failed++;
            }else{

                QString argsString = item->data(0,Qt::UserRole+2).toString();
                QString output = item->text(0);
                if(output.isEmpty())
                    output = "unamed";
                output = m_outputDir.path() + QDir::separator() + output + "-output.xml";

                QString envString = item->data(0,Qt::UserRole+4).toString();

                QStringList arguments = argsString.split(" ",QString::SkipEmptyParts);
                arguments << "-xml" << "-o" << output;

                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.insert("PATH", env.value("Path") + ";" + envString );

                QProcess *process = new QProcess(this);
                connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finished(int,QProcess::ExitStatus)));
                process->setProcessEnvironment( env );
                process->setArguments( arguments );
                process->setProgram( exec );

                m_mapProcess.insert(process,item);
            }
        }
    }

    for(int i=0;i<item->childCount();i++){
        QTreeWidgetItem *child = item->child(i);
        startTests(child);
    }

    if(nbTest==m_nbTest){
        item->setText(1,tr("No test"));
    }
}

void MainWindow::finished(int /*errorCode*/, QProcess::ExitStatus status)
{
    QProcess *process = qobject_cast<QProcess*>(sender());

    if(process){
        if(m_mapProcess.contains(process)){
            QTreeWidgetItem *item = m_mapProcess.value(process);

            while(item->childCount()>0){
                item->removeChild(item->child(0));
            }

            if(status==QProcess::NormalExit){

                QString output = item->text(0);
                if(output.isEmpty())
                    output = "unamed";
                output = m_outputDir.path() + QDir::separator() + output + "-output.xml";

                XmlOutputParser parser;
                TestReport report = parser.parse(output);

                for(int i=0;i<report.testedFunction.size();i++){
                    TestFunctionReport r = report.testedFunction.at(i);
                    QTreeWidgetItem *child = new QTreeWidgetItem(item);
                    child->setData(0,Qt::UserRole, QString("function"));
                    child->setData(0,Qt::UserRole+1, r);
                    child->setText(0,r.name);
                    child->setText(1,r.incident.type);


                    child->setCheckState(0,Qt::Checked);
                    /*if(r.name=="initTestCase" || r.name=="cleanupTestCase")*/
                        child->setFlags(child->flags() ^ Qt::ItemIsUserCheckable);

                    if(r.incident.type.isEmpty()){
                        child->setIcon(0, this->style()->standardIcon( QStyle::SP_ArrowDown ));
                    }else if(r.incident.type=="pass"){
                        child->setIcon(0, this->style()->standardIcon( QStyle::SP_DialogApplyButton ));
                    }else{
                        child->setIcon(0, this->style()->standardIcon( QStyle::QStyle::SP_DialogCancelButton ));
                    }
                }

                if(report.passed()){
                    item->setText(1,tr("Passed"));
                    m_passed++;
                }else{
                    item->setText(1,tr("Failed"));
                    m_failed++;
                }

            }else{
                item->setText(1,tr("Failed to start"));
                m_failed++;
            }

            m_mapProcess.remove(process);
            process->deleteLater();
        }

        m_nbTest--;
    }

    if(m_nbTest==0){
        m_startAction->setEnabled(true);
        if(m_failed==0)
            ui->statusLabel->setText(tr("Passed"));
        else
            ui->statusLabel->setText(tr("Failed"));
    }

    ui->detailsLabel->setText(QString("%1\n%2\n%3").arg(tr("%n passed","",m_passed)).arg(tr("%n failed","",m_failed)).arg(tr("%n skipped","",m_skipped)));

    if(!m_mapProcess.isEmpty()){
        m_mapProcess.firstKey()->start();
    }

}

void MainWindow::saveItem(QXmlStreamWriter &xml, QTreeWidgetItem *item)
{
    if(!item) return;

    QString type = item->data(0,Qt::UserRole).toString();
    if(type!="project" && type!="test")
        return;

    xml.writeStartElement(type);

    if(type=="project"){
        xml.writeAttribute("name", item->text(0));
        xml.writeAttribute("disabled", STRING(item->checkState(0)==Qt::Unchecked));
        xml.writeAttribute("expanded", STRING(item->isExpanded()));
        QStringList env = item->data(0,Qt::UserRole+1).toString().split(";");
        foreach(QString e, env){
            xml.writeTextElement("env",e);
        }
    }else if(type=="test"){
        xml.writeTextElement("title", item->text(0));
        xml.writeTextElement("exec", item->data(0,Qt::UserRole+1).toString());
        xml.writeTextElement("arguments", item->data(0,Qt::UserRole+2).toString());
        QStringList env = item->data(0,Qt::UserRole+3).toString().split(";");
        foreach(QString e, env){
            xml.writeTextElement("env",e);
        }
    }

    for(int i=0;i<item->childCount();i++){
        saveItem(xml,item->child(i));
    }

    xml.writeEndElement();
}


bool MainWindow::saveXML()
{
    QFile output(m_currentFilepath);
    if(!output.open(QFile::WriteOnly))
        return false;

    QXmlStreamWriter stream(&output);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("QTestRunner");
    stream.writeAttribute("version", XML_VERSION);

    {
        QTreeWidgetItem *root = ui->treeWidget->invisibleRootItem();
        for(int i=0;i<root->childCount();i++){
            saveItem(stream,root->child(i));
        }
    }

    stream.writeEndElement();
    stream.writeEndDocument();


    m_modified = false;
    this->setWindowTitle(QString("%1 - %2").arg(tr("QTestRunner"),m_currentFilename));

    return true;
}


void MainWindow::parseXML(QString filepath)
{
    if(!QFile::exists(filepath)){
        QMessageBox::critical(this,tr("Error"),tr("The file '%1' doesn't exist").arg(filepath));
        return;
    }

    QFile input(filepath);
    if(!input.open(QFile::ReadOnly)){
        QMessageBox::critical(this,tr("Error"),tr("Can't open '%1'").arg(filepath));
        return;
    }

    m_currentFilepath.clear();

    QXmlStreamReader xml(&input);

    if(xml.readNextStartElement()){
        if(xml.name()=="QTestRunner"){
            if(xml.attributes().value("version").toString()!=QString(XML_VERSION)){
                qDebug() << "Invalid version";
                return;
            }
        }else{
            qDebug() << "Invalid document";
            return;
        }
    }

    QFileInfo info(filepath);
    m_currentFilepath = info.absoluteFilePath();
    m_currentFilename = info.fileName();

    while(xml.readNextStartElement()){
        if(xml.name() == "project"){
            readProject(xml,0);
        }else{
            xml.skipCurrentElement();
        }
    }

    this->setWindowTitle(QString("%1 - %2").arg(tr("QTestRunner"),m_currentFilename));
}

void MainWindow::readProject(QXmlStreamReader &xml, QTreeWidgetItem *item)
{
    QTreeWidgetItem *projectItem = createChildItem(item);
    projectItem->setIcon(0, m_projectIcon );
    projectItem->setData(0, Qt::UserRole, xml.name().toString());
    projectItem->setText(0, xml.attributes().value("name").toString());
    projectItem->setText(1,tr("Unknow"));
    projectItem->setFlags(projectItem->flags() | Qt::ItemIsTristate);

    bool expanded = true;
    bool disabled = false;

    if(xml.attributes().value("expanded").toString()=="false")
        expanded = false;
    if(xml.attributes().value("disabled").toString()=="true")
        disabled = true;

    projectItem->setExpanded(expanded);
    projectItem->setCheckState(0,disabled?Qt::Unchecked:Qt::Checked);

    while (xml.readNextStartElement()) {
        if (xml.name() == "project"){
            readProject(xml,projectItem);
        }else if (xml.name() == "test"){
            readTest(xml,projectItem);
        }else if (xml.name() == "env"){
            QString env = projectItem->data(0,Qt::UserRole+3).toString();
            QString nv = xml.readElementText();
            if(!nv.isEmpty())
                env += nv + ";";
            projectItem->setData(0,Qt::UserRole+1,env);
        }else
            xml.skipCurrentElement();
    }
}

void MainWindow::readTest(QXmlStreamReader &xml, QTreeWidgetItem *item)
{
    QTreeWidgetItem *testItem = createChildItem(item);
    testItem->setIcon(0, m_testIcon );
    testItem->setData(0, Qt::UserRole, xml.name().toString());
    testItem->setText(0,tr("Unamed"));
    testItem->setText(1,tr("Unknow"));
    testItem->setFlags(testItem->flags() ^ Qt::ItemIsTristate);

    bool expanded = true;
    bool disabled = false;

    if(xml.attributes().value("expanded").toString()=="false")
        expanded = false;
    if(xml.attributes().value("disabled").toString()=="true")
        disabled = true;

    testItem->setExpanded(expanded);
    testItem->setCheckState(0,disabled?Qt::Unchecked:Qt::Checked);

    while (xml.readNextStartElement()) {
       if (xml.name() == "title"){
            testItem->setText(0,xml.readElementText());
        }else if (xml.name() == "exec"){
            QString path = xml.readElementText();
            testItem->setData(0,Qt::UserRole+1,path);
        }else if (xml.name() == "arguments"){
          QString output = xml.readElementText();
          testItem->setData(0,Qt::UserRole+2,output);
        }else if (xml.name() == "env"){
            QString env = testItem->data(0,Qt::UserRole+3).toString();
            QString nv = xml.readElementText();
            if(!nv.isEmpty())
                env += nv + ";";
            testItem->setData(0,Qt::UserRole+3,env);
        }else
            xml.skipCurrentElement();
    }
}

QTreeWidgetItem* MainWindow::createChildItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *childItem;
    if(item){
        childItem = new TreeWidgetItem(item);
    }else{
        childItem = new TreeWidgetItem(ui->treeWidget);
    }
    return childItem;
}

void MainWindow::displayItemInfo()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.isEmpty())
        return;
    QTreeWidgetItem *item = selected.at(0);
    QListWidget *list = ui->infoListWidget;
    list->clear();

    if(item->data(0,Qt::UserRole).toString()=="project"){
        list->addItem(tr("Project:"));
        QStringList env = item->data(0,Qt::UserRole+1).toString().split(";",QString::SkipEmptyParts);
        if(!env.isEmpty())
            list->addItem(" " + tr("Environment:"));
        foreach(QString e, env){
            list->addItem("  " + e);
        }
    }else if(item->data(0,Qt::UserRole).toString()=="test"){
        list->addItem(tr("Test:"));
        list->addItem(" " + tr("Path:") + " " + item->data(0,Qt::UserRole+1).toString());
        list->addItem(" " + tr("Arguments:") + " " + item->data(0,Qt::UserRole+2).toString());
        QStringList env = item->data(0,Qt::UserRole+3).toString().split(";",QString::SkipEmptyParts);
        if(!env.isEmpty())
            list->addItem(" " + tr("Environment:"));
        foreach(QString e, env){
            list->addItem("  " + e);
        }
    }else if(item->data(0,Qt::UserRole).toString()=="function"){
        list->addItem(tr("Function:"));
        TestFunctionReport r = item->data(0,Qt::UserRole+1).value<TestFunctionReport>();
        list->addItem(" " + tr("Name:") + " " + r.name);
        list->addItem(" " + tr("Duration:") + " " + r.duration);
        if(!r.messages.isEmpty()){
            list->addItem(" " + tr("Messages:"));
            foreach(Message m, r.messages){
                list->addItem("  " + m.type + " " + m.description);
                if(!m.file.isEmpty())
                    list->addItem("   " + m.file + " " + QString::number(m.line));
            }
        }
        list->addItem(" " + tr("Incident:") + " " + r.incident.type + " " + r.incident.description);
        if(!r.incident.file.isEmpty())
            list->addItem("  " + r.incident.file + " " + QString::number(r.incident.line));
    }

}

bool MainWindow::pressNew()
{
    return pressClose();
}

void MainWindow::pressOpen()
{
    if(pressNew()){
        parseXML(QFileDialog::getOpenFileName(this,tr("Open"),QDir::homePath(),"XML (*.xml)"));
    }
}

bool MainWindow::pressSave()
{
    if(m_currentFilepath.isEmpty()){
        return pressSaveAs();
    }else{
        return saveXML();
    }
}

bool MainWindow::pressSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this,tr("Save as"),QDir::homePath(),"XML (*.xml)");
    if(path.isEmpty())
        return false;

    m_currentFilepath = path;

    return saveXML();
}

bool MainWindow::pressClose()
{
    if(saveBeforeClose()!=QMessageBox::Cancel){
        m_currentFilepath.clear();
        m_currentFilename.clear();
        ui->treeWidget->clear();
        m_modified = false;
        return true;
    }else{
        return false;
    }
}

void MainWindow::pressQuit()
{
    if(pressClose())
        this->close();
}

void MainWindow::pressEdit()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.size()!=1){
        QMessageBox::warning(this,tr("Warning"),tr("Select one item"));
        return;
    }
    QTreeWidgetItem *item = selected.at(0);
    if(item->data(0,Qt::UserRole).toString()=="project"){
        QDialog *edit = new QDialog(this);
        Ui::ProjectEdit *projectUI = new Ui::ProjectEdit;
        projectUI->setupUi(edit);
        projectUI->nameLineEdit->setText(item->text(0));
        projectUI->environmentTextEdit->setPlainText(item->data(0,Qt::UserRole+1).toString());
        if(edit->exec()){
            item->setText(0, projectUI->nameLineEdit->text() );
            item->setData(0,Qt::UserRole+2, projectUI->environmentTextEdit->toPlainText() );
            documentModified();
            sort();

        }
    }else if(item->data(0,Qt::UserRole).toString()=="test"){
        QDialog *edit = new QDialog(this);
        Ui::TestEdit *testUI = new Ui::TestEdit;
        testUI->setupUi(edit);
        testUI->nameLineEdit->setText(item->text(0));
        testUI->executableLineEdit->setText(item->data(0,Qt::UserRole+1).toString());
        testUI->argumentsLineEdit->setText(item->data(0,Qt::UserRole+2).toString());
        testUI->environmentTextEdit->setPlainText(item->data(0,Qt::UserRole+3).toString());
        if(edit->exec()){
            item->setText(0, testUI->nameLineEdit->text() );
            item->setData(0,Qt::UserRole+1, testUI->executableLineEdit->text() );
            item->setData(0,Qt::UserRole+2, testUI->argumentsLineEdit->text() );
            item->setData(0,Qt::UserRole+3, testUI->environmentTextEdit->toPlainText() );
            documentModified();
            sort();
        }
    }else{
        return;
    }
}

void MainWindow::pressRename()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.size()!=1){
        QMessageBox::warning(this,tr("Warning"),tr("Select one item"));
        return;
    }
    QTreeWidgetItem *item = selected.at(0);
    if(!item)
        return;
    QString name = QInputDialog::getText(this,tr("Rename"),tr("New name:"),QLineEdit::Normal,item->text(0));
    if(!name.isEmpty()){
        item->setText(0,name);
        documentModified();
        sort();
    }
}

void MainWindow::pressDelete()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.size()!=1){
        QMessageBox::warning(this,tr("Warning"),tr("Select one item"));
        return;
    }
    QTreeWidgetItem *item = selected.at(0);
    delete item;
}

QMessageBox::StandardButton MainWindow::saveBeforeClose()
{
    if(m_modified){
        QMessageBox::StandardButton button = QMessageBox::question(this,tr("Save"),tr("Do you want to save modified project?"),
                                                                   QMessageBox::StandardButtons( QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel), QMessageBox::Yes);
        if(button==QMessageBox::Yes){
            if(!pressSave())
                return QMessageBox::Cancel;
        }
        return button;
    }else{
        return QMessageBox::Yes;
    }
}
