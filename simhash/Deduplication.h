// NOLINTBEGIN
#pragma once

#define LOGGER_LEVEL LL_WARN

#include "simhash/Simhasher.hpp"

#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>

// 该类聚焦去重逻辑：给定一组 simhash 特征，基于海明距离阈值进行去重
class Deduplication {
public:
    Deduplication() = default;
    ~Deduplication() = default;

    // 基于海明距离阈值进行去重，返回保留的下标集合（稳定顺序：按原始下标升序）
    // threshold 默认为 3
    std::vector<size_t> deduplicateFeatures(const std::vector<uint64_t>& features,
                                            int threshold = 3) const;

private:
    // 判断两特征是否相似（海明距离 <= threshold）
    static inline bool isSimilar(uint64_t a, uint64_t b, int threshold);
};
// NOLINTEND
