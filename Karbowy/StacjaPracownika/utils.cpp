#include "utils.h"
#include <chrono>

QString formatTime(int timeInSeconds)
{
    std::chrono::seconds t(timeInSeconds);
    int hours = std::chrono::duration_cast<std::chrono::hours>(t).count();
    t -= std::chrono::hours(hours);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(t).count();
    t -= std::chrono::minutes(minutes);
    int seconds = t.count();
    return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

QString join(const std::vector<std::string>& lst)
{
    QString res;
    if (! lst.empty())
    {
        res += lst.cbegin()->c_str();
        std::for_each(lst.cbegin() + 1, lst.cend(), [&res](const auto& str){ res += '\n'; res += str.c_str(); });
    }
    return res;
}
