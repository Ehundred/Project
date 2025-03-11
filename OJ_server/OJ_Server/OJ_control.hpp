#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <cassert>

#include <jsoncpp/json/json.h>

#include "../Comm/httplib.h"
#include "../Comm/Log.hpp"
#include "../Comm/Utility.hpp"
#include "OJ_model.hpp"
#include "OJ_view.hpp"

namespace ns_OJ_control
{
    using namespace ns_OJ_model;
    using namespace ns_OJ_view;
    using namespace ns_Log;
    using namespace ns_Util;
    using namespace httplib;

    // 进行服务的主机
    class Machine
    {
    public:
        std::string ip;   // 主机ip
        int port;         // 主机服务端口
        uint64_t load;    // 主机负载
        std::mutex *lock; // 主机锁
    public:
        Machine()
            : ip(""), port(0), load(0), lock(nullptr)
        {
        }
        ~Machine()
        {
        }

    public:
        // 增加主机负载
        void IncreaseLoad()
        {
            if (lock)
                lock->lock();
            load++;
            if (lock)
                lock->unlock();
        }

        // 减少主机负载
        void DecreaseLoad()
        {
            if (lock)
                lock->lock();
            load--;
            if (lock)
                lock->unlock();
        }

        // 获取主机负载
        uint64_t Load()
        {
            if (lock)
                lock->lock();
            int ans = load;
            if (lock)
                lock->unlock();

            return ans;
        }

        // 重置主机负载
        void ResetLoad()
        {
            if (lock)
                lock->lock();
            load = 0;
            if (lock)
                lock->unlock();
        }
    };

    const std::string ServerMachineConfigure = "./conf/ServerMachine.conf";

    // 负责管理所有的主机，让主机的负载均衡
    class LoadBlance
    {
    private:
        // 在MachinesContainer里面，存储着所有要被用于OJ服务的主机，每个主机都有着自己的下标
        // 为什么不用哈希表？因为主机是固定的，一般来说，一个服务器一个主机，一旦被用于了OJ服务，那么就不会再发生变化了
        // 这个时候，我们采用数组的方式，效率要比哈希表容器要高，但是本质还是哈希的思想，仅此而已
        std::vector<Machine> MachinesContainer;
        // 用数组的下标，来表示主机
        std::vector<int> onlineMachine;  // 在线的主机
        std::vector<int> offlineMachine; // 离线的主机

        std::mutex *lock;

    public:
        LoadBlance()
        {
            lock = new std::mutex();
            assert(LoadConfigure(ServerMachineConfigure));
            Log(Normal) << "加载" << ServerMachineConfigure << "成功！" << '\n';
        }
        ~LoadBlance()
        {
        }

    public:
        // 读取所有主机列表
        // 和题目列表的读取一样，我们也是一行一行读取，题目的数据格式为：
        //  IP:Port
        bool LoadConfigure(const std::string &configurePath)
        {
            std::ifstream machineConfigure(configurePath);
            if (!machineConfigure.is_open())
            {
                Log(Normal) << "打开服务主机目录失败！" << '\n';
                return false;
            }

            // 打开成功之后，和题目列表的读取一样，读取所有主机的数据，然后添加到container中
            // 主机的数据格式为： IP:Port
            std::string buffer;
            while (std::getline(machineConfigure, buffer))
            {
                std::vector<std::string> data;
                // 切分字符串，提取到IP和Port
                StringUtil::SplitString(buffer, &data, ":");

                // 正常来说，切分出来的数据只有两个元素：IP和port
                if (data.size() != 2)
                {
                    Log(Warnning) << "读取主机数据失败，已跳过该主机" << '\n';
                    continue;
                }

                std::string machineIP = data[0];
                std::string machinePort = data[1];

                // 读取完毕后，生成结构体，添加到Container中
                Machine machine;
                machine.ip = machineIP;
                machine.port = std::atoi(machinePort.c_str());
                machine.load = 0;
                machine.lock = new std::mutex();

                // 当主机被加入时，默认添加到在线主机中
                onlineMachine.push_back(MachinesContainer.size());
                MachinesContainer.push_back(machine);
            }

            machineConfigure.close();
            Log(Normal) << "所有主机已加载完毕！" << '\n';
            return true;
        }

