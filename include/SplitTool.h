/**
 * Project SearchEngine
 */


#ifndef _SPLITTOOL_H
#define _SPLITTOOL_H

#include <string>
#include <vector>

class SplitTool {
public: 
    // 基类务必提供虚析构，避免通过基类指针删除派生类对象时出现未定义行为
    virtual ~SplitTool() = default;

    // 纯虚函数仅声明在头文件中即可
    virtual std::vector<std::string> cut(const std::string& sentence) = 0;
};

#endif //_SPLITTOOL_H