#ifndef CONCAT_H
#define CONCAT_H

#include <sstream>

namespace impl
{

inline void streamTo(std::ostream&) { }

template <typename Arg, typename... RestOfArgs>
void streamTo(std::ostream& stream, const Arg& arg, const RestOfArgs&... args)
{
    stream << arg;
    streamTo(stream, args...);
}

}

template <typename... Args>
std::string concat(const Args&... args)
{
    std::ostringstream stream;
    impl::streamTo(stream, args...);
    return stream.str();
}

template <typename... Args>
std::string concatln(const Args&... args)
{
    std::ostringstream stream;
    impl::streamTo(stream, args...);
    stream << std::endl;
    return stream.str();
}

#endif // CONCAT_H
