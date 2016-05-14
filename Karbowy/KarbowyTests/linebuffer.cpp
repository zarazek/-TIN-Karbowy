#include <gtest/gtest.h>
#include "linebuffer.h"

class LineBufferTest : public testing::Test
{
protected:
    LineBuffer _buffer;
};

#define EXPECT_EMPTY()                   \
    EXPECT_FALSE(_buffer.hasFullLine()); \
    EXPECT_FALSE(_buffer.isEof())

#define EXPECT_FULL(str)                   \
    EXPECT_TRUE(_buffer.hasFullLine());    \
    EXPECT_FALSE(_buffer.isEof());         \
    EXPECT_EQ(_buffer.getFirstLine(), str)

#define EXPECT_EOF()                     \
    EXPECT_FALSE(_buffer.hasFullLine()); \
    EXPECT_TRUE(_buffer.isEof())


TEST_F(LineBufferTest, InitiallyEmpty)
{
    EXPECT_EMPTY();
}

TEST_F(LineBufferTest, AddingZeroBytes)
{
    _buffer.addData(nullptr, 0);
    EXPECT_EMPTY();
}

TEST_F(LineBufferTest, AddingMultipleLines)
{
    static const char* txt = "siała baba mak\nnie wiedziała jak";
    static const size_t len = strlen(txt);
    _buffer.addData(txt, len);
    EXPECT_FULL("siała baba mak");
    EXPECT_EMPTY();
    _buffer.addData("\n", 1);
    EXPECT_FULL("nie wiedziała jak");
    EXPECT_EMPTY();
}

TEST_F(LineBufferTest, IncrementalAddLine)
{
    static const char* txt1 = "siała ";
    static const size_t len1 = strlen(txt1);
    static const char* txt2 = "baba ";
    static const size_t len2 = strlen(txt2);
    static const char* txt3 = "mak\nnie wiedziała jak\n";
    static const size_t len3 = strlen(txt3);
    _buffer.addData(txt1, len1);
    EXPECT_EMPTY();
    _buffer.addData(txt2, len2);
    EXPECT_EMPTY();
    _buffer.addData(txt3, len3);
    EXPECT_FULL("siała baba mak");
    EXPECT_FULL("nie wiedziała jak");
    EXPECT_EMPTY();
}

TEST_F(LineBufferTest, AddEmptyLines)
{
    _buffer.addData("\n\n\n", 3);
    EXPECT_FULL("");
    EXPECT_FULL("");
    EXPECT_FULL("");
    EXPECT_EMPTY();
}

TEST_F(LineBufferTest, SignalEofAtTheEndOfLine)
{
    static const char* txt = "siała baba mak\n";
    static const size_t len = strlen(txt);
    _buffer.addData(txt, len);
    _buffer.setEof();
    EXPECT_FULL("siała baba mak");
    EXPECT_EOF();
}

TEST_F(LineBufferTest, SignalEofInTheMiddleOfLine)
{
    static const char* txt = "siała baba mak";
    static const size_t len = strlen(txt);
    _buffer.addData(txt, len);
    _buffer.setEof();
    EXPECT_FULL("siała baba mak");
    EXPECT_EOF();
}
