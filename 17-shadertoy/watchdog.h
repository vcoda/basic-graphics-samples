#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>

#ifdef _MSC_VER
#define stat _stat
typedef struct _stat sys_stat;
#else
typedef struct stat sys_stat;
#endif // _MSC_VER

class FileWatchdog
{
    struct Item
    {
        std::string name;
        std::function<void(const std::string&)> onModified;
        time_t lastModifiedTime;
    };

    std::thread watchThread;
    std::mutex itemListAccess;
    std::list<Item> itemList;

public:
    FileWatchdog(const std::chrono::milliseconds pollFrequency)
    {
        watchThread = std::thread([this, pollFrequency]()
        {   // Dedicated thread that periodically checks files for modification
            while (true)
            {   // Do not abuse processor time
                std::this_thread::sleep_for(pollFrequency);
                std::lock_guard<std::mutex> guard(itemListAccess);
                for (Item& item : itemList)
                {
                    sys_stat st;
                    const int result = stat(item.name.c_str(), &st);
                    if (0 == result)
                    {
                        if (item.lastModifiedTime != st.st_mtime)
                        {
                            item.onModified(item.name);
                            item.lastModifiedTime = st.st_mtime;
                        }
                    }
                }
            }
        });
    }

    void watchFor(const std::string& filename, std::function<void(const std::string&)> callback)
    {
        sys_stat st;
        const int result = stat(filename.c_str(), &st);
        if (0 == result)
        {
            Item item;
            item.name = filename;
            item.onModified = callback;
            item.lastModifiedTime = st.st_mtime;
            std::lock_guard<std::mutex> guard(itemListAccess);
            itemList.push_back(item);
        }
    }

    ~FileWatchdog()
    {
        watchThread.detach();
    }
};
