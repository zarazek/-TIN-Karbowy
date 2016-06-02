#ifndef SYSTEMERROR_H
#define SYSTEMERROR_H

#include "formatedexception.h"

class SystemError : public FormatedException
{
public:
    SystemError(std::string&& errorMsg);
private:
    std::string _errorMsg;
    int _errno;
    std::string _errorStr;

    void formatWhatMsg(std::ostream& stream) const override;
};

#endif // SYSTEMERROR_H
