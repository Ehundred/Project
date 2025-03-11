#pragma once

#include <iostream>
#include <string>

#include <ctemplate/template.h>

#include "OJ_model.hpp"
// #include "OJ_model2.hpp"

namespace ns_OJ_view
{
    using namespace ns_OJ_model;

    const std::string TemplatePath = "./template_html/";

    class View
    {
    public:
        View(){}
        ~View(){}
    public:
        void AllExpandHtml(const std::vector<struct Question> &questions, std::string *html)
        {
            // 题目的编号 题目的标题 题目的难度
            // 推荐使用表格显示
            // 1. 形成路径
            std::string src_html = TemplatePath + "AllQuestions.html";
            // 2. 形成数字典
            ctemplate::TemplateDictionary root("AllQuestions");
            for (const auto& q : questions)
            {
                ctemplate::TemplateDictionary *sub = root.AddSectionDictionary("QuestionList");
                sub->SetValue("id", q.id);
                sub->SetValue("title", q.title);
                sub->SetValue("star", q.star);
            }

            //3. 获取被渲染的html
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);

            //4. 开始完成渲染功能
            tpl->Expand(html, &root);
        }
        void OneExpandHtml(const struct Question &q, std::string *html)
        {
            // 1. 形成路径
            std::string src_html = TemplatePath + "OneQuestion.html";

            // 2. 形成数字典
            ctemplate::TemplateDictionary root("OneQuestion");
            root.SetValue("id", q.id);
            root.SetValue("title", q.title);
            root.SetValue("star", q.star);
            root.SetValue("description", q.description);
            root.SetValue("pre_code", q.header);

            //3. 获取被渲染的html
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);
           
            //4. 开始完成渲染功能
            tpl->Expand(html, &root);
        }
    };
}