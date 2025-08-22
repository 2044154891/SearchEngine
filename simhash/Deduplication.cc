#include "Deduplication.h"

#include <algorithm>

// 计算是否相似：海明距离 <= threshold
inline bool Deduplication::isSimilar(uint64_t a, uint64_t b, int threshold) {
    uint64_t x = a ^ b;
    // 早停：超过 threshold 则尽早返回
    int dist = 0;
    while (x != 0) {
        x &= (x - 1); // 去掉最低位的 1
        ++dist;
        if (dist > threshold) return false;
    }
    return dist <= threshold;
}

std::vector<size_t> Deduplication::deduplicateFeatures(const std::vector<uint64_t>& features,
                                                       int threshold) const {
    const size_t n = features.size();
    if (n <= 1) {
        std::vector<size_t> keep;
        keep.reserve(n);
        for (size_t i = 0; i < n; ++i) keep.push_back(i);
        return keep;
    }

    // 使用下标集合来模拟“遍历 set 并在其中删除”的语义，保持 O(k^2) 但避免拷贝
    std::vector<size_t> candidates;
    candidates.reserve(n);
    for (size_t i = 0; i < n; ++i) candidates.push_back(i);

    std::vector<size_t> kept;
    kept.reserve(n);

    for (size_t iPos = 0; iPos < candidates.size(); ++iPos) {
        const size_t i = candidates[iPos];
        // 保留第一个出现的作为代表
        kept.push_back(i);

        // 与之后的候选比较，如相似则从 candidates 中删除
        size_t writePos = iPos + 1;
        for (size_t jPos = iPos + 1; jPos < candidates.size(); ++jPos) {
            const size_t j = candidates[jPos];
            if (!isSimilar(features[i], features[j], threshold)) {
                if (writePos != jPos) candidates[writePos] = candidates[jPos];
                ++writePos;
            }
        }
        candidates.resize(writePos);
    }

    // 升序（按原出现顺序本已保证，如果需要可稳定排序）
    std::sort(kept.begin(), kept.end());
    return kept;
}