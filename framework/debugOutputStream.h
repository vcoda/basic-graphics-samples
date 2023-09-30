#pragma once
#include <streambuf>

class DebugOutputStream : public std::streambuf
{
public:
    DebugOutputStream()
    {
        std::cout.rdbuf(this);
        std::cerr.rdbuf(this);
    }

    virtual int_type overflow(int_type n)
    {
        if (n != std::streambuf::traits_type::eof())
        {
            if (pos == sizeof(buf) - 2)
            {
                buf[pos++] = '\n';
                flush();
            }
            const char ch = static_cast<char>(n);
            buf[pos++] = ch;
            if ('\n' == ch || '.' == ch || ',' == ch)
                flush();
        }
        return n;
    }

    void flush()
    {
        std::lock_guard<std::mutex> lock(mtx);
        buf[pos] = '\0';
        OutputDebugStringA(buf);
        pos = 0;
    }

private:
    char buf[4096 + 1];
    size_t pos = 0;
    std::mutex mtx;
};