        // 采用负载均衡的原则，选择最佳的主机
        // 为什么要用二重指针？因为一个指针表示的是，这是一个输出参数，而另一个指针，表示我们想返回的是一个机器的指针参数
        bool SmartChoice(int *ID, Machine **machine)
        {
            lock->lock();

            // 负载均衡的算法——遍历所有主机，找到负载最小的那一个
            // 1. 首先判断是否有在线的主机
            if (onlineMachine.size() == 0)
            {
                Log(Normal) << "所有主机都已下线 请维护服务器" << '\n';
                lock->unlock();
                return false;
            }

            // 2. 从在线主机数组下标0开始，依次往后遍历所有在线的主机
            Machine* minLoadMachine = &MachinesContainer[onlineMachine[0]];
            int minLoadMachineID = onlineMachine[0];

            for (int i = 1; i < onlineMachine.size(); i++)
            {
                Machine* curMachine = &MachinesContainer[onlineMachine[i]];
                if (minLoadMachine->load > curMachine->load)
                {
                    minLoadMachineID = i;
                    minLoadMachine = curMachine;
                }
            }

            // 3. 遍历完所有之后，找到了最小负载的机器，赋值给输出型参数
            *ID = minLoadMachineID;
            *machine = minLoadMachine;

            lock->unlock();

            return true;
        }

        //离线一个主机
        void OffLineMachine(int MachineID)
        {
            lock->lock();

            // 通过遍历所有数组的方式，找到需要被找到的主机
            for (auto iter = onlineMachine.begin(); iter < onlineMachine.end(); iter++)
            {
                if (*iter == MachineID)
                {
                    MachinesContainer[MachineID].ResetLoad();
                    onlineMachine.erase(iter);
                    offlineMachine.push_back(MachineID);
                    break;
                }
            }

            lock->unlock();
        }

        void OnlineMachine()
        {
            // 我们统⼀上线，后⾯统⼀解决
            // TODO
        }

        //显示所有主机，为了做测试才用的函数
        void ShowMachines()
        {
            lock->lock();

            std::cout<<"当前在线主机的列表："<<std::endl;
            for(auto& id : onlineMachine)
            {
                std::cout<<id<<" ";
            }
            std::cout<<std::endl;

            std::cout<<"当前离线主机的列表："<<std::endl;
            for(auto& id : offlineMachine)
            {
                std::cout<<id<<" ";
            }
            std::cout<<std::endl;

            lock->unlock();
        }
    };

    class Control
    {
    private:
        Model _model;
        View _view;
        LoadBlance _loadBlance;
    public:
        Control()
        {}
        ~Control()
        {}
    public:
        //获取所有题目的页面
        bool AllQuestion(std::string* html)
        {
            std::vector<Question> questions;
            bool isGetAllQuestions = _model.GetAllQuestions(&questions);
            if(!isGetAllQuestions)
            {
                Log(Error)<<"获取题目列表失败！"<<'\n';
                return false;
            }

            _view.AllExpandHtml(questions,html);
            return true;
        }

        //获取单个题目的页面
        bool GetOneQuestion(const std::string& questionNumber,std::string* html)
        {
            Question question;
            bool isGetOneQuestion = _model.GetOneQuestion(questionNumber,&question);
            if(!isGetOneQuestion)
            {
                Log(Error)<<"题目： "<<questionNumber<<" 不存在！"<<'\n';
                return false;
            }

            _view.OneExpandHtml(question,html);
            return true;
        }

