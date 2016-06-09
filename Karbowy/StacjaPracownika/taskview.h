#ifndef TASKVIEW_H
#define TASKVIEW_H

#include <QWidget>
#include "timestamp.h"

namespace Ui {
class TaskView;
}

class ClientTask;

class TaskView : public QWidget
{
    Q_OBJECT

public:
    explicit TaskView(QWidget *parent = 0);
    ~TaskView();
    void setData(int employeeId, const ClientTask& task);

signals:
    void switchingOff();

private:
    Ui::TaskView *_ui;
    QTimer *_timer;
    bool _running;
    int _employeeId;
    int _taskId;
    int _secondsSpent;

    void startCounting();
    void stopCounting();
    void finishTask();
    void switchOff();
    void tick();

    void updateTime(Timestamp upTo);

};

#endif // TASKVIEW_H
