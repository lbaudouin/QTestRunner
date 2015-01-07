#ifndef TESTEDITDIALOG_H
#define TESTEDITDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include "ui_testedit.h"

namespace Ui {
class TestEdit;
}

class TestEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TestEditDialog(QWidget *parent = 0) : QDialog(parent), ui(new Ui::TestEdit)
    {
        ui->setupUi(this);
        connect(ui->openFilePathButtons,SIGNAL(clicked()),this,SLOT(openFilePath()));
    }
    void setName(QString name) { ui->nameLineEdit->setText(name); }
    QString name() const { return ui->nameLineEdit->text(); }
    void setExecutable(QString executable) { ui->executableLineEdit->setText(executable); }
    QString executable() const { return ui->executableLineEdit->text(); }
    void setArguments(QString arguments) { ui->argumentsLineEdit->setText(arguments); }
    QString arguments() const { return ui->argumentsLineEdit->text(); }
    void setEnvironment(QString environment) { ui->environmentTextEdit->setPlainText(environment); }
    QString environment() const { return ui->environmentTextEdit->toPlainText(); }

private:
    Ui::TestEdit *ui;

public slots:
    void openFilePath(){
        QString filepath = QFileDialog::getOpenFileName(this,tr("Executable"),QDir::homePath());
        if(!filepath.isEmpty())
            ui->executableLineEdit->setText(filepath);
    }

};

#endif // TESTEDITDIALOG_H
