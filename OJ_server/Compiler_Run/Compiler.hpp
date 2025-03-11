#pragma once

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include "../Comm/Utility.hpp"
#include "../Comm/Log.hpp"

namespace ns_Compiler
{
    using namespace ns_Util;
    using namespace ns_Log;

    // 编译模块，主要负责代码的编译，不管运行
    class Compiler
    {
    public:
        Compiler()
        {
        }

        ~Compiler()
        {
        }

    public:
        // 我们假设，temp中已有一个源文件叫test.cpp
        // 在Compile函数中，我们只负责编译传入的filename的文件，我们不去管这个文件是否存在，也不管文件路径是否正确，那都是其他模块去做的事情
        // 在编译中，我们会产生两个文件——生成的可执行test.exe,如果编译错误的标准错误输出stderr
        // 而为了和run生成的stderr相区分，我们把compile生成的stderr命名为compileError
        static bool Compile(const std::string &FileName)
        {
            // 如果我们想编译temp中的test.cpp文件，我们只需要传入的FileName为./temp/test
            std::string src = PathUtil::GetSrcName(FileName);
            // 生成exe和标准错误的文件名
            std::string stderr = PathUtil::GetCompileErrorName(FileName);
            std::string exe = PathUtil::GetExeName(FileName);

            // 开始进行编译
            // 首先创建子进程，让子进程做程序替换来编译
            pid_t pid = fork();
            // 首先子进程应该做的事情——进程替换，替换为g++来编译
            if (pid == 0)
            {
                // 我们希望，stderr输出到文件里，所以我们应该干什么？
                // 1. 创建stderr文件
                umask(0);
                int _stderr = open(stderr.c_str(), O_CREAT | O_WRONLY, 0644);
                if (_stderr < 0)
                {
                    Log(Error) << "没有成功生成stderr文件" << "\n";
                    exit(1);
                }

                // 2. 输出重定向,让标准错误输出到stderr文件中
                dup2(_stderr,2);

                // 3. 通过进程替换，开始编译
                execlp("g++", "g++", "-o", exe.c_str(), src.c_str(),"-D", "COMPILER_ONLINE","-std=c++11",nullptr);

                // 如果运行到了这里，说明进程替换失败了
                Log(Error) << "进程替换失败，编译器没有成功启动" << '\n';
                exit(2);
            }
            // 再来看父进程，父进程要干什么？
            // 父进程只需要等待子进程跑完，然后检查是否成功编译就可以了
            // 但是如何检查是否成功编译？最简单的方法——看是否存在exe文件
            else
            {
                //等待子进程编译完全
                waitpid(pid, nullptr, 0);

                //等待完之后，检查是否生成了可执行文件
                if(!FileUtil::IsFileExist(exe))
                {
                    Log(Normal)<<"生成可执行程序失败"<<'\n';
                    return false;
                }
            }

            Log(Normal)<<"编译成功，可执行程序： "<<exe<<'\n';
            return true;
        }
    };
}