#include "taskview.h"
#include "ui_taskview.h"
#include "task.h"
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
    _duration = std::chrono::seconds(task._secondsSpent);
    _ui->titleLabel->setText(task._title.c_str());
    _ui->descriptionEdit->document()->setPlainText(join(task._description));
    _ui->timeSpentLabel->setText(formatTime(_duration));
}

void TaskView::startCounting()
{
    _lastCheckpoint = Clock::now();
    addLogEntry(LogEntryType_TASK_START, _lastCheckpoint);
    startTimer();
}

void TaskView::stopCounting()
{
    Timestamp stopTime = Clock::now();
    addLogEntry(LogEntryType_TASK_PAUSE, stopTime);
    updateDuration(stopTime);
    stopTimer();
}

void TaskView::finishTask()
{
    Timestamp stopTime = Clock::now();
    if (_running)
    {
        stopTimer();
        addLogEntry(LogEntryType_TASK_FINISH, stopTime);
        updateDuration(stopTime);
    }
    else
    {
        addLogEntry(LogEntryType_TASK_START, stopTime);
        addLogEntry(LogEntryType_TASK_FINISH, stopTime);
    }
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
    updateDuration(Clock::now());
}

void TaskView::updateDuration(Timestamp newCheckpoint)
{
    _duration += newCheckpoint - _lastCheckpoint;
    auto& updateTimeSpentCmd = updateTimeSpentOnTaskC();
    updateTimeSpentCmd.execute(_duration, _employeeId, _taskId);
    _ui->timeSpentLabel->setText(formatTime(_duration));
    _lastCheckpoint = newCheckpoint;
}

void TaskView::addLogEntry(LogEntryType type, Timestamp timestamp)
{
    auto& addLogEntryCmd = insertLogEntryC();
    addLogEntryCmd.execute(type, _employeeId, timestamp, boost::optional<int>(_taskId));
}

void TaskView::startTimer()
{
    assert(! _running);

    _timer->start(1000);
    _ui->startButton->setEnabled(false);
    _ui->pauseButton->setEnabled(true);
    _running = true;
}

void TaskView::stopTimer()
{
    assert(_running);
    _timer->stop();
    _ui->startButton->setDisabled(false);
    _ui->pauseButton->setDisabled(true);
    _running = false;
}
