#ifndef LINEBUFFER_H
#define LINEBUFFER_H

#include <string>
#include <deque>

class LineBuffer
{
public:
    LineBuffer();
    LineBuffer(const LineBuffer&) = delete;
    LineBuffer(LineBuffer&& other) = default;
    LineBuffer& operator=(const LineBuffer&) = delete;
    LineBuffer& operator=(LineBuffer&&) = default;

    bool hasFullLine() const;
    std::string getFirstLine();
    bool isEof() const;

    void addData(const char* data, size_t size);
    void setEof();
private:
    std::deque<std::string> _lines;
    bool _lastLineComplete;
    bool _eofReceived;
};


#endif // LINEBUFFER_H
