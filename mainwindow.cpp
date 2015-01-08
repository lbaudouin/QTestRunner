#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_projectIcon =  QIcon(":images/project");
    m_testIcon =  QIcon(":images/test");
    m_waitIcon =  QIcon(":images/wait");
    m_currentFilepath.clear();
    m_currentFilename.clear();
    m_modified = false;
    m_busy = false;

    {
        QStringList labels;
        labels << tr("Name") << tr("Status");
        ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        ui->treeWidget->setHeaderLabels(labels);
    }
    {
        QStringList labels;
        labels << tr("Info") << tr("Value");
        ui->infoTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        ui->infoTreeWidget->setHeaderLabels(labels);
    }

    m_addProjectAction = ui->mainToolBar->addAction( m_projectIcon , tr("Add project"), this, SLOT(addProject()) );
    m_addTestAction = ui->mainToolBar->addAction( m_testIcon , tr("Add test"), this, SLOT(addTest()) );
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
        if(selected.size()!=1 || selected.at(0)->data(0,TYPE_ROLE).toString()!="project"){
            QMessageBox::warning(this,tr("Warning"),tr("Select one project"));
            return;
        }
        parentItem = selected.at(0);
    }

    ProjectEditDialog *editDialog = new ProjectEditDialog(this);
    editDialog->setWindowTitle( tr("Add project"));
    editDialog->setName( tr("New project") );
    if(editDialog->exec()){
        QTreeWidgetItem *projectItem = createChildItem(parentItem);
        projectItem->setData(0, TYPE_ROLE, QString("project") );
        projectItem->setIcon(0, m_projectIcon );
        projectItem->setText(0, editDialog->name() );
        projectItem->setData(0, ENV_ROLE, editDialog->environment() );
        projectItem->setText(1,tr("New project"));
        projectItem->setFlags(projectItem->flags() | Qt::ItemIsTristate);
        projectItem->setExpanded(true);
        projectItem->setCheckState(0,Qt::Checked);
        documentModified();
        sort();
    }
}

void MainWindow::addTest()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.size()!=1 || selected.at(0)->data(0,TYPE_ROLE).toString()!="project"){
        QMessageBox::warning(this,tr("Warning"),tr("Select one project"));
        return;
    }

    TestEditDialog *editDialog = new TestEditDialog(this);
    editDialog->setWindowTitle( tr("Add test"));
    editDialog->setName( tr("New test") );
    if(editDialog->exec()){
        QTreeWidgetItem *testItem = createChildItem(selected.at(0));
        testItem->setData(0, TYPE_ROLE, QString("test"));
        testItem->setIcon(0, m_testIcon );
        testItem->setText(0, editDialog->name() );
        testItem->setData(0, EXEC_ROLE, editDialog->executable() );
        testItem->setData(0, ARGS_ROLE, editDialog->arguments() );
        testItem->setData(0, ENV_ROLE, editDialog->environment() );
        testItem->setText(1, tr("New test"));
        testItem->setFlags(testItem->flags() ^ Qt::ItemIsTristate);
        testItem->setExpanded(true);
        testItem->setCheckState(0,Qt::Checked);
        documentModified();
        sort();
    }
}

void MainWindow::showContextMenu(QPoint pt)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(pt);
    if(!item)
        return;

    QString type = item->data(0,TYPE_ROLE).toString();

    if(type!="test" && type!="project")
        return;

    QMenu *menu = new QMenu(this);
    menu->setDisabled(m_busy);

    menu->addAction(tr("Run"),this,SLOT(runSingleTest()))->setEnabled(false);
    menu->addSeparator();

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
    item->setIcon(1, QIcon() );

    if(item->data(0,TYPE_ROLE).toString()=="test"){
        while(item->childCount()>0){
            item->removeChild(item->child(0));
        }
    }

    for(int i=0;i<item->childCount();i++){
        QTreeWidgetItem *child = item->child(i);
        clearStatus(child);
    }

}

