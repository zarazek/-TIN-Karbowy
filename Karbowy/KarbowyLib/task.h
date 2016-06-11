#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>
#include "timestamp.h"

struct ClientTask
{
    int _id;
    std::string _title;
    std::vector<std::string> _description;
    Duration _timeSpent;
    Timestamp _lastCheckpoint;
    bool _workingNow;

    ClientTask() = default;
    ClientTask(int id, std::string&& title, const std::string& description, Duration timeSpent);
};

#endif
