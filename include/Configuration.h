#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <memory>

class Configuration
{
public:
    // 获取单例实例的静态方法
    static Configuration* getInstance(const std::string& filepath = "");
    
    // 删除拷贝构造函数和赋值操作符
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
    
    // 析构函数
    ~Configuration();
    
    // 获取配置映射和停用词列表
    std::map<std::string, std::string>& getConfigMap();
    std::set<std::string>& getStopWordList();

private:
    // 私有构造函数，防止外部创建实例
    Configuration(const std::string& filepath);
    
    // 私有成员变量
    std::string _filepath;
    std::map<std::string, std::string> _configMap;
    std::set<std::string> _stopWordList;
    
    // 静态单例指针
    static Configuration* _instance;
};

#endif