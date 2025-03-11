#pragma once

#include <iostream>
#include <string>

#include "Utility.hpp"

namespace ns_Log
{
    using namespace ns_Util;

    enum LogLevel
    {
        Normal,
        Warnning,
        Error
    };

    std::ostream& LogDairy(const std::string& level,const std::string& FileName, const int line)
    {
        std::string message;
        
        message+='[';
        message+=level;
        message+=']';

        message+='[';
        std::string curTime;
        TimeUtil::GetCurTimeInDay(&curTime);
        message+=curTime;
        message+=']';

        message+='[';
        message+=FileName+'/';
        message+=std::to_string(line);
        message+=']';

        std::cout<<message;
        return std::cout;
    }

    //Log(level)<< "message"
    #define Log(level) LogDairy(#level,__FILE__,__LINE__)
} // namespace Log