void MainWindow::startTests()
{
    if(m_busy){
        foreach(QProcess* p, m_listProcess) {
            p->close();
            p->deleteLater();
        }
        m_listProcess.clear();
        m_mapProcess.clear();

        setBusy( false );
    }else{
        setBusy( true );
        m_nbTest = m_passed = m_failed = m_skipped = 0;

        QTreeWidgetItem *root = ui->treeWidget->invisibleRootItem();

        root->setCheckState(0,Qt::Checked);
        clearStatus(root);
        startTests(root);

        if(m_nbTest==0){
            m_startAction->setEnabled(true);
            ui->statusLabel->setText(tr("No test"));
            setBusy( false );
        }else{
            ui->statusLabel->setText(tr("Processing"));

            if(!m_listProcess.isEmpty()){
                QTreeWidgetItem *item = m_mapProcess.value(m_listProcess.first());
                while(item && item!=root){
                    item->setText(1,tr("Processing"));
                    item = item->parent();
                }
                m_listProcess.first()->start();
            }
        }

        ui->detailsLabel->setText(QString("%1\n%2\n%3").arg(tr("%n passed","",m_passed)).arg(tr("%n failed","",m_failed)).arg(tr("%n skipped","",m_skipped)));
    }
 }

void MainWindow::startTests(QTreeWidgetItem *item)
{
    if(item->checkState(0)==Qt::Unchecked){
        setItemStatus(item,STATUS_SKIP);
        return;
    }else{
        setItemStatus(item,STATUS_WAIT);
    }

    int nbTest = m_nbTest;
    int nbTestFail = m_failed;

    QString type = item->data(0,TYPE_ROLE).toString();
    if(type=="test"){
        m_nbTest++;
        QString exec = item->data(0,EXEC_ROLE).toString();

        if(!QFile::exists(exec)){
            setItemStatus(item,STATUS_FAIL,tr("File not found"));
            m_failed++;
        }else{

            QString argsString = item->data(0,ARGS_ROLE).toString();
            QString output = item->text(0);
            if(output.isEmpty())
                output = "unamed";
            output = m_outputDir.path() + QDir::separator() + output + "-output.xml";

            QString envString;

            QTreeWidgetItem *i = item;
            while(i && i!=ui->treeWidget->invisibleRootItem()){
                QString e = i->data(0,ENV_ROLE).toString();
                if(!e.isEmpty())
                    envString += e + ";";
                i = i->parent();
            }

            QStringList arguments = argsString.split(" ",QString::SkipEmptyParts);
            arguments << "-xml" << "-o" << output;

            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert("PATH", env.value("Path") + ";" + envString );

            QProcess *process = new QProcess(this);
            connect(process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(finished(int,QProcess::ExitStatus)));
            process->setProcessEnvironment( env );
            process->setArguments( arguments );
            process->setProgram( exec );

            //qDebug() << exec << arguments << envString;

            m_listProcess << process;
            m_mapProcess.insert(process,item);
        }
    }

    for(int i=0;i<item->childCount();i++){
        QTreeWidgetItem *child = item->child(i);
        startTests(child);
    }

    if(nbTest==m_nbTest){
        setItemStatus(item,STATUS_SKIP,tr("No test"));
    }
    if(nbTestFail<m_failed && type=="project"){
        setItemStatus(item,STATUS_FAIL);
    }
}

