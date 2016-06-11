#include "parse.h"
#include <boost/format.hpp>
#include <sstream>

IntToken::IntToken(int& target, int digitCount) :
    _target(target),
    _digitCount(digitCount) { }

bool IntToken::parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const
{
    int digitCount = 0;
    int result = 0;
    for (; begin < end && isdigit(*begin); ++begin)
    {
        result = 10*result + (*begin - '0');
        ++digitCount;
    }
    if (digitCount > 0)
    {
        if (_digitCount < 0 || _digitCount == digitCount)
        {
            _target = result;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

BareStringToken::BareStringToken(std::string& target) :
    _target(target) { }

bool BareStringToken::parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const
{
    int charCount = 0;
    std::string result;
    for (; begin < end && isalnum(*begin); ++begin)
    {
        result += *begin;
        ++charCount;
    }
    if (charCount > 0)
    {
        _target = result;
        return true;
    }
    else
    {
        return false;
    }
}

QuotedStringToken::QuotedStringToken(std::string& target) :
    _target(target) { }

bool QuotedStringToken::parse(std::string::const_iterator &begin, const std::string::const_iterator &end) const
{
    if (begin >= end || *begin++ != '"')
    {
        return false;
    }
    std::string result;
    bool escaped = false;
    bool stop = false;
    while (begin < end && ! stop)
    {
        char c = *begin++;
        switch (c)
        {
        case '"':
            if (escaped)
            {
                result += c;
                escaped = false;
            }
            else
            {
                stop = true;
            }
            break;
        case '\\':
            if (escaped)
            {
                result += c;
                escaped = false;
            }
            else
            {
                escaped = true;
            }
            break;
        default:
            result += c;
            escaped = false;
        }
    }

    if (stop)
    {
        _target = result;
        return true;
    }
    else
    {
        return false;
    }
}

TimestampToken::TimestampToken(Timestamp &target) :
    _target(target) { }

bool TimestampToken::parse(std::string::const_iterator &begin, const std::string::const_iterator &end) const
{
    tm t;
    memset(&t, 0, sizeof(t));
    int millis;
    if (! impl::parse(begin, end,
                      IntToken(t.tm_year, 4), '-', IntToken(t.tm_mon, 2), '-', IntToken(t.tm_mday, 2), ' ',
                      IntToken(t.tm_hour, 2), ':', IntToken(t.tm_min, 2), ':', IntToken(t.tm_sec, 2), '.', IntToken(millis, 3)))
    {
        return false;
    }
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    time_t time = mktime(&t);
    if (time == -1)
    {
        return false;
    }
    _target = Clock::from_time_t(time) + std::chrono::milliseconds(millis);
    return true;
}

SecondsToken::SecondsToken(Duration& target) :
    _target(target) { }

bool SecondsToken::parse(std::string::const_iterator &begin, const std::string::const_iterator &end) const
{
    int count;
    IntToken token(count);
    if (token.parse(begin, end))
    {
        _target = std::chrono::seconds(count);
        return true;
    }
    else
    {
        return false;
    }
}

std::string quoteString(const std::string& str)
{
    std::string res("\"");
    for (char c : str)
    {
        switch (c)
        {
        case '"':
        case '\\':
            res += '\\';
        default:
            res += c;
        }
    }
    res += '"';
    return res;
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
