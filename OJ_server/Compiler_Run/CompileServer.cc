#include "CompileAndRun.hpp"
#include "../Comm/httplib.h"

using namespace ns_CompileAndRun;
using namespace httplib;

void Usage(const std::string proc)
{
    std::cerr<<"Uasge:"<<"\n\t"<<proc<<std::endl;
}

// ./CompileServer 端口号port
int main(int argc,char*argv[])
{
    if(argc!=2)
    {
        Usage(argv[0]);
        return 1;
    }

    Server svr;

    // svr.Get("/Hello",[](const Request &req, Response &resp){
    //     // 用来进行基本测试
    //     resp.set_content("hello httplib,你好 httplib!", "text/plain;charset=utf-8");
    // });

    svr.Post("/CompileAndRun", [](const Request &req, Response &resp){
        // 用户请求的服务正文是我们想要的json string
        std::string in_json = req.body;
        std::string out_json;
        if(!in_json.empty()){
            CompileAndRun::Start(in_json, &out_json);
            resp.set_content(out_json, "application/json;charset=utf-8");
        }
    });

    svr.listen("0.0.0.0",atoi(argv[1]));

    return 0;
}