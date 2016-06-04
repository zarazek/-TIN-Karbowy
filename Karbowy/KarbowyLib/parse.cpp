#include "parse.h"

IntToken::IntToken(int& target) :
    _target(target) { }

bool IntToken::parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const
{
    int digitsCount = 0;
    int result = 0;
    for (; begin < end && isdigit(*begin); ++begin)
    {
        result = 10*result + (*begin - '0');
        ++digitsCount;
    }
    if (digitsCount > 0)
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

namespace impl
{

bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end)
{
    return begin == end;
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
