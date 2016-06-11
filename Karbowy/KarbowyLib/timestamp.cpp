#include "timestamp.h"
#include <boost/format.hpp>
#include <sstream>

int toSeconds(Duration duration)
{
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

std::string formatTimestamp(const Timestamp &timestamp)
{
    time_t time = Clock::to_time_t(timestamp);
    tm t;
    if (gmtime_r(&time, &t) == nullptr)
    {
        throw std::runtime_error("gmtime_r error");
    }
    int milis = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - Clock::from_time_t(time)).count();
    std::ostringstream stream;
    stream << boost::format("%04d-%02d-%02d %02d:%02d:%02d.%03d") % (t.tm_year + 1900) % (t.tm_mon + 1) % t.tm_mday
                                                                  % t.tm_hour  % t.tm_min % t.tm_sec %  milis;
    return stream.str();
}
