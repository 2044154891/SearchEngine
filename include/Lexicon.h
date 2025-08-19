#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

// 只读词典/索引仓库：启动时一次性加载，后续多线程无锁访问
class Lexicon {
public:
    // 获取全局实例
    static Lexicon &instance();


    // 访问接口（只读）
    const std::vector<std::pair<std::string,int>> & dict() const { return dict_; }
    const std::unordered_map<std::string, std::vector<int>> & index() const { return index_; }

    // 根据子词（字母/UTF-8码点片段）获取候选下标列表；不存在则返回空指针
    const std::vector<int> * findPosting(const std::string &sub) const;


private:
    // 构造时自动加载（从 Configuration 单例读取路径）
    Lexicon();
    ~Lexicon() = default;
    Lexicon(const Lexicon&) = delete;
    Lexicon& operator=(const Lexicon&) = delete;


private:
    std::vector<std::pair<std::string,int>> dict_;
    std::unordered_map<std::string, std::vector<int>> index_;
    std::atomic<bool> loaded_{false};
};


