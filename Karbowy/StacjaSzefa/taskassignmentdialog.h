#ifndef TASKASSIGNMENTDIALOG_H
#define TASKASSIGNMENTDIALOG_H

#include <QDialog>
#include <string>
#include <set>

namespace Ui {
class TaskAssignmentDialog;
}

class QListWidget;

class TaskAssignmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TaskAssignmentDialog(std::set<std::string>& availableEmployees,
                                  std::set<std::string>& assignedEmployees,
                                  QWidget *parent = 0);
    ~TaskAssignmentDialog();
private:
    Ui::TaskAssignmentDialog *_ui;
    std::set<std::string>& _availableEmployees;
    std::set<std::string>& _assignedEmployees;

    void addEmployees();
    void removeEmployees();
    static void fillWidget(QListWidget* widget, const std::set<std::string>& set);
    static void moveItems(QListWidget* fromWidget, std::set<std::string>& fromSet,
                          QListWidget* toWidget, std::set<std::string>& toSet);
};

#endif // TASKASSIGNMENTDIALOG_H
