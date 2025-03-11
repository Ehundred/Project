#pragma once

#include <string>

#include <jsoncpp/json/json.h>

#include "Compiler.hpp"
#include "Runner.hpp"

namespace ns_CompileAndRun
{
    using namespace ns_Compiler;
    using namespace ns_Runner;

    enum CompileAndRunState
    {
        CodeEmpty = -1,
        UnknownError = -2,
        CompileError = -3
    };

    class CompileAndRun
    {
    public:
        CompileAndRun()
        {
        }

        ~CompileAndRun()
        {
        }

    public:
        // CompileAndRun模块，负责的是将compiler和runner模块结合起来
        // 我们在compiler和runner模块中，经常会说到一个问题：假如存在一个文件xxx，那么凭什么是假如存在这个文件呢？
        // 很简单，在compiler和runner模块中，就是负责生成这些文件的。
        // 所谓的.cpp,.exe,...,他们都叫什么，叫作临时文件。正常来说，用户只会传入一个json串：包含用户提交的代码，和题目的基本信息等等
        // 当然，这也不是用户传进来的，这是我们内部通过json打包，然后传递给CompileAndRun的，这个json串是用于内部交流的。所以该json串一定是符合要求的
        // 那我们这里应该做些什么？
        // 1. 解析用户传入的json串，把字符数据转换为可以被使用的数据
        // 2. 提取json串中的代码，生成.cpp资源临时文件
        // 3. 交给compiler去编译
        // 4. 如果编译成功，则交给runner去运行，否则不运行直接跳过这一步
        // 5. 获取运行结果，即stdout，stderr等的输出。我们已经在runner中把输出重定向到了文件里，获取输出，即获取文件内容
        // 6. 把输出结果打包成json串，返回给用户
        static int Start(const std::string &inJson, std::string *outJson)
        {
            // 返回值的状态码
            int statusCode = 0;

            // 1. 解析用户传入的json串
            Json::Value inValue;
            Json::Reader reader;
            reader.parse(inJson, inValue);

            // inJson的结构为：
            /*****
             * inJson:
             * Code : 用户提交的代码
             * Input : 用户输入
             * CpuLimit : Cpu限制
             * MemoryLimit : 内存限制
             *****/
            std::string code = inValue["Code"].asString();
            std::string input = inValue["Input"].asString();
            int cpuLimit = inValue["CpuLimit"].asInt();
            int memoryLimit = inValue["MemoryLimit"].asInt();

            // 2.提取code来生成.cpp文件
            // 用户在传入的时候，是不会传入他的代码文件名的。或者说，文件名其实并不重要，也只有我们服务器内部才需要知道。
            // 所以，这个文件名我们可以随便取，只要保证，我们自己知道，我们自己可以使用，并且不会重复就可以了。

            std::string fileName = FileUtil::MakeUniqueFileName();
            bool compileStatus = true;
            int RunStatusCode = 0;

            std::string src = PathUtil::GetSrcName(fileName);
            FileUtil::WriteToFile(src, code);

            if (code.empty())
            {
                statusCode = CodeEmpty;
                goto END;
            }

            // 3. 交给compiler去编译
            compileStatus = Compiler::Compile(fileName);
            if (!compileStatus)
            {
                statusCode = CompileError;
                goto END;
            }

            // 4. 交给runner去运行
            RunStatusCode = Runner::Run(fileName, cpuLimit, memoryLimit);
            if (RunStatusCode < 0)
            {
                // 运行前崩溃
                statusCode = UnknownError;
            }
            else if (RunStatusCode > 0)
            {
                // 运行时崩溃
                statusCode = RunStatusCode;
            }
            else
            {
                // 运行成功
                statusCode = 0;
            }
        END:

            // 5. 获取运行结果
            std::string stdout;
            FileUtil::ReadFromFile(PathUtil::GetStdoutName(fileName), &stdout, true);

            std::string stderr;
            FileUtil::ReadFromFile(PathUtil::GetStderrName(fileName), &stderr, true);

            Json::Value outValue;
            /****
             * outValue:
             * Status : 状态码
             * Reason : 原因
             * Stdout : 标准输出
             * Stderr : 标准错误
             **** */
            outValue["Status"] = statusCode;
            outValue["Reason"] = StatusReason(statusCode, fileName);
            outValue["Stdout"] = stdout;
            outValue["Stderr"] = stderr;

            Json::StyledWriter writer;
            *outJson = writer.write(outValue);

            RemoveTempFile(fileName);

            return statusCode;
        }

    private:
        static std::string StatusReason(int code, const std::string &fileName)
        {
            std::string reason;
            std::string buffer;
            switch (code)
            {
            case 0:
                reason = "成功编译并运行";
                break;
            case CodeEmpty:
                reason = "提交的代码为空";
                break;
            case UnknownError:
                reason = "未知错误";
                break;
            case CompileError:
                FileUtil::ReadFromFile(PathUtil::GetCompileErrorName(fileName), &buffer);
                reason = "编译错误: \n" + buffer;
                break;
            case SIGABRT:
                reason = "内存超出范围";
                break;
            case SIGXCPU:
                reason = "时间超出范围";
                break;
            case SIGFPE:
                reason = "浮点数溢出";
                break;
            default:
                reason = "位置错误码" + std::to_string(code);
                break;
            }

            return reason;
        }

        static void RemoveTempFile(const std::string &fileName)
        {
            // 清理文件的个数是不确定的，但是有哪些我们是知道的
            std::string _src = PathUtil::GetSrcName(fileName);
            if (FileUtil::IsFileExist(_src))
                unlink(_src.c_str());

            std::string _compiler_error = PathUtil::GetCompileErrorName(fileName);
            if (FileUtil::IsFileExist(_compiler_error))
                unlink(_compiler_error.c_str());

            std::string _execute = PathUtil::GetExeName(fileName);
            if (FileUtil::IsFileExist(_execute))
                unlink(_execute.c_str());

            std::string _stdin = PathUtil::GetStdinName(fileName);
            if (FileUtil::IsFileExist(_stdin))
                unlink(_stdin.c_str());

            std::string _stdout = PathUtil::GetStdoutName(fileName);
            if (FileUtil::IsFileExist(_stdout))
                unlink(_stdout.c_str());

            std::string _stderr = PathUtil::GetStderrName(fileName);
            if (FileUtil::IsFileExist(_stderr))
                unlink(_stderr.c_str());
        }
    };

}
