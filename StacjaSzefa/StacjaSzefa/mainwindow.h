#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>

namespace Ui {
class MainWindow;
}

class QMenu;
class SortFilterEmployeeModel;

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    Ui::MainWindow *ui;
    QMenu* _employeesContextMenu;

    QMenu *createEmployeesContextMenu(SortFilterEmployeeModel* model);
};

#endif // MAINWINDOW_H
