#include "Configuration.h"

// 静态成员变量初始化
Configuration* Configuration::_instance = nullptr;

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
        std::cerr << "无法打开配置文件: " << filepath << "\n";
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // 处理配置项
        std::istringstream iss(line);
        std::string key, value;
        std::getline(iss, key, '=');
        std::getline(iss, value);
        key.erase(key.end() - 1);
        value.erase(value.begin());
        if (!key.empty() && !value.empty()) {
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