#include "linebuffer.h"

#include <string.h>
#include <stdexcept>
#include <assert.h>

LineBuffer::LineBuffer() :
    _lastLineComplete(true),
    _eofReceived(false) { }

bool LineBuffer::hasFullLine() const
{
    size_t size = _lines.size();
    return size > 1 || (size > 0 && _lastLineComplete);
}

bool LineBuffer::isEof() const
{
    return _eofReceived && _lines.empty();
}

std::string LineBuffer::getFirstLine()
{
    assert(hasFullLine());
    std::string line = _lines.front();
    _lines.pop_front();
    return line;
}

void LineBuffer::addData(const char* data, size_t size)
{
    assert(! _eofReceived);

    while (size > 0)
    {
        if (_lastLineComplete)
        {
            _lines.push_back(std::string());
        }
        std::string& target = _lines.back();
        const char* delimeter = static_cast<const char*>(memchr(data, '\n', size));
        if (delimeter)
        {
            target.insert(target.end(), data, delimeter);
            _lastLineComplete = true;
            size -= delimeter - data + 1;
            data = delimeter + 1;
        }
        else
        {
            target.insert(target.end(), data, data + size);
            _lastLineComplete = false;
            size = 0;
        }
    }
}

void LineBuffer::setEof()
{
    assert(! _eofReceived);

    _eofReceived = true;
    _lastLineComplete = true;
}

