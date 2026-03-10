#pragma once

#include <string>

#include "noncopyable.h"

// 使用glog日志库
#include <glog/logging.h>
#include <glog/log_severity.h>
#include <sstream>
#include <cstdarg>

using namespace google;

// 辅助函数：将printf格式字符串转换为流式输出
inline std::string formatString(const char* format) {
    return std::string(format);
}

template<typename... Args>
std::string formatString(const char* format, Args... args) {
    char buf[1024];
    snprintf(buf, sizeof(buf), format, args...);
    return std::string(buf);
}

// 使用流式输出的兼容宏 - 将printf格式转换为流式
// 注意：glog没有DEBUG级别，使用WARNING代替
#define INFO LOG(INFO) << formatString
#define ERROR LOG(ERROR) << formatString
#define FATAL LOG(FATAL) << formatString
#define WARNING LOG(WARNING) << formatString

// 初始化glog (在main函数开始时调用)
#define INIT_GLOG(argc, argv) \
    google::InitGoogleLogging(argv[0]); \
    google::SetLogDestination(google::INFO, "log/info_"); \
    google::SetLogDestination(google::WARNING, "log/warning_"); \
    google::SetLogDestination(google::ERROR, "log/error_"); \
    google::SetLogDestination(google::FATAL, "log/fatal_"); \
    FLAGS_logtostderr = false

// 关闭glog
#define SHUTDOWN_GLOG google::ShutdownGoogleLogging()
