#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "commongui.h"
#include "sortfilteremployeemodel.h"
#include "employeetablemodel.h"
#include "taskstablemodel.h"
#include <QContextMenuEvent>
#include <QMenu>

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
    TasksTableModel *tasksModel = new TasksTableModel(this);
    ui->tasksView->setModel(tasksModel);
    ui->tasksView->hideColumn(TASK_COLUMN_ID);
    ui->tasksView->hideColumn(TASK_COLUMN_ALL_FINISHED);
    ui->tasksView->hideColumn(TASK_COLUMN_SOME_FINISHED);
    connect(ui->newTaskButton, &QPushButton::clicked,
            tasksModel, &TasksTableModel::addRow);
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
