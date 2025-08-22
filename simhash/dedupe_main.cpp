#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <tuple>
#include <cstdint>
#include <algorithm>

#define LOGGER_LEVEL LL_WARN

#include "simhash/Simhasher.hpp"
#include "Deduplication.h"
#include "../include/Config.h"
#include "../include/FileUtils.h"

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

struct OffsetRecord {
    int docId;
    std::streamoff offset;
    size_t length;
};

static bool readOffsets(const string& offsetPath, vector<OffsetRecord>& records) {
    ifstream in(offsetPath);
    if (!in) {
        cerr << "Error: cannot open offset file: " << offsetPath << "\n";
        return false;
    }
    records.clear();
    int id = 0;
    long long off = 0;
    long long len = 0;
    while (in >> id >> off >> len) {
        records.push_back(OffsetRecord{ id, static_cast<std::streamoff>(off), static_cast<size_t>(len) });
    }
    return true;
}

static bool readBlock(ifstream& dataIn, std::streamoff offset, size_t length, string& out) {
    out.clear();
    out.resize(length);
    dataIn.clear();
    dataIn.seekg(offset, std::ios::beg);
    if (!dataIn.good()) return false;
    dataIn.read(&out[0], static_cast<std::streamsize>(length));
    return static_cast<size_t>(dataIn.gcount()) == length;
}

int main(int argc, char** argv) {
    // 读取配置
    const string conf = Config::configFilePath;
    const string oldPagePath  = read_config_value(conf, "old_webpage_path");
    const string oldIndexPath = read_config_value(conf, "old_webpage_offset_path");
    const string newPagePath  = read_config_value(conf, "new_webpage_path");
    const string newIndexPath = read_config_value(conf, "new_webpage_offset_path");
    if (oldPagePath.empty() || oldIndexPath.empty() || newPagePath.empty() || newIndexPath.empty()) {
        cerr << "Error: missing required paths in config: " << conf << "\n";
        return 1;
    }

    // 读取旧索引
    vector<OffsetRecord> records;
    if (!readOffsets(oldIndexPath, records)) return 1;
    if (records.empty()) {
        cerr << "Warning: no records found in offset file: " << oldIndexPath << "\n";
        return 0;
    }

    // 打开旧网页库
    ifstream dataIn(oldPagePath, std::ios::in | std::ios::binary);
    if (!dataIn) {
        cerr << "Error: cannot open data file: " << oldPagePath << "\n";
        return 1;
    }

    // 初始化 simhash
    // 运行目录为 simhash/ 时，词典相对路径应为 dict/*
    simhash::Simhasher simhasher(
        "dict/jieba.dict.utf8",
        "dict/hmm_model.utf8",
        "dict/idf.utf8",
        "dict/stop_words.utf8"
    );
    const size_t topN = 5;

    // 第一遍：计算所有文档的 simhash 特征（仅保留特征，避免把全文常驻内存）
    vector<uint64_t> features;
    features.reserve(records.size());

    {
        string block;
        block.reserve(1 << 16);
        for (const auto& rec : records) {
            if (!readBlock(dataIn, rec.offset, rec.length, block)) {
                cerr << "Error: failed to read block for docId=" << rec.docId << " offset=" << rec.offset << " length=" << rec.length << "\n";
                return 1;
            }

            uint64_t h = 0;
            simhasher.make(block, topN, h);
            features.push_back(h);
        }
    }

    // 去重（海明距离<=3）：遍历原集合，重复的从 keptDocIds 中删除
    std::unordered_set<int> keptDocIds;
    keptDocIds.reserve(records.size() * 2);
    for (const auto& rec : records) keptDocIds.insert(rec.docId);

    auto isSimilar = [](uint64_t a, uint64_t b, int threshold) {
        uint64_t x = a ^ b;
        int dist = 0;
        while (x) {
            x &= (x - 1);
            if (++dist > threshold) return false;
        }
        return dist <= threshold;
    };

    const int threshold = 3;
    for (size_t i = 0; i < records.size(); ++i) {
        if (keptDocIds.find(records[i].docId) == keptDocIds.end()) continue;
        for (size_t j = i + 1; j < records.size(); ++j) {
            if (keptDocIds.find(records[j].docId) == keptDocIds.end()) continue;
            if (isSimilar(features[i], features[j], threshold)) {
                keptDocIds.erase(records[j].docId);
            }
        }
    }

    // 写入新网页库与新索引（保持原 docid，不 renumber）
    ofstream newData(newPagePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!newData) {
        cerr << "Error: cannot open new data file for write: " << newPagePath << "\n";
        return 1;
    }
    ofstream newIdx(newIndexPath, std::ios::out | std::ios::trunc);
    if (!newIdx) {
        cerr << "Error: cannot open new index file for write: " << newIndexPath << "\n";
        return 1;
    }

    // 第二遍：只拷贝保留的文档块到新网页库，同时写新偏移
    dataIn.clear();
    dataIn.seekg(0, std::ios::beg);
    std::streamoff currentOffset = 0;
    string block;
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& rec = records[i];
        if (keptDocIds.find(rec.docId) == keptDocIds.end()) continue;
        if (!readBlock(dataIn, rec.offset, rec.length, block)) {
            cerr << "Error: failed to re-read block for docId=" << rec.docId << "\n";
            return 1;
        }
        newData.write(block.data(), static_cast<std::streamsize>(block.size()));
        newIdx << rec.docId << ' ' << currentOffset << ' ' << block.size() << '\n';
        currentOffset += static_cast<std::streamoff>(block.size());
    }

    cout << "Dedup finished. Kept " << keptIdx.size() << " of " << records.size()
         << ". New data: " << newPagePath << ", new index: " << newIndexPath << endl;
    return 0;
}


