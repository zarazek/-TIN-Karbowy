#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logindialog.h"
#include "communicationthread.h"

MainWindow::MainWindow(std::string&& myUuid, CommunicationThread& commThread, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _myUuid(std::forward<std::string>(myUuid)),
    _commThread(commThread)
{
    ui->setupUi(this);
    connect(ui->connectAction, &QAction::triggered, this, &MainWindow::showLoginDialog);
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
        _commThread.setClientConfig(*_config);
    }
}
