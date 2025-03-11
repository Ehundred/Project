#include <ctemplate/template.h>
#include <string>
#include <iostream>
 
int main(){
  // 形成数据字典
  ctemplate::TemplateDictionary dic("test");
  dic.SetValue("name", "张三");                // 相当于插入了一个键值对（name会在下面的网页模板中出现）
 
  // 构建空网页模板对象
  std::string empty_html = "./test.html";     // 空的网页模板
  ctemplate::Template* tp = ctemplate::Template::GetTemplate(empty_html, ctemplate::DO_NOT_STRIP);
  
  // 渲染网页模板（将网页中的变量 name 替换成 "张三"）
  std::string filled_html;
  tp->Expand(&filled_html, &dic);
 
  std::cout << filled_html << std::endl;
  return 0;
}