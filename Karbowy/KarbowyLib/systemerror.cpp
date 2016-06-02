#include "systemerror.h"
#include <string.h>

SystemError::SystemError(std::string&& errorMsg) :
    _errorMsg(std::forward<std::string>(errorMsg)),
    _errno(errno),
    _errorStr(strerror(_errno)) { }

void SystemError::formatWhatMsg(std::ostream& stream) const
{
    stream << _errorMsg << ": " << _errorStr << " (errno " << _errno << ')';
}
