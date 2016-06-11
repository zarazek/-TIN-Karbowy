#include "task.h"
#include <boost/algorithm/string.hpp>

ClientTask::ClientTask(int id, std::string &&title, const std::string& description, Duration timeSpent) :
    _id(id),
    _title(std::forward<std::string>(title)),
    _timeSpent(timeSpent),
    _workingNow(false)
{
    boost::split(_description, description, [](char c){ return c == '\n'; });
}
