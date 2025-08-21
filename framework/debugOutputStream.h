#pragma once
#include <streambuf>
#include <mutex>

class DebugOutputStream : public std::streambuf
{
public:
    DebugOutputStream():
        cout(std::cout.rdbuf(this)),
        cerr(std::cerr.rdbuf(this))
    {}

    ~DebugOutputStream()
    {
        std::cout.rdbuf(cout);
        std::cerr.rdbuf(cerr);
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
        OutputDebugStringA(buf);
        pos = 0;
    }

private:
    std::streambuf *cout, *cerr;
    char buf[4096 + 1];
    size_t pos = 0;
    std::mutex mtx;
};
