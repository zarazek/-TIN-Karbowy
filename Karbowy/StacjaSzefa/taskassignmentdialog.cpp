#include "taskassignmentdialog.h"
#include "ui_taskassignmentdialog.h"
#include <assert.h>

TaskAssignmentDialog::TaskAssignmentDialog(std::set<std::string>& availableEmployees,
                                           std::set<std::string>& assignedEmployees,
                                           QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::TaskAssignmentDialog),
    _availableEmployees(availableEmployees),
    _assignedEmployees(assignedEmployees)
{
    _ui->setupUi(this);
    fillWidget(_ui->availableEmployeeSListWidget, _availableEmployees);
    fillWidget(_ui->assignedEmployeesListWidget, _assignedEmployees);
    connect(_ui->addEmployeeButton, &QPushButton::clicked, this, &TaskAssignmentDialog::addEmployees);
    connect(_ui->removeEmployeeButton, &QPushButton::clicked, this, &TaskAssignmentDialog::removeEmployees);
    connect(_ui->buttonBox, &QDialogButtonBox::accepted, this, &TaskAssignmentDialog::accept);
    connect(_ui->buttonBox, &QDialogButtonBox::rejected, this, &TaskAssignmentDialog::reject);
}

TaskAssignmentDialog::~TaskAssignmentDialog()
{
    delete _ui;
}

void TaskAssignmentDialog::addEmployees()
{
    moveItems(_ui->availableEmployeeSListWidget, _availableEmployees,
              _ui->assignedEmployeesListWidget, _assignedEmployees);
}

void TaskAssignmentDialog::removeEmployees()
{
    moveItems(_ui->assignedEmployeesListWidget, _assignedEmployees,
              _ui->availableEmployeeSListWidget, _availableEmployees);
}

void TaskAssignmentDialog::fillWidget(QListWidget* widget, const std::set<std::string>& set)
{
    widget->clear();
    for (const auto& employee : set)
    {
        widget->addItem(QString(employee.c_str()));
    }
    widget->sortItems();
}

void TaskAssignmentDialog::moveItems(QListWidget* fromWidget, std::set<std::string>& fromSet,
                                     QListWidget* toWidget, std::set<std::string>& toSet)
{
    auto items = fromWidget->selectedItems();
    for (auto item : items)
    {
        QString txt = item->text();
        fromWidget->removeItemWidget(item);
        delete item;
        toWidget->addItem(txt);
        std::string employee = txt.toStdString();
        size_t numOfRemoved = fromSet.erase(employee);
        assert(numOfRemoved == 1);
        auto res = toSet.insert(employee);
        assert(res.second);
    }
    toWidget->sortItems();
    fromWidget->repaint();
    toWidget->repaint();
}
