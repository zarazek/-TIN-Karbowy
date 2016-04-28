#include "checkboxdelegate.h"
#include <QCheckBox>
#include <QPainter>

CheckBoxDelegate::CheckBoxDelegate(QAbstractItemView* parentView, QObject *parent)
    : QItemDelegate(parent), parentView(parentView)
{

}

CheckBoxDelegate::~CheckBoxDelegate()
{

}

QWidget *CheckBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QCheckBox *editor = new QCheckBox(parent);
  editor->setTristate(false);
    return editor;
}

void CheckBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    bool value = index.model()->data(index, Qt::EditRole).toBool();

  QCheckBox *locEdit = static_cast<QCheckBox*>(editor);
  locEdit->setChecked(value);
}

void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QCheckBox *locEdit = static_cast<QCheckBox*>(editor);
  bool value = locEdit->isChecked();

  model->setData(index, value, Qt::EditRole);
}

void CheckBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    editor->setGeometry(option.rect);
}

void CheckBoxDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
    QPalette::ColorGroup cg;
    if (option.state & QStyle::State_Enabled) {
        cg = (option.state & QStyle::State_Active) ? QPalette::Normal : QPalette::Inactive;
    }
    else
        cg = QPalette::Disabled;

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.color(cg, QPalette::Highlight));

    //if (! (parentView->editTriggers() > QAbstractItemView::NoEditTriggers && option.state & QStyle::State_Selected) )
        drawCheck(painter, option, option.rect, index.data().toBool() ? Qt::Checked : Qt::Unchecked);

    drawFocus(painter, option, option.rect);
}

