#include "taskview.h"
#include "ui_taskview.h"

TaskView::TaskView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaskView)
{
    ui->setupUi(this);
}

TaskView::~TaskView()
{
    delete ui;
}
