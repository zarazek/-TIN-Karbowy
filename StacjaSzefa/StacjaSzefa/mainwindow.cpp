#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "checkboxdelegate.h"
#include <QSqlQueryModel>
#include <QSqlTableModel>

MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSqlQueryModel* employeesModel = new QSqlQueryModel(this);
    employeesModel->setQuery("SELECT * FROM Employees", QSqlDatabase::database("KarbowyDb"));
//    QSqlTableModel *employeesModel = new QSqlTableModel(this, QSqlDatabase::database("KarbowyDb"));
//    employeesModel->setTable("Employees");
//    employeesModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    ui->employeesView->setModel(employeesModel);
    CheckBoxDelegate *delegate = new CheckBoxDelegate(ui->employeesView);
    ui->employeesView->setItemDelegateForColumn(3, delegate);
}

MainWindow::~MainWindow()
{
    delete ui;
}