        // 判题模块
        // 判题模块，我们需要干什么？
        // 首先，我们需要把Compiler&Run部分的模块引入进来吗？有人可能会说，不引入怎么判题呢？
        // 其实，这两个模块是分开的，他们形成的是两个服务。他们的关系可以理解为：
        // OJ_Server：包含主机 ComplieServer,ComplieServer2,...
        // 所以，我们实际上要进行的请求，是向另一个端口发送服务请求，我们这个模块只负责自己的一部分，不属于自己的直接丢给其他模块等结果
        // 其次，在Control中，我们需要做的，就是找到合适的主机，然后向该主机发送请求，等待该主机编译并运行完，然后我们返回给用户结果，仅此而已
        // 1.根据题目编号，找到题目
        // 2.读取inJson的数据，形成编译所需要的CompileJson串
            /*****
             * CompileJson:
             * Code : 用户提交的代码
             * Input : 用户输入
             * CpuLimit : Cpu限制
             * MemoryLimit : 内存限制
             *****/
        // 3.找到负载最低的主机
        // 4.向主机发送请求，得到结果
        void Judge(const std::string& questionNumber,const std::string& inJson,std::string* outJson)
        {
            // 1.根据题目编号，找到题目
            Question question;
            if(!_model.GetOneQuestion(questionNumber,&question))
            {
                Log(Error)<<"需要被判题的题目不存在！"<<"题目ID： "<<questionNumber<<'\n';
                return;
            }

            // 2.根据inJson的数据，形式编译所需要的compileJson串
            // 2.1. 读取inJson中的数据
            Json::Value inValue;
            Json::Reader reader;
            reader.parse(inJson,inValue);

            std::string code = inValue["Code"].asString();
            std::string input = inValue["Input"].asString();

            // 2.2. 形成compileJson串
            Json::Value compileValue;

            compileValue["Code"] = code+"\n"+question.tail;//编译所需要的代码，由用户写的代码和包含测试用例与主函数的tail拼接而成
            compileValue["Input"] = input;
            compileValue["CpuLimit"] = question.cpuLimit;
            compileValue["MemoryLimit"] = question.memoryLimit;

            Json::FastWriter writer;
            std::string compileJson = writer.write(compileValue);

            // 3.找到负载最小的主机
            // 这里会产生一个问题——当我们找到了负载最小的主机，然后这个主机突然下线了，怎么办？
            // 如果我们不去管，只去发请求而不检查回复的可靠性，那么有可能会因为这个问题导致无法正确判题
            // 所以在这里，我们采取的方式是——循环。不断找到负载最小的主机，向其发送请求，一直到收到了正确的回复为止
            while(true)
            {
                int machineID = 0;
                Machine* machine = nullptr;
                if(!_loadBlance.SmartChoice(&machineID,&machine))
                {
                    //如果没有找到合适的主机，那么说明服务器挂了，服务也没必要进行了
                    Log(Normal)<<"所有主机都已经离线"<<'\n';
                    return;
                }

                // 4. 找到主机后，向主机发送请求
                Client client(machine->ip,machine->port);
                machine->IncreaseLoad();
                Log(Normal)<<"选择主机成功，主机号为： "<<machineID<<'\n';
                auto response = client.Post("/CompileAndRun",compileJson,"application/json;charset=utf-8");

                // 如果有应答
                if(response)
                {   
                    // 如果应答的状态码为200，则表示正常，从response中获取outJson
                    if(response->status==200)
                    {
                        *outJson = response->body;
                        machine->DecreaseLoad();
                        Log(Normal)<<"编译和运行服务成功！"<<'\n';
                        break;
                    }
                    
                    machine->DecreaseLoad();
                }
                // 如果没有应答，则表示主机已经离线，重新找其他主机
                else
                {
                    Log(Warnning)<<"请求的主机"<<machineID<<"已经离线，尝试请求其他主机"<<'\n';
                    _loadBlance.OffLineMachine(machineID);
                    //为了测试
                    _loadBlance.ShowMachines();
                }

            }
        }
    };
}