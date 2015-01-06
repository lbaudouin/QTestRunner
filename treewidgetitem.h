#ifndef TREEWIDGETITEM_H
#define TREEWIDGETITEM_H

#include <QTreeWidgetItem>

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget *parent) : QTreeWidgetItem(parent)  {}
    TreeWidgetItem(QTreeWidgetItem *parent) : QTreeWidgetItem(parent)  {}

    bool operator< (const QTreeWidgetItem &other) const
    {
        QString type = data(0,Qt::UserRole).toString();
        QString other_type = other.data(0,Qt::UserRole).toString();
        if( type=="project" && other_type!="project")
            return true;
        if( type!="project" && other_type=="project")
            return false;
        return text(0) < other.text(0);
    }
};

#endif // TREEWIDGETITEM_H
