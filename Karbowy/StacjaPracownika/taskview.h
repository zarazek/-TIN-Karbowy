#ifndef TASKVIEW_H
#define TASKVIEW_H

#include <QWidget>
#include "logentry.h"

namespace Ui {
class TaskView;
}

// class QTimer;
// class QModelIndex;
class TaskTableModel;

class TaskView : public QWidget
{
    Q_OBJECT

public:
    explicit TaskView(QWidget *parent = 0);
    ~TaskView();
    void setData(TaskTableModel& model, size_t rowIdx);

signals:
    void switchingOff();

private:
    Ui::TaskView *_ui;
    QTimer *_timer;
    TaskTableModel* _model;
    size_t _rowIdx;

    void setModel(TaskTableModel& model);
    void startTimer();
    void tick();
    void stopTimer();
    void finishTask();
    void switchOff();
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setTimeSpent(const QString& timeSpent);
    void setActive(bool active);

    void modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
};

#endif // TASKVIEW_H
