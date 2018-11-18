#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <functional>

class FileWatchdog
{
    struct Item
    {
        std::string name;
        std::function<void(const std::string&)> onModified;
        __time64_t lastModifiedTime;
    };

    std::thread watchThread;
    std::mutex itemListAccess;
    std::list<Item> itemList;

public:
    FileWatchdog(uint32_t pollFrequency)
    {
        watchThread = std::thread([this, pollFrequency]()
        {   // Dedicated thread that periodically checks files for modification
            while (true)
            {
                Sleep(pollFrequency);
                {
                    std::lock_guard<std::mutex> guard(itemListAccess);
                    for (Item& item : itemList)
                    {
                        struct _stat stat;
                        const int result = _stat(item.name.c_str(), &stat);
                        if (0 == result)
                        {
                            if (item.lastModifiedTime != stat.st_mtime)
                            {
                                item.onModified(item.name);
                                item.lastModifiedTime = stat.st_mtime;
                            }
                        }
                    }
                }
            }
        });
    }

    void watchFor(const std::string& filename, std::function<void(const std::string&)> callback)
    {
        struct _stat stat;
        const int result = _stat(filename.c_str(), &stat);
        if (0 == result)
        {
            Item item;
            item.name = filename;
            item.onModified = callback;
            item.lastModifiedTime = stat.st_mtime;
            std::lock_guard<std::mutex> guard(itemListAccess);
            itemList.push_back(item);
        }
    }

    ~FileWatchdog()
    {
        watchThread.detach();
    }
};
