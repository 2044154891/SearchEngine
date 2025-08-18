#include "SplitToolCppJieba.h"
#include <iostream>


// 静态成员初始化
std::unique_ptr<cppjieba::Jieba> SplitToolCppJieba::_jieba = nullptr;
std::mutex SplitToolCppJieba::_mutex;
bool SplitToolCppJieba::_initialized = false;
// 修正词典文件路径 - 相对于项目根目录
// 使用绝对路径
const char* const DICT_PATH = "/home/zhang/Search_Engine/include/cppjieba/dict/jieba.dict.utf8";
const char* const HMM_PATH = "/home/zhang/Search_Engine/include/cppjieba/dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "/home/zhang/Search_Engine/include/cppjieba/dict/user.dict.utf8";
const char* const IDF_PATH = "/home/zhang/Search_Engine/include/cppjieba/dict/idf.utf8";
const char* const STOP_WORD_PATH = "/home/zhang/Search_Engine/include/cppjieba/dict/stop_words.utf8";

/**
 * SplitToolCppJieba implementation
 */
 SplitToolCppJieba& SplitToolCppJieba::getInstance() {
    static SplitToolCppJieba instance;
    return instance;
}

SplitToolCppJieba::SplitToolCppJieba() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (!_initialized) {
        // 初始化jieba对象
        _jieba = std::make_unique<cppjieba::Jieba>(
            DICT_PATH,           // 词典路径
            HMM_PATH,            // HMM模型路径
            USER_DICT_PATH,      // 用户词典路径
            IDF_PATH,            // IDF文件路径
            STOP_WORD_PATH       // 停用词路径
        );
        
        if (!_jieba) {
            std::cerr << "Error: Failed to initialize Jieba object" << std::endl;
            throw std::runtime_error("Jieba initialization failed");
        }
        
        _initialized = true;
        std::cout << "SplitToolCppJieba singleton initialized successfully" << std::endl;
    }
}

vector<string> SplitToolCppJieba::cut(const string& sentence) {
    if (!_jieba) {
        std::cerr << "Error: Jieba object is not initialized" << "\n";
        return {};
    }
    
    vector<string> words;
    try {
        // 使用jieba进行分词，true表示全模式分词
        _jieba->Cut(sentence, words, true);
    } catch (const std::exception& e) {
        std::cerr << "Error during word segmentation: " << e.what() << "\n";
        return {};
    }
    
    return words;
}