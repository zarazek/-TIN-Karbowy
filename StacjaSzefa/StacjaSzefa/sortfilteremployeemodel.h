#ifndef SORTFILTEREMPLOYEEMODEL_H
#define SORTFILTEREMPLOYEEMODEL_H

#include <QSortFilterProxyModel>

class SortFilterEmployeeModel : public QSortFilterProxyModel
{
public:
    SortFilterEmployeeModel(QObject* parent = nullptr);

    void showActive(bool v);
    void showInactive(bool v);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
private:
    bool _showActive;
    bool _showInactive;
};

#endif // SORTFILTEREMPLOYEEMODEL_H
