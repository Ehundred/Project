#pragma once

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../Comm/Utility.hpp"
#include "../Comm/Log.hpp"

namespace ns_Runner
{
    using namespace ns_Util;
    using namespace ns_Log;

    enum RunState
    {
        NoFileExist = -1,
        MakeFileError = -2,
    };

    class Runner
    {
    public:
        Runner()
        {}

        ~Runner()
        {}
    
    public:
        //在执行模块中，我们需要做什么？
        //执行模块，我们不需要考虑代码跑的是否正确，也不去考虑代码的运行结果怎么样，Run只做一件事情——把代码跑起来，把结果存起来
        //在一般代码的运行后，会有三个IO结果：标准输入，标准输出，标准错误。同样，我们并不希望去把这些结果输出在shell中，我们需要将其输入到文件里
        //这个时候又会生成三个临时文件：stdin，stdout，stderr
        //和compiler中一样，我们还是采取类似的方法：
        //1. 检查需要被执行的文件是否存在
        //2. 创建三个临时文件
        //3. 创建子进程。子进程用于执行文件，父进程等待子进程
        //4. 执行完毕
        static int Run(const std::string& FileName,int CpuLimit,int MemoryLimit)
        {
            std::string exe = PathUtil::GetExeName(FileName);
            std::string stdin = PathUtil::GetStdinName(FileName);
            std::string stdout = PathUtil::GetStdoutName(FileName);
            std::string stderr = PathUtil::GetStderrName(FileName);

            //1.检查文件是否存在
            if(!FileUtil::IsFileExist(exe))
            {
                Log(Warnning)<<"没有找到对应的可执行文件"<<'\n';
                return NoFileExist;
            }

            //2.生成临时文件
            umask(0);
            int _stdin = open(stdin.c_str(),O_CREAT|O_WRONLY,0644);
            if(_stdin<0)
            {
                Log(Warnning)<<"生成stdin文件失败"<<'\n';
                return MakeFileError;
            }

            int _stdout = open(stdout.c_str(),O_CREAT|O_WRONLY,0644);
            if(_stdout<0)
            {
                Log(Warnning)<<"生成stdout文件失败"<<'\n';
                return MakeFileError;
            }

            int _stderr = open(stderr.c_str(),O_CREAT|O_WRONLY,0644);
            if(_stderr<0)
            {
                Log(Warnning)<<"生成stdin文件失败"<<'\n';
                return MakeFileError;
            }

            //3.创建子进程
            pid_t pid = fork();
            //对子进程，执行可执行文件
            if(pid==0)
            {
                //1. 把输入输出重定向到文件里
                dup2(_stdin,0);
                dup2(_stdout,1);
                dup2(_stderr,2);

                //2. 设置系统资源
                SetProcLimit(CpuLimit,MemoryLimit);

                //3. 开始程序替换，执行传入的exe文件
                execlp(exe.c_str(),exe.c_str(),nullptr);

                //如果运行到这里，说明进程替换失败了
                Log(Error)<<"进程替换失败，未能成功运行"<<'\n';
                exit(2);
            }
            //对于父进程，父进程只需要等待子进程运行完成，就可以了
            else
            {
                //但是，等待之前，要记得关闭文件描述符！
                close(_stdin);
                close(_stdout);
                close(_stderr);

                int status = 0;
                waitpid(pid,&status,0);
                
                //不需要管status是什么状态，只需要把返回码完完整整打印出来就可以
                Log(Normal)<<"运行完毕，运行结果为："<<(status&0x7f)<<'\n';
                return status&0x7f;
            }
        }

    private:
        // 设置程序的资源
        // 因为只在Run中会用到，而且大部分为系统调用，所以直接封装为类中函数
        static void SetProcLimit(int CpuLimit,int MemoryLimit)
        {
            // 设置CPU时长
            struct rlimit cpu_rlimit;
            cpu_rlimit.rlim_max = RLIM_INFINITY;
            cpu_rlimit.rlim_cur = CpuLimit;
            setrlimit(RLIMIT_CPU, &cpu_rlimit);

            // 设置内存大小
            struct rlimit mem_rlimit;
            mem_rlimit.rlim_max = RLIM_INFINITY;
            mem_rlimit.rlim_cur = MemoryLimit * 1024; //转化成为KB
            setrlimit(RLIMIT_AS, &mem_rlimit);
        }
    };
}