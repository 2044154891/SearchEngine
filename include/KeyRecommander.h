#pragma once

#include "SplitToolCppJieba.h"

#include <string>
#include <vector>
#include <set>
#include <queue>

// 候选结果条目
struct MyResult {
    std::string word;   // 候选词
    int frequency = 0;  // 词频
    int distance = 0;   // 与查询词的编辑距离
};

// 优先级队列比较器（使“最差在顶”）：
// 1) 编辑距离小 更好 => 小的排后（顶是差）
// 2) 距离相同 词频大 更好 => 大的排后
// 3) 再相同 字典序小 更好 => 小的排后
struct MyCompare {
    bool operator()(const MyResult &lhs, const MyResult &rhs) const {
        if (lhs.distance != rhs.distance) return lhs.distance < rhs.distance;
        if (lhs.frequency != rhs.frequency) return lhs.frequency > rhs.frequency;
        return lhs.word < rhs.word;
    }
};

class KeyRecommander {
public:
    explicit KeyRecommander(const std::string &query);

    // 执行查询主流程
    void execute();

    // 查询索引，得到候选集合（索引命中集的并集）
    void queryIndexTable();

    // 基于索引命中集计算候选（计算编辑距离并入队）
    void statistic(std::set<int> &iset);

    // 计算与查询词的最小编辑距离（需按 UTF-8 码点实现）
    int distance(const std::string &rhs) const;

    // 构造完整候选串（不发送）
    void buildSuggestions();
    const std::vector<std::string>& suggestions() const { return _fullSuggestions; }

    void sendFrame(const TcpConnectionPtr& conn, uint32_t msgId, const std::string& content);

private:
    std::string _originalQuery;                          // 原始完整查询
    std::string _queryWord;                              // 查询词（最后一个词/片段）
    std::priority_queue<MyResult, std::vector<MyResult>, MyCompare> _resultQue; // 候选队列
    std::vector<std::string> _fullSuggestions;           // 完整候选查询串
};


