#include "Configuration.h"

// 静态成员变量初始化
Configuration* Configuration::_instance = nullptr;

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return ""; // 如果没有找到非空格字符，返回空字符串
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

Configuration* Configuration::getInstance(const std::string& filepath) {
    if (_instance == nullptr) {
        _instance = new Configuration(filepath);
    }
    return _instance;
}

Configuration::Configuration(const std::string& filepath) {
    _filepath = filepath;
    _configMap.clear();
    _stopWordList.clear();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_FATAL("can't open conf file, pleaser check the path%s /n",filepath.c_str());
        return;
    }
    std::string line;

    // 正则表达式，用于匹配 key = value
    std::regex pattern(R"((\w[\w\s/_-]*)\s*=\s*(.*))");

    // 逐行读取文件
    while (std::getline(file, line)) {
        std::smatch matches;

        // 如果匹配成功，提取 key 和 value
        if (std::regex_search(line, matches, pattern)) {
            std::string key = trim(matches[1].str());  // 去掉 key 两端的空格
            std::string value = trim(matches[2].str());  // 去掉 value 两端的空格

            // 存储到 map 中
            _configMap[key] = value;
        }
    }
    file.close();
    // 加载停用词表
    std::ifstream stopWordFilechinese(_configMap["chinese_stop_words"]);
    if (!stopWordFilechinese.is_open()) {
        std::cerr << "无法打开停用词文件: " << _configMap["chinese_stop_words"] << std::endl;
        return;
    }
    line.clear();
    while (std::getline(stopWordFilechinese, line)) {
        _stopWordList.insert(line);
    }
    stopWordFilechinese.close();
    // 加载停用词表
    std::ifstream stopWordFileenglish(_configMap["english_stop_words"]);
    if (!stopWordFileenglish.is_open()) {
        std::cerr << "无法打开停用词文件: " << _configMap["english_stop_words"] << std::endl;
        return;
    }
    line.clear();
    while (std::getline(stopWordFileenglish, line)) {
        _stopWordList.insert(line);
    }
    stopWordFileenglish.close();
    _stopWordList.insert(" ");
    _stopWordList.insert("\n");
}

Configuration::~Configuration() {
    // 清理静态实例
    if (_instance != nullptr) {
        delete _instance;
        _instance = nullptr;
    }
}

std::map<std::string, std::string>& Configuration::getConfigMap() {
    return _configMap;
}

std::set<std::string>& Configuration::getStopWordList() {
    return _stopWordList;
}