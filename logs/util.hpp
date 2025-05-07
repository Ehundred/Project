#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cassert>
#include <sys/stat.h>

namespace log
{
    namespace util
    {
        class date
        {
        public:
            static size_t now() { return (size_t)time(nullptr); }
        };
        class file
        {
        public:
            static bool exists(const std::string &name)
            {
                struct stat st;
                return stat(name.c_str(), &st) == 0;
            }
            static std::string path(const std::string &name)
            {
                if (name.empty())
                    return ".";
                size_t pos = name.find_last_of("/\\");
                if (pos == std::string::npos)
                    return ".";
                return name.substr(0, pos + 1);
            }
            static void create_directory(const std::string &path)
            {
                if (path.empty())
                    return;
                if (exists(path))
                    return;
                size_t pos, idx = 0;
                while (idx < path.size())
                {
                    pos = path.find_first_of("/\\", idx);
                    if (pos == std::string::npos)
                    {
                        mkdir(path.c_str(), 0755);
                        return;
                    }
                    if (pos == idx)
                    {
                        idx = pos + 1;
                        continue;
                    }
                    std::string subdir = path.substr(0, pos);
                    if (subdir == "." || subdir == "..")
                    {
                        idx = pos + 1;
                        continue;
                    }
                    if (exists(subdir))
                    {
                        idx = pos + 1;
                        continue;
                    }
                    mkdir(subdir.c_str(), 0755);
                    idx = pos + 1;
                }
            }
        };
    }
}