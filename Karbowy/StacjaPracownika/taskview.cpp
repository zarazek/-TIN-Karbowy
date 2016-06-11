#include "taskview.h"
#include "ui_taskview.h"
#include "tasktablemodel.h"
#include <QTimer>

TaskView::TaskView(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::TaskView),
    _timer(new QTimer(this)),
    _model(nullptr),
    _rowIdx(std::numeric_limits<size_t>::max())
{
    _ui->setupUi(this);
    connect(_ui->startButton, &QPushButton::clicked, this, &TaskView::startTimer);
    connect(_timer, &QTimer::timeout, this, &TaskView::tick);
    connect(_ui->pauseButton, &QPushButton::clicked, this, &TaskView::stopTimer);
    connect(_ui->finishButton, &QPushButton::clicked, this, &TaskView::finishTask);
    connect(_ui->pushButton_4, &QPushButton::clicked, this, &TaskView::switchOff);
}

TaskView::~TaskView()
{
    delete _ui;
}

void TaskView::setData(TaskTableModel& model, size_t rowIdx)
{
    setModel(model);
    _rowIdx = rowIdx;
    setTitle(model.title(rowIdx));
    setDescription(model.description(rowIdx));
    setTimeSpent(model.timeSpent(rowIdx));
    setActive(model.active(rowIdx));
}

void TaskView::setModel(TaskTableModel& model)
{
    if (_model != &model)
    {
        _model = &model;
        connect(&model, &TaskTableModel::dataChanged, this, &TaskView::modelDataChanged);
        connect(&model, &TaskTableModel::taskActivated, [this](size_t rowIdx) { if (rowIdx == _rowIdx) setActive(true); });
        connect(&model, &TaskTableModel::taskDeactivated, [this](size_t rowIdx) { if (rowIdx == _rowIdx) setActive(false); });
    }
}

void TaskView::startTimer()
{
    _model->startWork(_rowIdx);
    _timer->start(1000);
}

void TaskView::tick()
{
    _model->workCheckpoint(_rowIdx);
}

void TaskView::stopTimer()
{
    _timer->stop();
    _model->pauseWork(_rowIdx);
}

void TaskView::finishTask()
{
    _timer->stop();
    _model->finishWork(_rowIdx);
    emit switchingOff();
}

void TaskView::switchOff()
{
    if (_model->active(_rowIdx))
    {
        stopTimer();
    }
    emit switchingOff();
}

void TaskView::setTitle(const QString& title)
{
    _ui->titleLabel->setText(title);
}

void TaskView::setDescription(const QString& description)
{
    _ui->descriptionEdit->document()->setPlainText(description);
}

void TaskView::setTimeSpent(const QString& timeSpent)
{
    _ui->timeSpentLabel->setText(timeSpent);
}

void TaskView::setActive(bool active)
{
    if (active)
    {
        _ui->startButton->setEnabled(false);
        _ui->pauseButton->setEnabled(true);
    }
    else
    {
        _ui->startButton->setEnabled(true);
        _ui->pauseButton->setEnabled(false);
    }
}

void TaskView::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (topLeft.row() <= _rowIdx && _rowIdx <= bottomRight.row())
    {
        if (topLeft.column() <= TaskTableModel::ColumnIndex_TITLE && TaskTableModel::ColumnIndex_TITLE <= bottomRight.column())
        {
            setTitle(_model->title(_rowIdx));
        }
        if (topLeft.column() <= TaskTableModel::ColumnIndex_DESCRIPTION && TaskTableModel::ColumnIndex_DESCRIPTION <= bottomRight.column())
        {
            setDescription(_model->description(_rowIdx));
        }
        if (topLeft.column() <= TaskTableModel::ColumnIndex_TIME_SPENT && TaskTableModel::ColumnIndex_TIME_SPENT <= bottomRight.column())
        {
            setTimeSpent(_model->timeSpent(_rowIdx));
        }
    }
}
