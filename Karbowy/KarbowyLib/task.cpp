#include "task.h"
#include <boost/algorithm/string.hpp>

ClientTask::ClientTask(int id, std::string &&title, const std::string& description, int secondsSpent) :
    _id(id),
    _title(std::forward<std::string>(title)),
    _secondsSpent(secondsSpent)
{
    boost::split(_description, description, [](char c){ return c == '\n'; });
}
