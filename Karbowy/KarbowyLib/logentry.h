#ifndef LOGENTRY_H
#define LOGENTRY_H

#include "timestamp.h"
#include <boost/optional.hpp>

enum LogEntryType
{
    LogEntryType_LOGIN = 0,
    LogEntryType_LOGOUT = 1,
    LogEntryType_TASK_START = 2,
    LogEntryType_TASK_PAUSE = 3,
    LogEntryType_TASK_FINISH = 4
};

struct LogEntry
{
    LogEntryType _type;
    Timestamp _timestamp;
    std::string _userId;
    boost::optional<int> _taskId;
};

#endif // LOGENTRY_H
