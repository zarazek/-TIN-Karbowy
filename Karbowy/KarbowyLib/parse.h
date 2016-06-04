#ifndef PARSE_H
#define PARSE_H

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
    IntToken(int &target);
    bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const override;
private:
    int& _target;
};

class QuotedStringToken : public Token
{
public:
    QuotedStringToken(std::string& target);
    bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end) const override;
private:
    std::string& _target;
};

namespace impl
{

bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end);
template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const char* str, const Args&... args);
template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const std::string& str, const Args&... args);
template <typename... Args>
bool parse(std::string::const_iterator& begin, const std::string::const_iterator& end, const Token& token, const Args&... args);


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
    return impl::parse(begin, str.end(), args...);
}

std::string quoteString(const std::string& str);

#endif
