#include "utils.h"
#include <chrono>

QString formatTime(Duration duration)
{
    int hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    duration -= std::chrono::hours(hours);
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    duration -= std::chrono::minutes(minutes);
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

QString formatTime(int durationInSeconds)
{
    return formatTime(std::chrono::seconds(durationInSeconds));
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