void MainWindow::finished(int /*errorCode*/, QProcess::ExitStatus status)
{
    QProcess *process = qobject_cast<QProcess*>(sender());

    if(process){
        if(m_listProcess.contains(process)){
            QTreeWidgetItem *item = m_mapProcess.value(process);
            if(item){
                item->setExpanded(false);
                /*while(item->childCount()>0){
                    item->removeChild(item->child(0));
                }*/
                //item->setExpanded(false);

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
                        child->setData(0,TYPE_ROLE, QString("function"));
                        child->setData(0,REPORT_ROLE, QVariant::fromValue(r));
                        child->setText(0,r.name);

                        child->setCheckState(0,Qt::Checked);
                        /*if(r.name=="initTestCase" || r.name=="cleanupTestCase")*/
                            child->setFlags(child->flags() ^ Qt::ItemIsUserCheckable);

                        if(r.incident.type.isEmpty()){
                            setItemStatus(child,STATUS_SKIP,"");
                        }else if(r.incident.type=="pass"){
                            setItemStatus(child,STATUS_PASS);
                        }else if(r.incident.type=="fail"){
                            setItemStatus(child,STATUS_FAIL);
                        }else{
                            setItemStatus(child,STATUS_UNK,"\""+r.incident.type+"\"");
                        }
                    }

                    if(report.passed()){
                        m_passed++;
                    }else{
                        m_failed++;
                    }

                }else{ // < if(status!=QProcess::NormalExit)
                    setItemStatus(item,STATUS_FAIL,tr("Failed to start"));
                    m_failed++;
                }

                m_listProcess.removeAll(process);
                m_mapProcess.remove(process);
                process->deleteLater();


                //Check parent item
                QTreeWidgetItem *i = item;
                while(i && i!=ui->treeWidget->invisibleRootItem()){
                    bool isFinished = true;
                    bool isPassed = true;

                    for(int k=0;k<i->childCount();k++){
                        int status = i->child(k)->data(0,STATUS_ROLE).toInt();
                        if( status == STATUS_WAIT)
                            isFinished = false;
                        if( status == STATUS_FAIL || status == STATUS_UNK )
                            isPassed = false;
                    }

                    if(isFinished){
                        if(isPassed){
                            setItemStatus(i,STATUS_PASS);
                        }else{
                            setItemStatus(i,STATUS_FAIL);
                        }
                    }else{
                        setItemStatus(i,STATUS_WAIT);
                    }
                    i = i->parent();
                }
            }
        }
    }

    if(m_nbTest==m_failed+m_passed){
        if(m_failed==0)
            ui->statusLabel->setText(tr("Passed"));
        else
            ui->statusLabel->setText(tr("Failed"));
        setBusy(false);
    }

    ui->detailsLabel->setText(QString("%1\n%2\n%3").arg(tr("%n passed","",m_passed)).arg(tr("%n failed","",m_failed)).arg(tr("%n skipped","",m_skipped)));

    if(!m_listProcess.isEmpty()){
        QTreeWidgetItem *item = m_mapProcess.value(m_listProcess.first());
        while(item && item!=ui->treeWidget->invisibleRootItem()){
            item->setText(1,tr("Processing"));
            item = item->parent();
        }
        m_listProcess.first()->start();
    }

}

void MainWindow::setItemStatus(QTreeWidgetItem *item, int status)
{
    QString text;
    switch(status){
    case STATUS_FAIL: text = tr("Failed"); break;
    case STATUS_PASS: text = tr("Passed"); break;
    case STATUS_SKIP: text = tr("Skipped"); break;
    case STATUS_UNK:  text = tr("Unknow"); break;
    case STATUS_WAIT: text = tr("Wait"); break;
    case STATUS_BUSY: text = tr("Processing"); break;
    default: break;
    }
    setItemStatus(item,status,text);
}

void MainWindow::setItemStatus(QTreeWidgetItem *item, int status, QString text)
{
    item->setText(1,text);
    switch(status){
    case STATUS_FAIL: item->setIcon(1, this->style()->standardIcon( QStyle::QStyle::SP_DialogCancelButton )); break;
    case STATUS_PASS: item->setIcon(1, this->style()->standardIcon( QStyle::QStyle::SP_DialogApplyButton )); break;
    case STATUS_SKIP: item->setIcon(1, this->style()->standardIcon( QStyle::QStyle::SP_ArrowDown )); break;
    case STATUS_UNK : item->setIcon(1, this->style()->standardIcon( QStyle::QStyle::SP_MessageBoxQuestion )); break;
    case STATUS_WAIT: item->setIcon(1, m_waitIcon ); break;
    case STATUS_BUSY: item->setIcon(1, this->style()->standardIcon( QStyle::QStyle::SP_BrowserReload )); break;
    default: item->setIcon(1, this->style()->standardIcon(QStyle::SP_DialogHelpButton) ); break;
    }
    item->setData(0,STATUS_ROLE,status);
}

void MainWindow::saveItem(QXmlStreamWriter &xml, QTreeWidgetItem *item)
{
    if(!item) return;

    QString type = item->data(0,TYPE_ROLE).toString();
    if(type!="project" && type!="test")
        return;

    xml.writeStartElement(type);

    if(type=="project"){
        xml.writeAttribute("name", item->text(0));
        xml.writeAttribute("disabled", STRING(item->checkState(0)==Qt::Unchecked));
        xml.writeAttribute("expanded", STRING(item->isExpanded()));
        QStringList env = item->data(0,ENV_ROLE).toString().split(";");
        foreach(QString e, env){
            xml.writeTextElement("env",e);
        }
    }else if(type=="test"){
        xml.writeTextElement("title", item->text(0));
        xml.writeTextElement("exec", item->data(0,EXEC_ROLE).toString());
        xml.writeTextElement("arguments", item->data(0,ARGS_ROLE).toString());
        QStringList env = item->data(0,ENV_ROLE).toString().split(";");
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
    projectItem->setData(0, TYPE_ROLE, xml.name().toString());
    projectItem->setText(0, xml.attributes().value("name").toString());
    projectItem->setFlags(projectItem->flags() | Qt::ItemIsTristate);
    setItemStatus(projectItem,STATUS_UNK);

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
            QString env = projectItem->data(0,ENV_ROLE).toString();
            QString nv = xml.readElementText();
            if(!nv.isEmpty())
                env += nv + ";";
            projectItem->setData(0,ENV_ROLE,env);
        }else
            xml.skipCurrentElement();
    }
}

