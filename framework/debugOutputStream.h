#pragma once
#include <streambuf>
#include <mutex>

class DebugOutputStream : public std::streambuf
{
public:
    DebugOutputStream()
    {
        std::cout.rdbuf(this);
        std::cerr.rdbuf(this);
    }

    int_type overflow(int_type n) override
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (n != std::streambuf::traits_type::eof())
        {
            if (pos == sizeof(buf) - 2)
                buf[pos++] = '\n';
            else
                buf[pos++] = (char)n;
            if (buf[pos - 1] == '\n')
                flush();
        }
        return n;
    }

    void flush()
    {
        buf[pos] = '\0';
    #ifdef QT_CORE_LIB
        qDebug() << buf;
    #else
        OutputDebugStringA(buf);
    #endif
        pos = 0;
    }

private:
    char buf[4096 + 1];
    size_t pos = 0;
    std::mutex mtx;
};
