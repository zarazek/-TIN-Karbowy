#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace Ui {
class MainWindow;
}

class CommunicationThread;
class TaskTableModel;
class ClientConfig;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(std::string&& myUuid, CommunicationThread& commThread, QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    std::string _myUuid;
    CommunicationThread& _commThread;
    TaskTableModel *_model;
    std::unique_ptr<ClientConfig> _config;

    void showLoginDialog();
    void showAllTasksView();
    void showSingleTaskView(const QModelIndex& index);
    void showTableView();
};

#endif // MAINWINDOW_H
