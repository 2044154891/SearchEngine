#include "Lexicon.h"
#include "Configuration.h"
#include "Config.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Lexicon & Lexicon::instance() {
    static Lexicon inst;
    return inst;
}

Lexicon::Lexicon() {
    // 从 Configuration 单例读取路径并加载一次
    Configuration* cfg = Configuration::getInstance(Config::configFilePath);
    auto &mp = cfg->getConfigMap();
    auto itDict = mp.find("dictionary_path");
    auto itIndex = mp.find("dictionary_index_path");
    if (itDict == mp.end() || itIndex == mp.end()) {
        std::cerr << "Lexicon: dictionary_path or dictionary_index_path not set in config" << std::endl;
        return;
    }
    const std::string dictPath = itDict->second;
    const std::string indexPath = itIndex->second;

    LOG_INFO("%s:dictpath\n",dictPath.c_str());
    // 加载词典与索引（与之前 load 内容一致）
    std::ifstream df(dictPath);
    if (!df.is_open()) {
        std::cerr << "Lexicon: cannot open dict file: " << dictPath << "\n";
        return;
    }
    std::string line;
    while (std::getline(df, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string word; int freq = 0;
        if (iss >> word >> freq) {
            dict_.emplace_back(word, freq);
        }
    }
    df.close();

    if (dict_.empty()) {
        std::cerr << "Lexicon: dict file is empty: " << dictPath << "\n";
        return;
    }

    if (indexPath.empty()) {
        std::cerr << "Lexicon: index path is empty" << "\n";
        return;
    }

    std::ifstream inf(indexPath);
    if (!inf.is_open()) {
        std::cerr << "Lexicon: cannot open index file: " << indexPath << "\n";
        return;
    }
    while (std::getline(inf, line)) {
        if (line.empty()) continue;
        auto sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string sub = line.substr(0, sp);
        std::string rest = line.substr(sp + 1);
        for (char &c : rest) {
            if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-') c = ' ';
        }
        std::istringstream iss(rest);
        int id; std::vector<int> ids;
        while (iss >> id) ids.push_back(id);
        if (!ids.empty()) index_[sub] = std::move(ids);
    }
    inf.close();

    if (index_.empty()) {
        std::cerr << "Lexicon: index file is empty or invalid: " << indexPath << "\n";
        return;
    }

    loaded_.store(true);
}


const std::vector<int> * Lexicon::findPosting(const std::string &sub) const {
    auto it = index_.find(sub);
    if (it == index_.end()) return nullptr;
    return &it->second;
}

