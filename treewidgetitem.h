#ifndef TREEWIDGETITEM_H
#define TREEWIDGETITEM_H

#include <QTreeWidgetItem>

#define TYPE_ROLE Qt::UserRole
#define NAME_ROLE Qt::UserRole+1
#define ENV_ROLE  Qt::UserRole+2
#define EXEC_ROLE Qt::UserRole+3
#define ARGS_ROLE Qt::UserRole+4
#define REPORT_ROLE Qt::UserRole+5
#define STATUS_ROLE Qt::UserRole+10

#define STATUS_FAIL 0
#define STATUS_PASS 1
#define STATUS_SKIP 2
#define STATUS_UNK  3
#define STATUS_WAIT 4
#define STATUS_BUSY 5

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget *parent) : QTreeWidgetItem(parent)  {}
    TreeWidgetItem(QTreeWidgetItem *parent) : QTreeWidgetItem(parent)  {}

    bool operator< (const QTreeWidgetItem &other) const
    {
        QString type = data(0,TYPE_ROLE).toString();
        QString other_type = other.data(0,TYPE_ROLE).toString();
        if( type=="project" && other_type!="project")
            return true;
        if( type!="project" && other_type=="project")
            return false;
        return text(0) < other.text(0);
    }
};

#endif // TREEWIDGETITEM_H
