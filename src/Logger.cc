#include <iostream>

#include "Logger.h"
#include "Timestamp.h"

// 获取log唯一的实例对象 单例
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置log级别
void Logger::setLogLevel(int level)
{
    logLevel_ = level;
}

// 写log [级别信息] time : msg
void Logger::log(std::string msg)
{
    std::string pre = "";
    switch (logLevel_)
    {
    case INFO:
        pre = "[INFO]";
        break;
    case ERROR:
        pre = "[ERROR]";
        break;
    case FATAL:
        pre = "[FATAL]";
        break;
    case DEBUG:
        pre = "[DEBUG]";
    default:
        break;
    }

    // 打印时间和msg
    std::cout << pre + Timestamp::now().toString() << ":" << msg << std::endl;
}