void MainWindow::readTest(QXmlStreamReader &xml, QTreeWidgetItem *item)
{
    QTreeWidgetItem *testItem = createChildItem(item);
    testItem->setIcon(0, m_testIcon );
    testItem->setData(0, TYPE_ROLE, xml.name().toString());
    testItem->setText(0,tr("Unamed"));
    testItem->setFlags(testItem->flags() ^ Qt::ItemIsTristate);
    setItemStatus(testItem,STATUS_UNK);

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
            QString exec = xml.readElementText();
            testItem->setData(0,EXEC_ROLE,exec);
        }else if (xml.name() == "arguments"){
          QString output = xml.readElementText();
          testItem->setData(0,ARGS_ROLE,output);
        }else if (xml.name() == "env"){
            QString env = testItem->data(0,ENV_ROLE).toString();
            QString nv = xml.readElementText();
            if(!nv.isEmpty())
                env += nv + ";";
            testItem->setData(0,ENV_ROLE,env);
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
    ui->infoTreeWidget->clear();
    QList<QTreeWidgetItem*> toCollapse;

    QString type = item->data(0,TYPE_ROLE).toString();
    if(type=="project"){
        QTreeWidgetItem *i = new QTreeWidgetItem(ui->infoTreeWidget);
        i->setText(0,tr("Project:"));
        QTreeWidgetItem *i1 = new QTreeWidgetItem(i);
        i1->setText(0,tr("Name:"));
        i1->setText(1,item->text(0));

        QStringList env = item->data(0,ENV_ROLE).toString().split(";",QString::SkipEmptyParts);
        if(!env.isEmpty()){
            QTreeWidgetItem *i2 = new QTreeWidgetItem(i);
            i2->setText(0,tr("Environment:"));
            foreach(QString e, env){
                QTreeWidgetItem *i3 = new QTreeWidgetItem(i2);
                i3->setText(1,e);
            }
        }
    }else if(type=="test"){
        QTreeWidgetItem *i = new QTreeWidgetItem(ui->infoTreeWidget);
        i->setText(0,tr("Test:"));
        QTreeWidgetItem *i1 = new QTreeWidgetItem(i);
        i1->setText(0,tr("Name:"));
        i1->setText(1,item->text(0));
        QTreeWidgetItem *i2 = new QTreeWidgetItem(i);
        i2->setText(0,tr("Executable:"));
        i2->setText(1,item->data(0,EXEC_ROLE).toString());
        QTreeWidgetItem *i3 = new QTreeWidgetItem(i);
        i3->setText(0,tr("Arguments:"));
        i3->setText(1,item->data(0,ARGS_ROLE).toString());

        QStringList env = item->data(0,ENV_ROLE).toString().split(";",QString::SkipEmptyParts);
        if(!env.isEmpty()){
            QTreeWidgetItem *i4 = new QTreeWidgetItem(i);
            i4->setText(0,tr("Environment:"));
            foreach(QString e, env){
                QTreeWidgetItem *i5 = new QTreeWidgetItem(i4);
                i5->setText(1,e);
            }
        }
    }else if(type=="function"){
        QTreeWidgetItem *i = new QTreeWidgetItem(ui->infoTreeWidget);
        i->setText(0,tr("Function:"));

        TestFunctionReport r = item->data(0,REPORT_ROLE).value<TestFunctionReport>();
        QTreeWidgetItem *i1 = new QTreeWidgetItem(i);
        i1->setText(0,tr("Name:"));
        i1->setText(1,r.name);
        QTreeWidgetItem *i2 = new QTreeWidgetItem(i);
        i2->setText(0,tr("Duration:"));
        i2->setText(1,QString::number(r.duration) + "ms");
        QTreeWidgetItem *i3 = new QTreeWidgetItem(i);
        i3->setText(0,tr("Messages:"));
        i3->setText(1, tr("%n messages","",r.messages.size()));
        foreach(Message m, r.messages){
            QTreeWidgetItem *i4 = new QTreeWidgetItem(i3);
            i4->setText(0,m.type);
            i4->setText(1,m.description);
            if(!m.file.isEmpty()){
                QTreeWidgetItem *i5 = new QTreeWidgetItem(i4);
                i5->setText(0,tr("file"));
                i5->setText(1,m.file);
                QTreeWidgetItem *i5b = new QTreeWidgetItem(i4);
                i5b->setText(0,tr("line"));
                i5b->setText(1,QString::number(m.line));
            }
            toCollapse << i4;
        }
        QTreeWidgetItem *i6 = new QTreeWidgetItem(i);
        i6->setText(0,tr("Incident:"));
        QTreeWidgetItem *i7 = new QTreeWidgetItem(i6);
        i7->setText(0,r.incident.type);
        i7->setText(1,r.incident.description);
        if(!r.incident.file.isEmpty()){
            QTreeWidgetItem *i8 = new QTreeWidgetItem(i7);
            i8->setText(0,tr("file"));
            i8->setText(1,r.incident.file);
            QTreeWidgetItem *i8b = new QTreeWidgetItem(i7);
            i8b->setText(0,tr("line"));
            i8b->setText(1,QString::number(r.incident.line));
        }
        toCollapse << i7;
    }
    ui->infoTreeWidget->expandAll();
    foreach(QTreeWidgetItem *it, toCollapse) {
        it->setExpanded(false);
    }

}

