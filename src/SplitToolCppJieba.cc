#include "SplitToolCppJieba.h"
#include <iostream>

std::unique_ptr<SplitToolCppJieba> SplitToolCppJieba::_instance = nullptr;
std::once_flag SplitToolCppJieba::_onceFlag;

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
SplitToolCppJieba* SplitToolCppJieba::getInstance() {
    std::call_once(_onceFlag, []{
        _instance.reset(new SplitToolCppJieba());
    });
    return _instance.get();
}

void SplitToolCppJieba::destroy() {
    _instance.reset(); // 析构里会清理 _jieba
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
            std::cerr << "Error: Failed to initialize Jieba object" << "\n";
            throw std::runtime_error("Jieba initialization failed");
        }
        
        _initialized = true;
        std::cout << "SplitToolCppJieba singleton initialized successfully" << "\n";
    }
}

std::vector<std::string> SplitToolCppJieba::cut(const std::string& sentence) {
    std::lock_guard<std::mutex> lock(_mutex); // 保险起见给 Cut 加锁
    if (!_jieba) return {};
    std::vector<std::string> words;
    _jieba->Cut(sentence, words, true);
    return words;
}