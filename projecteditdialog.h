#ifndef PROJECTEDITDIALOG_H
#define PROJECTEDITDIALOG_H

#include <QDialog>
#include "ui_projectedit.h"

namespace Ui {
class ProjectEdit;
}

class ProjectEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectEditDialog(QWidget *parent = 0) : QDialog(parent), ui(new Ui::ProjectEdit)
    {
        ui->setupUi(this);
    }
    void setName(QString name) { ui->nameLineEdit->setText(name); }
    QString name() const { return ui->nameLineEdit->text(); }
    void setEnvironment(QString environment) { ui->environmentTextEdit->setPlainText(environment); }
    QString environment() const { return ui->environmentTextEdit->toPlainText(); }

private:
    Ui::ProjectEdit *ui;
};

#endif // PROJECTEDITDIALOG_H
