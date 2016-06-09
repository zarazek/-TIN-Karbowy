#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include "communicationthread.h"
#include "tasktablemodel.h"

MainWindow::MainWindow(std::string&& myUuid, CommunicationThread& commThread, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _myUuid(std::forward<std::string>(myUuid)),
    _commThread(commThread)
{
    ui->setupUi(this);
    connect(ui->connectAction, &QAction::triggered, this, &MainWindow::showLoginDialog);
    connect(ui->retrieveTasksAction, &QAction::triggered, &_commThread, &CommunicationThread::retrieveTasks);
    connect(ui->sendLogsAction, &QAction::triggered, &_commThread, &CommunicationThread::sendLogs);
    connect(ui->disconnectAction, &QAction::triggered, &_commThread, &CommunicationThread::logout);

    QHeaderView *header = ui->tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSortIndicatorShown(true);
    TaskTableModel *model = new TaskTableModel(this);
    connect(&_commThread, &CommunicationThread::loggedIn, model, &TaskTableModel::setEmployeeId, Qt::QueuedConnection);
    connect(&_commThread, &CommunicationThread::tasksRetrieved, model, &TaskTableModel::refresh, Qt::QueuedConnection);
    ui->tableView->setModel(model);
    connect(ui->tableView, &QTableView::doubleClicked, this, &MainWindow::showSingleTaskView);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showLoginDialog()
{
    LoginDialog dialog(_myUuid, _config.get(), this);
    auto res = dialog.retrieveResults();
    if (res)
    {
        _config = std::move(res);
        _commThread.login(*_config);
    }
}

void MainWindow::showSingleTaskView(const QModelIndex& index)
{
    assert(false);
}

