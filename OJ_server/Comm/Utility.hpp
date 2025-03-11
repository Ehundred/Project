#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/time.h>
#include <sys/stat.h>
#include <mutex>
#include <atomic>

#include <boost/algorithm/string.hpp>

namespace ns_Util
{
    //单例模式基类
    class Singleton
    {
    protected:
        Singleton()
        {}

        Singleton(const Singleton& singleton) = delete;
        Singleton& operator=(const Singleton& singleton) =delete;
    };

    //时间工具
    class TimeUtil
    {
    public:
        static void GetCurTimeInYear(std::string *output)
        {
            if(output == nullptr)
                return;

            timeval out;
            gettimeofday(&out,nullptr);

            tm* curTime = localtime(&out.tv_sec);
            std::string str;
            str+=std::to_string(curTime->tm_year)+' ';
            str+=std::to_string(curTime->tm_mon)+' ';
            str+=std::to_string(curTime->tm_mday)+' ';
            str+=std::to_string(curTime->tm_hour)+':'+std::to_string(curTime->tm_min)+':'+std::to_string(curTime->tm_sec);

            *output = str;
        }

        static void GetCurTimeInDay(std::string *output)
        {
            if(output == nullptr)
                return;

            timeval out;
            gettimeofday(&out,nullptr);

            tm* curTime = localtime(&out.tv_sec);
            std::string str;
            str+=std::to_string(curTime->tm_hour)+':'+std::to_string(curTime->tm_min)+':'+std::to_string(curTime->tm_sec);

            *output = str;
        }

        static void GetCurTimeMs(std::string *output)
        {
            if(output == nullptr)
                return;

            timeval out;
            gettimeofday(&out,nullptr);

            *output = std::to_string(out.tv_usec);
        }
    };

    const std::string TempPath = "./temp/";

    //生成路径工具
    class PathUtil
    {
    private:
        static std::string AddSuffix(const std::string& fileName,const std::string& suffix,const std::string& filePath = TempPath)
        {
            return filePath + fileName + suffix;
        }
    public:
        static std::string GetSrcName(const std::string& fileName,const std::string& filePath = TempPath)
        {
            return AddSuffix(fileName,".cpp");
        }

        static std::string GetExeName(const std::string& fileName,const std::string& filePath = TempPath)
        {
            return AddSuffix(fileName,".exe");
        }

        static std::string GetStdinName(const std::string& fileName,const std::string& filePath = TempPath)
        {
            return AddSuffix(fileName,".stdin");        
        }

        static std::string GetStdoutName(const std::string& fileName,const std::string& filePath = TempPath)
        {
            return AddSuffix(fileName,".stdout");       
        }

        static std::string GetStderrName(const std::string& fileName,const std::string& filePath = TempPath)
        {
            return AddSuffix(fileName,".stderr");   
        }

        static std::string GetCompileErrorName(const std::string& fileName,const std::string& filePath = TempPath)
        {
            return AddSuffix(fileName,".compileError");
        }
    };

    class FileUtil
    {
    public:
        static bool IsFileExist(const std::string& FileName)
        {
            //通过获取文件属性的方法，来判断一个文件是否存在
            struct stat st;
            if(stat(FileName.c_str(),&st)==0)
            {
                return true;
            }

            return false;
        }

        static std::string MakeUniqueFileName()
        {
            //两种实现方式：
            //1. 加锁，然后在锁中去生成名字
            //2. 采用原子库的方式
            //不管采用哪种方式 都是为了保证同一件事——避免多线程的冲突
            
            //采用原子库的方法
            static std::atomic_uint id(0);
            id++;

            //用毫秒级的时间戳+原子库，确保每个名字是唯一的
            //可不可以只用时间戳或者只用原子库？不行。
            //因为，时间戳在同一毫秒中，也可能会有多个数据传来，这几个文件就是同名的。
            //而只用原子库，uint类型数据是有上限的，当数据溢出的时候也会导致重复
            //所以，时间戳+原子库，是为了保证，在一个毫秒内只有接收超过2^32的数据，才会导致重名
            std::string ms;
            TimeUtil::GetCurTimeMs(&ms);

            std::string uniqueId = std::to_string(id);

            return ms+'_'+uniqueId;
        }

        static bool WriteToFile(const std::string& fileName,const std::string& content)
        {
            std::ofstream outStream(fileName);
            if(!outStream.is_open())
            {
                return false;
            }

            outStream.write(content.c_str(),content.size());
            outStream.close();

            return true;
        }

        //keep 为是否保留换行
        static bool ReadFromFile(const std::string& fileName,std::string* content,bool keep = false)
        {
            content->clear();

            std::ifstream inStream(fileName);
            if(!inStream.is_open())
            {
                return false;
            }

            std::string buffer;
            while(std::getline(inStream,buffer))
            {
                *content += buffer;
                *content += keep?"\n":"";
            }

            return true;
        }
    };

    class StringUtil
    {
    public:
        //将输入的input字符串，分隔符为sep，切在output数组中
        static void SplitString(const std::string& input,std::vector<std::string>* output,const std::string& sep)
        {
            // 采用的是boost库中split的方法，参数is_any_of(sep)，即任何为sep的字符
            // boost::algorithm::token_compress_on，表示的是启用压缩
            // 如果不启用压缩，那么对于多个sep连在一起的情况，则无法正确切割，比如：
            // 一个字符串为："1aaa2",seq为a
            // 如果启用压缩，那么就会切为("1","2")
            // 如果不启用压缩，那么就会切为("1","","","","2")
            boost::split(*output,input,boost::is_any_of(sep),boost::algorithm::token_compress_on);
        }
    };
};