#include <iostream>
#include "../Comm/httplib.h"
#include "OJ_control.hpp"

using namespace httplib;
using namespace ns_OJ_control;

int main()
{
    Server svr;

    Control control;

    svr.Get("/AllQuestions",[&control](const Request& req,Response& resp)
    {
        std::string html;
        control.AllQuestion(&html);
        resp.set_content(html,"text/html;charset=utf-8");
    });

    svr.Get(R"(/Question/(\d+))",[&control](const Request& req,Response& resp)
    {
        std::string number = req.matches[1];
        std::string html;
        control.GetOneQuestion(number,&html);
        resp.set_content(html,"text/html;charset=utf-8");
    });

    svr.Post(R"(/Judge/(\d+))",[&control](const Request& req,Response& resp)
    {
        std::string number = req.matches[1];
        std::string respJson;

        control.Judge(number,req.body,&respJson);
        resp.set_content(respJson,"application/json;charset=utf-8");
    });

    svr.set_base_dir("./wwwroot");
    svr.listen("0.0.0.0",8888);

    return 0;
}