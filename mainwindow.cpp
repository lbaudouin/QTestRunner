#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    //Connect menu actions
    connect(ui->action_New,SIGNAL(triggered()),this,SLOT(pressNew()));
    connect(ui->action_Open,SIGNAL(triggered()),this,SLOT(pressOpen()));
    connect(ui->action_Save,SIGNAL(triggered()),this,SLOT(pressSave()));
    connect(ui->actionSave_As,SIGNAL(triggered()),this,SLOT(pressSaveAs()));
    connect(ui->action_Close,SIGNAL(triggered()),this,SLOT(pressClose()));
    connect(ui->action_Quit,SIGNAL(triggered()),this,SLOT(pressQuit()));

    connect(ui->action_Rename,SIGNAL(triggered()),this,SLOT(pressRename()));
    connect(ui->action_Delete,SIGNAL(triggered()),this,SLOT(pressDelete()));

    connect(ui->treeWidget,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));

    //Test
    parseXML("test.xml");
}

MainWindow::~MainWindow()
{
    QSettings settings(qAppName(),qAppName());
    settings.setValue("ui/geometry",this->saveGeometry());
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
    QTreeWidgetItem *projectItem = createChildItem(parentItem);
    projectItem->setIcon(0, m_projectIcon );
    projectItem->setData(0, Qt::UserRole, QString("project") );
    projectItem->setText(0, "<empty_name>");
    projectItem->setText(1,tr("New project"));
    projectItem->setFlags(projectItem->flags() | Qt::ItemIsTristate);
    projectItem->setExpanded(true);
    projectItem->setCheckState(0,Qt::Checked);

    documentModified();
}

void MainWindow::addTest()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.size()!=1 || selected.at(0)->data(0,Qt::UserRole).toString()!="project"){
        QMessageBox::warning(this,tr("Warning"),tr("Select one project"));
        return;
    }
    QTreeWidgetItem *testItem = createChildItem(selected.at(0));
    testItem->setIcon(0, m_testIcon );
    testItem->setData(0, Qt::UserRole, QString("test"));
    testItem->setText(0,tr("<empty_name>"));
    testItem->setText(1,tr("New test"));
    testItem->setFlags(testItem->flags() ^ Qt::ItemIsTristate);
    testItem->setExpanded(true);
    testItem->setCheckState(0,Qt::Checked);

    documentModified();
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
            }
        }
        if(selectedAction==deleteAction){
            delete item;
            documentModified();
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
    }

    ui->detailsLabel->setText(QString("%1, %2, %3").arg(tr("%n passed","",m_passed)).arg(tr("%n failed","",m_failed)).arg(tr("%n skipped","",m_skipped)));
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

                QString output = item->data(0,Qt::UserRole+2).toString();
                output.prepend("output/");
                QString path = item->data(0,Qt::UserRole+3).toString();
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.insert("PATH", env.value("Path") + ";" + path );

                QStringList arguments;
                arguments << "-xml" << "-o" << output;

                QProcess *process = new QProcess(this);
                process->setProcessEnvironment(env);
                m_mapProcess.insert(process,item);
                process->start(exec, arguments);

                connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finished(int,QProcess::ExitStatus)));
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

    if(!process)
        return;

    if(m_mapProcess.contains(process)){
        QTreeWidgetItem *item = m_mapProcess.value(process);

        /*while(item->childCount()>0){
            item->removeChild(item->child(0));
        }*/

        if(status==QProcess::NormalExit){

            QString output = item->data(0,Qt::UserRole+2).toString();
            output.prepend("output/");

            XmlOutputParser parser;
            TestReport report = parser.parse(output);
            //qDebug() << report;

            for(int i=0;i<report.testedFunction.size();i++){
                /*struct TestFunctionReport{
                    QString name;
                    QString type;
                    QString file;
                    int line;
                    QString description;
                    double duration;
                };*/
                TestFunctionReport r = report.testedFunction.at(i);
                QTreeWidgetItem *child = new QTreeWidgetItem(item);
                child->setData(0,Qt::UserRole,QString("function"));
                child->setText(0,r.name);
                child->setText(1,r.type);
                child->setCheckState(0,Qt::Checked);
                /*if(r.name=="initTestCase" || r.name=="cleanupTestCase")*/
                    child->setFlags(child->flags() ^ Qt::ItemIsUserCheckable);

                if(r.type.isEmpty()){
                    child->setIcon(0, this->style()->standardIcon( QStyle::SP_ArrowDown ));
                }else if(r.type=="pass"){
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

    if(m_nbTest==0){
        m_startAction->setEnabled(true);
        if(m_failed==0)
            ui->statusLabel->setText(tr("Passed"));
        else
            ui->statusLabel->setText(tr("Failed"));
    }

    ui->detailsLabel->setText(QString("%1, %2, %3").arg(tr("%n passed","",m_passed)).arg(tr("%n failed","",m_failed)).arg(tr("%n skipped","",m_skipped)));

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
        xml.writeTextElement("output_path", item->data(0,Qt::UserRole+1).toString());
    }else if(type=="test"){
        xml.writeTextElement("title", item->text(0));
        xml.writeTextElement("exec", item->data(0,Qt::UserRole+1).toString());
        xml.writeTextElement("output", item->data(0,Qt::UserRole+2).toString());
        xml.writeTextElement("env", item->data(0,Qt::UserRole+3).toString());
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
        if (xml.name() == "output_path"){
            projectItem->setData(0,Qt::UserRole+1, xml.readElementText() );
        }else if (xml.name() == "project"){
            readProject(xml,projectItem);
        }else if (xml.name() == "test"){
            readTest(xml,projectItem);
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
        }else if (xml.name() == "output"){
           QString output = xml.readElementText();
           testItem->setData(0,Qt::UserRole+2,output);
        }else if (xml.name() == "env"){
          QString output = xml.readElementText();
          testItem->setData(0,Qt::UserRole+3,output);
        }else
            xml.skipCurrentElement();
    }
}

QTreeWidgetItem* MainWindow::createChildItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *childItem;
    if(item){
        childItem = new QTreeWidgetItem(item);
    }else{
        childItem = new QTreeWidgetItem(ui->treeWidget);
    }
    return childItem;
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
    if(!name.isEmpty())
        item->setText(0,name);
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
