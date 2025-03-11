#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <fstream>

#include <jsoncpp/json/json.h>

#include "../Comm/Utility.hpp"
#include "../Comm/Log.hpp"

namespace ns_OJ_model
{
    using namespace ns_Log;
    using namespace ns_Util;

    struct Question
    {
        std::string id;          // 题目id
        std::string title;       // 题目标题
        std::string star;        // 题目难度
        int cpuLimit;            // cpu限制
        int memoryLimit;         // 内存限制
        std::string description; // 题目描述
        std::string header;      // 题目头，需要给用户的初始代码
        std::string tail;        // 题目尾，包含测试用例，用于和用户传入的代码拼接
    };

    const std::string QuestionList = "./questions/questions.list";
    const std::string QuestionPath = "./questions/";

    class Model
    {
    private:
        std::unordered_map<std::string, Question> questionsContainer;

    public:
        Model()
        {
            assert(LoadQuestionList(QuestionList));
        }
        ~Model()
        {
        }

    public:
        // 加载题目列表
        // 题目列表的格式是：
        //  题目ID 题目标题 题目难度 CPU限制  内存限制
        // 我们在加载的时候，采用的方法是一行一行读取，每一行都是一个题目的数据，一直读完整个文件，就是所有题目列表
        bool LoadQuestionList(const std::string &questionListFileName)
        {
            // 从传入的文件名中读取题目列表
            std::ifstream questionList(questionListFileName);
            if (!questionList.is_open())
            {
                Log(Error) << "未成功找到题目列表，请检查目录中是否存在文件" << questionListFileName << '\n';
                return false;
            }

            // 打开文件成功后，开始读取单个题目的数据，单个题目数据格式为：
            // id 标题 难度 时间限制 空间限制
            std::string buffer;
            while (std::getline(questionList, buffer))
            {
                std::vector<std::string> data;
                // 把数据切开 ———— 通过格式我们发现，分隔符为空格
                StringUtil::SplitString(buffer, &data, " ");

                if (data.size() != 5)
                {
                    Log(Error) << "读取题目格式错误，已跳过该题目" << '\n';
                    continue;
                }

                // 用数据生成结构体
                std::string questionId = data[0];
                std::string questionTitle = data[1];
                std::string questionStar = data[2];
                int questionCpuLimit = std::stoi(data[3].c_str());
                int questionMemoryLimit = std::stoi(data[4].c_str());

                // 读取了题目的部分信息后，进入具体题目的文件夹，去读取剩余的信息
                // 具体的题目文件夹名，与题目的ID相同
                // 在一个题目的文件夹内，会存在三个文件：
                // 1.desc.txt
                // 2.head.cpp
                // 3.tail.cpp
                // 这三个文件分别是什么？题目的描述，题目预设，题目尾
                // 为什么要单独建一个文件夹？如果全部放一个文件内，那么不说可读性差，不好分割等问题
                // 在以后，假如我们想只去加载一个题目，那么还要把所有文件全部读一遍，代价极
                std::string questionPath = QuestionPath + questionId + "/";

                std::string questionDescription;
                FileUtil::ReadFromFile(questionPath + "desc.txt", &questionDescription, true);
                std::string questionHead;
                FileUtil::ReadFromFile(questionPath + "header.cpp", &questionHead, true);
                std::string questionTail;
                FileUtil::ReadFromFile(questionPath + "tail.cpp", &questionTail, true);

                // 开始填充Question结构体，添加到哈希表中
                Question question;
                question.id = questionId;
                question.title = questionTitle;
                question.star = questionStar;
                question.cpuLimit = questionCpuLimit;
                question.memoryLimit = questionMemoryLimit;
                question.description = questionDescription;
                question.header = questionHead;
                question.tail = questionTail;

                questionsContainer.insert({questionId, question});
            }

            // 结束加载
            questionList.close();
            Log(Normal) << "加载题库成功！" << '\n';
            return true;
        }

        // 获取所有题目
        bool GetAllQuestions(std::vector<Question> *out)
        {
            if (out == nullptr)
                return false;

            // 如果题库容量为空，则表示要么题库没有被正常加载，要么因为网络问题，题库异常丢失
            if (questionsContainer.size() == 0)
            {
                Log(Warnning) << "用户题库获取失败！" << '\n';
                return false;
            }

            for (const auto &e : questionsContainer)
            {
                out->push_back(e.second);
            }

            return true;
        }

        // 获取单个题目
        bool GetOneQuestion(const std::string &id, Question *out)
        {
            if (out == nullptr)
                return false;

            const auto &iter = questionsContainer.find(id);
            if (iter == questionsContainer.end())
            {
                Log(Warnning) << "没有找到ID为" << "“" + id + "”" << "的题目" << '\n';
                return false;
            }

            *out = iter->second;
            return true;
        }
    };
}
