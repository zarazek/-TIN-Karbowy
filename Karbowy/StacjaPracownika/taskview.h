#ifndef TASKVIEW_H
#define TASKVIEW_H

#include <QWidget>

namespace Ui {
class TaskView;
}

class TaskView : public QWidget
{
    Q_OBJECT

public:
    explicit TaskView(QWidget *parent = 0);
    ~TaskView();

private:
    Ui::TaskView *ui;
};

#endif // TASKVIEW_H
