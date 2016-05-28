#include "formatedexception.h"
#include <sstream>

const char* FormatedException::what() const noexcept
{
    if (! _whatBuffer)
    {
        std::stringstream stream;
        formatWhatMsg(stream);
        _whatBuffer = std::make_unique<std::string>(stream.str());
    }
    return _whatBuffer->c_str();
}
