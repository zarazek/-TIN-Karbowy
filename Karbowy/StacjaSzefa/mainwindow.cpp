#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "commongui.h"
#include "sortfilteremployeemodel.h"
#include "employeetablemodel.h"
#include "taskstablemodel.h"
#include "taskassignmentdialog.h"
#include "predefinedqueries.h"
#include <QContextMenuEvent>
#include <QMenu>
#include <assert.h>

#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    SortFilterEmployeeModel *filteredEmployeesModel = new SortFilterEmployeeModel(this);
    EmployeeTableModel *employeesModel = new EmployeeTableModel(filteredEmployeesModel);
    filteredEmployeesModel->setSourceModel(employeesModel);
    ui->employeesView->setModel(filteredEmployeesModel);
    ui->employeesView->hideColumn(EMPLOYEE_COLUMN_ACTIVE);
    connect(ui->addEmployeeButton, &QPushButton::clicked,
            employeesModel, &EmployeeTableModel::addRow);
    QHeaderView *header = ui->employeesView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSortIndicatorShown(true);
    connect(header, &QHeaderView::sortIndicatorChanged,
            filteredEmployeesModel, &SortFilterEmployeeModel::sort);
    _employeesContextMenu = createEmployeesContextMenu(filteredEmployeesModel);
    ui->employeesView->installEventFilter(this);
    _tasksModel = new TasksTableModel(this);
    ui->tasksView->setModel(_tasksModel);
    ui->tasksView->hideColumn(TASK_COLUMN_ID);
    ui->tasksView->hideColumn(TASK_COLUMN_ALL_FINISHED);
    ui->tasksView->hideColumn(TASK_COLUMN_SOME_FINISHED);
    connect(ui->newTaskButton, &QPushButton::clicked,
            _tasksModel, &TasksTableModel::addRow);
    connect(ui->tasksView, &QTableView::doubleClicked, this, &MainWindow::showTaskAssignmentDialog);
    header = ui->tasksView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSortIndicatorShown(true);
}

QMenu* MainWindow::createEmployeesContextMenu(SortFilterEmployeeModel *model)
{
    QMenu *menu = new QMenu(ui->employeesView);
    QAction *showActive = new QAction(ui->employeesView);
    showActive->setText("Aktywni");
    showActive->setCheckable(true);
    showActive->setChecked(true);
    connect(showActive, &QAction::toggled, model, &SortFilterEmployeeModel::showActive);
    menu->addAction(showActive);
    QAction *showInactive = new QAction(ui->employeesView);
    showInactive->setText("Nieaktywni");
    showInactive->setCheckable(true);
    showInactive->setChecked(true);
    connect(showInactive, &QAction::toggled, model, &SortFilterEmployeeModel::showInactive);
    menu->addAction(showInactive);
    return menu;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == ui->employeesView && e->type() == QEvent::ContextMenu)
    {
        QContextMenuEvent* cme = static_cast<QContextMenuEvent*>(e);
        _employeesContextMenu->exec(cme->globalPos());
        return true;
    }
    return QDialog::eventFilter(obj, e);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showTaskAssignmentDialog(const QModelIndex& index)
{
    try
    {
        QModelIndex idx = _tasksModel->index(index.row(), TASK_COLUMN_ID);
        bool ok = false;
        int taskId = _tasksModel->data(idx).toInt(&ok);
        assert(ok);
        auto& findAllEmployees = findAllActiveEmployeesQ();
        findAllEmployees.execute();
        std::set<std::string> allEmployees;
        std::string employee;
        while (findAllEmployees.next(employee))
        {
            allEmployees.insert(std::move(employee));
        }
        auto& findAssignedEmployees = findAllEmployeesAssignedToTaskQ();
        findAssignedEmployees.execute(taskId);
        std::set<std::string> activeAssignedEmployees;
        std::set<std::string> inactiveAssignedEmployees;
        TaskAssignedEmployee assignedEmployee;
        while (findAssignedEmployees.next(assignedEmployee))
        {
            if (assignedEmployee._assignmentActive)
            {
                activeAssignedEmployees.insert(std::move(assignedEmployee._employeeId));
            }
            else
            {
                inactiveAssignedEmployees.insert(std::move(assignedEmployee._employeeId));
            }
        }
        std::set<std::string> availableEmployees;
        std::set_difference(allEmployees.begin(), allEmployees.end(),
                            activeAssignedEmployees.begin(), activeAssignedEmployees.end(),
                            std::inserter(availableEmployees, availableEmployees.end()));
        std::set<std::string> assignedEmployees = activeAssignedEmployees;
        TaskAssignmentDialog dialog(availableEmployees, assignedEmployees, this);
        if (dialog.exec())
        {
            auto& changeStatus = changeEmployeeTaskAssignmentStatusC();
            auto& add = addEmployeeToTaskC();
            for (const auto& employee : availableEmployees)
            {
                if (activeAssignedEmployees.find(employee) != activeAssignedEmployees.end())
                {
                    changeStatus.execute(false, employee, taskId);
                }
            }
            for (const auto& employee : assignedEmployees)
            {
                if (activeAssignedEmployees.find(employee) != activeAssignedEmployees.end())
                {
                    // nothing to do
                }
                else if (inactiveAssignedEmployees.find(employee) != inactiveAssignedEmployees.end())
                {
                    changeStatus.execute(true, employee, taskId);
                }
                else
                {
                    add.execute(employee, taskId);
                }
            }
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}
