#ifndef CHECKBOXDELEGATE_H
#define CHECKBOXDELEGATE_H

#include <QItemDelegate>

class CheckBoxDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    CheckBoxDelegate(QAbstractItemView* parentView = NULL, QObject *parent = NULL);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    ~CheckBoxDelegate();

private:
    QAbstractItemView* parentView;
};

#endif // CHECKBOXDELEGATE_H
