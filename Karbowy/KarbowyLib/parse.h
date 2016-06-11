#ifndef PARSE_H
#define PARSE_H

#include "timestamp.h"
#include <boost/algorithm/string.hpp>

class Token
{
public:
    virtual ~Token() { }
    virtual bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const = 0;
};

class IntToken : public Token
{
public:
    IntToken(int &target, int digitCount = -1);
    bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const override;
private:
    int& _target;
    int _digitCount;
};

class BareStringToken : public Token
{
public:
    BareStringToken(std::string& target);
    bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const override;
private:
    std::string& _target;
};

class QuotedStringToken : public Token
{
public:
    QuotedStringToken(std::string& target);
    bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const override;
private:
    std::string& _target;
};

class TimestampToken : public Token
{
public:
    TimestampToken(Timestamp& target);
    bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const override;
private:
    Timestamp& _target;
};

class SecondsToken : public Token
{
public:
    SecondsToken(Duration& target);
    bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const override;
private:
    Duration& _target;
};

namespace impl
{

bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end);
template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, char c, const Args&... args);
template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const char* str, const Args&... args);
template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const std::string& str, const Args&... args);
template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const Token& token, const Args&... args);

inline bool parse(std::string::const_iterator&, const std::string::const_iterator&)
{
    return true;
}

template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, char c, const Args&... args)
{
    if (begin < end && *begin++ == c)
    {
        return parse(begin, end, args...);
    }
    else
    {
        return false;
    }
}

template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const char* str, const Args&... args)
{
    if (boost::istarts_with(std::make_pair(begin, end), str))
    {
        begin += strlen(str);
        return parse(begin, end, args...);
    }
    else
    {
        return false;
    }
}

template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const std::string& str, const Args&... args)
{
    if (boost::istarts_with(std::make_pair(begin, end), str))
    {
        begin += str.length();
        return parse(begin, end, args...);
    }
    else
    {
        return false;
    }
}

template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const Token& token, const Args&... args)
{
    if (token.parse(begin, end))
    {
        return parse(begin, end, args...);
    }
    else
    {
        return false;
    }
}

}

template <typename... Args>
bool parse(const std::string& str, const Args&... args)
{
    auto begin = str.begin();
    if (! impl::parse(begin, str.end(), args...))
    {
        return false;
    }
    else
    {
        return begin == str.end();
    }
}

std::string quoteString(const std::string& str);

#endif
