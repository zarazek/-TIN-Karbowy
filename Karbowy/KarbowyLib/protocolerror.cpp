#include "protocolerror.h"

ProtocolError::ProtocolError(const std::string& errorMsg, const std::string& line) :
    _errorMsg(errorMsg),
    _line(line) { }

void ProtocolError::formatWhatMsg(std::ostream& stream) const
{
    stream << _errorMsg << ": '" << _line << '\'';
}
