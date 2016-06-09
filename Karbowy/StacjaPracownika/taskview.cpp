#include "taskview.h"
#include "ui_taskview.h"
#include "task.h"
#include "logentry.h"
#include "utils.h"
#include "predefinedqueries.h"
#include <QTimer>

TaskView::TaskView(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::TaskView),
    _timer(new QTimer(this)),
    _running(false)
{
    _ui->setupUi(this);
    connect(_ui->startButton, &QPushButton::clicked, this, &TaskView::startCounting);
    connect(_ui->pauseButton, &QPushButton::clicked, this, &TaskView::stopCounting);
    connect(_ui->finishButton, &QPushButton::clicked, this, &TaskView::finishTask);
    connect(_ui->pushButton_4, &QPushButton::clicked, this, &TaskView::switchOff);
    connect(_timer, &QTimer::timeout, this, &TaskView::tick);
    _ui->startButton->setDisabled(false);
    _ui->pauseButton->setDisabled(true);
}

TaskView::~TaskView()
{
    delete _ui;
}

void TaskView::setData(int employeeId, const ClientTask& task)
{
    assert(! _running);

    _employeeId = employeeId;
    _taskId = task._id;
    _secondsSpent = task._secondsSpent;
    _ui->titleLabel->setText(task._title.c_str());
    // ui->descriptionEdit->TODO
    _ui->timeSpentLabel->setText(formatTime(_secondsSpent));
}

void TaskView::startCounting()
{
    assert(! _running);

    Timestamp startTime = Clock::now();
    _timer->start(1000);
    auto& cmd = insertLogEntryC();
    cmd.execute(LogEntryType_TASK_START, _employeeId, startTime, boost::optional<int>(_taskId));
    _ui->startButton->setDisabled(true);
    _ui->pauseButton->setDisabled(false);
    _running = true;
}

void TaskView::stopCounting()
{
    assert(_running);

    _timer->stop();
    Timestamp stopTime = Clock::now();
    auto& cmd = insertLogEntryC();
    cmd.execute(LogEntryType_TASK_PAUSE, _employeeId, stopTime, boost::optional<int>(_taskId));
    _ui->startButton->setDisabled(false);
    _ui->pauseButton->setDisabled(true);
    _running = false;
}

void TaskView::finishTask()
{
    Timestamp stopTime = Clock::now();
    auto& cmd = insertLogEntryC();
    if (_running)
    {
        _timer->stop();
        cmd.execute(LogEntryType_TASK_FINISH, _employeeId, stopTime, boost::optional<int>(_taskId));
    }
    else
    {
        cmd.execute(LogEntryType_TASK_START, _employeeId, stopTime, boost::optional<int>(_taskId));
        cmd.execute(LogEntryType_TASK_FINISH, _employeeId, stopTime, boost::optional<int>(_taskId));
    }
    _running = false;
    emit switchingOff();
}

void TaskView::switchOff()
{
    if (_running)
    {
        stopCounting();
    }
    emit switchingOff();
}

void TaskView::tick()
{
    ++_secondsSpent;
    _ui->timeSpentLabel->setText(formatTime(_secondsSpent));
}
