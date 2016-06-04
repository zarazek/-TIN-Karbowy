#ifndef PROTOCOLERROR_H
#define PROTOCOLERROR_H

#include "formatedexception.h"

class ProtocolError : public FormatedException
{
public:
    ProtocolError(const std::string& errorMsg, const std::string& line);
private:
    std::string _errorMsg;
    std::string _line;

    virtual void formatWhatMsg(std::ostream& stream) const;
};

#endif // PROTOCOLERROR_H
