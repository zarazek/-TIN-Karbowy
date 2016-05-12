#include "sortfilteremployeemodel.h"
#include "commongui.h"

SortFilterEmployeeModel::SortFilterEmployeeModel(QObject* parent) :
    QSortFilterProxyModel(parent),
    _showActive(true),
    _showInactive(true) { }

void SortFilterEmployeeModel::showActive(bool v)
{
    if (v != _showActive)
    {
        _showActive = v;
        invalidateFilter();
    }
}

void SortFilterEmployeeModel::showInactive(bool v)
{
    if (v != _showInactive)
    {
        _showInactive = v;
        invalidateFilter();
    }
}

bool SortFilterEmployeeModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, EMPLOYEE_COLUMN_ACTIVE, sourceParent);
    bool value = sourceModel()->data(index).toBool();
    return value ? _showActive : _showInactive;
}

