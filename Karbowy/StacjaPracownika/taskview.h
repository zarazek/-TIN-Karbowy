#ifndef TASKVIEW_H
#define TASKVIEW_H

#include <QWidget>
#include "timestamp.h"
#include "logentry.h"

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
    Duration _duration;
    Timestamp _lastCheckpoint;

    void startCounting();
    void stopCounting();
    void finishTask();
    void switchOff();
    void tick();

    void updateDuration(Timestamp newCheckpoint);
    void addLogEntry(LogEntryType type, Timestamp timestamp);
    void startTimer();
    void stopTimer();

};

#endif // TASKVIEW_H
