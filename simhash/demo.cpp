#include <iostream>
#include <fstream>

//this define can avoid some logs which you don't need to care about.
#define LOGGER_LEVEL LL_WARN 

#include "simhash/Simhasher.hpp"
using namespace simhash;

#include "Deduplication.h"
#include <vector>
#include <string>

int main(int argc, char** argv)
{
    Simhasher simhasher("dict/jieba.dict.utf8", "dict/hmm_model.utf8", "dict/idf.utf8", "dict/stop_words.utf8");
    string s("我是蓝翔技工拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上总经理，出任CEO，走上人生巅峰。");
    size_t topN = 5;
    uint64_t u64 = 0;
    vector<pair<string ,double> > res;
    simhasher.extract(s, res, topN);
    simhasher.make(s, topN, u64);
    cout<< "simhash值是: " << u64<<endl;


    const char * bin1 = "100010110110";
    const char * bin2 = "110001110011";
    uint64_t u1, u2;
    u1 = Simhasher::binaryStringToUint64(bin1); 
    u2 = Simhasher::binaryStringToUint64(bin2); 
    
    cout<< bin1 << "和" << bin2 << " simhash值的相等判断如下："<<endl;
    cout<< "海明距离阈值默认设置为3，则isEqual结果为：" << (Simhasher::isEqual(u1, u2)) << endl; 
    cout<< "海明距离阈值默认设置为5，则isEqual结果为：" << (Simhasher::isEqual(u1, u2, 5)) << endl; 

    // ===== Deduplication demo: 给定多段文本，按海明距离<=3去重，输出保留下标 =====
    std::vector<std::string> texts = {
        "拖拉机学院手扶拖拉机专业，升职加薪，当上总经理。",
        "手扶拖拉机专业，升职加薪，当上总经理。",
        "今天阳光很好，适合出去散步。",
        "今天阳光真不错，出去散散步。"
    };
    std::vector<uint64_t> features;
    features.reserve(texts.size());
    for (const auto& t : texts) {
        uint64_t h = 0;
        simhasher.make(t, topN, h);
        features.push_back(h);
    }

    Deduplication deduper;
    std::vector<size_t> kept = deduper.deduplicateFeatures(features, 3);
    std::cout << "Kept indices after dedup (Hamming<=3):";
    for (size_t idx : kept) std::cout << ' ' << idx;
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
