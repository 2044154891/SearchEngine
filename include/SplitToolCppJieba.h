/**
 * Project SearchEngine
 */


#ifndef _SPLITTOOLCPPJIEBA_H
#define _SPLITTOOLCPPJIEBA_H

#include "SplitTool.h"
#include "cppjieba/Jieba.hpp"
#include "NonCopyable.h"

#include <vector>
#include <string>
#include <mutex>
#include <memory>

using std::vector;
using std::string;


class SplitToolCppJieba: public SplitTool, public NonCopyable {
public: 
    static SplitToolCppJieba& getInstance();

    vector<string> cut(const string &sentence) override;

private:
    SplitToolCppJieba();
    ~SplitToolCppJieba() = default;

    static std::unique_ptr<cppjieba::Jieba> _jieba;
    static std::mutex _mutex;
    static bool _initialized;
};

#endif //_SPLITTOOLCPPJIEBA_H