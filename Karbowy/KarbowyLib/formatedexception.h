#ifndef FORMATEDEXCEPTION_H
#define FORMATEDEXCEPTION_H

#include <exception>
#include <memory>
#include <ostream>

class FormatedException : public std::exception
{
public:
    const char* what() const noexcept override;

private:
    mutable std::unique_ptr<std::string> _whatBuffer;

    virtual void formatWhatMsg(std::ostream& stream) const = 0;
};

#endif // FORMATEDEXCEPTION_H