bool MainWindow::pressNew()
{
    return pressClose();
}

void MainWindow::pressOpen()
{
    if(pressNew()){
        QString filepath = QFileDialog::getOpenFileName(this,tr("Open"),QDir::homePath(),"XML (*.xml)");
        if(!filepath.isEmpty())
            parseXML(filepath);
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
    if(item->data(0,TYPE_ROLE).toString()=="project"){
        ProjectEditDialog *editDialog = new ProjectEditDialog(this);
        editDialog->setName( item->text(0) );
        editDialog->setEnvironment( item->data(0,ENV_ROLE).toString() );
        if(editDialog->exec()){
            item->setText(0, editDialog->name() );
            item->setData(0, ENV_ROLE, editDialog->environment() );
            documentModified();
            sort();
        }
    }else if(item->data(0,TYPE_ROLE).toString()=="test"){
        TestEditDialog *editDialog = new TestEditDialog(this);
        editDialog->setName( item->text(0) );
        editDialog->setExecutable( item->data(0,EXEC_ROLE).toString() );
        editDialog->setArguments( item->data(0,ARGS_ROLE).toString() );
        editDialog->setEnvironment( item->data(0,ENV_ROLE).toString() );
        if(editDialog->exec()){
            item->setText(0, editDialog->name() );
            item->setData(0, EXEC_ROLE, editDialog->executable() );
            item->setData(0, ARGS_ROLE, editDialog->arguments() );
            item->setData(0, ENV_ROLE, editDialog->environment() );
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

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;

    ui->action_Edit->setDisabled( m_busy );
    ui->action_Rename->setDisabled( m_busy );
    ui->action_Delete->setDisabled( m_busy );
    m_addProjectAction->setDisabled( m_busy );
    m_addTestAction->setDisabled( m_busy );

    if(m_busy){
        m_startAction->setIcon( this->style()->standardIcon( QStyle::SP_MediaStop ) );
        m_startAction->setText( tr("Stop") );
        this->setCursor( Qt::WaitCursor );
    }else{
        m_startAction->setIcon( this->style()->standardIcon( QStyle::SP_ArrowRight ) );
        m_startAction->setText( tr("Start") );
        this->setCursor( Qt::ArrowCursor );
    }
}

void MainWindow::runSingleTest()
{
    QList<QTreeWidgetItem*> selected = ui->treeWidget->selectedItems();
    if(selected.size()!=1 || selected.at(0)->data(0,TYPE_ROLE).toString()=="function"){
        QMessageBox::warning(this,tr("Warning"),tr("Select one test or one project"));
        return;
    }
    setBusy(true);
    QTreeWidgetItem *item = selected.at(0);
    item->setCheckState(0,Qt::Checked);
    startTests(item);
}

void MainWindow::runTests()
{
    setBusy(true);
    startTests(ui->treeWidget->invisibleRootItem());
}
