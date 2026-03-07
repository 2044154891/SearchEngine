#include "PageLibPreprocessor.h"
#include "Config.h"
#include "FileUtils.h"

#include <unordered_set>
#include <map>
#include <fstream>

// 离线阶段：本文件不依赖在线 Configuration 单例
// 维护一次性的停用词集合
static std::unordered_set<std::string> g_stopWords;

static void loadStopWordsOnce() {
    if (!g_stopWords.empty()) return;
    const std::string conf = Config::configFilePath;
    const std::string zhPath = read_config_value(conf, "chinese_stop_words");
    const std::string enPath = read_config_value(conf, "english_stop_words");

    auto loadFile = [](const std::string& path) {
        if (path.empty()) return;
        std::ifstream in(path);
        if (!in.is_open()) return;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) g_stopWords.insert(line);
        }
    };
    loadFile(zhPath);
    loadFile(enPath);
    g_stopWords.insert(" ");
    g_stopWords.insert("\n");
}

PageLibPreprocessor::PageLibPreprocessor() {
}

PageLibPreprocessor::~PageLibPreprocessor() {
}

void PageLibPreprocessor::initPathsFromConfig() {
    const std::string conf = Config::configFilePath;
    _newPagePath      = read_config_value(conf, "new_webpage_path");
    _newIndexPath     = read_config_value(conf, "new_webpage_offset_path");
    _invertIndexPath  = read_config_value(conf, "invertindex_path");
    _wordFreqPath     = read_config_value(conf, "wordfrequence_path");
    _wordInPagePath   = read_config_value(conf, "wordinpage_path");
}

void PageLibPreprocessor::load() {
    initPathsFromConfig();
    loadStopWordsOnce();
    if (_newPagePath.empty() || _newIndexPath.empty()) {
        std::cerr << "PageLibPreprocessor: missing new page/index paths" << "\n";
        return;
    }

    std::ifstream idxIn(_newIndexPath);
    if (!idxIn.is_open()) {
        std::cerr << "PageLibPreprocessor: cannot open index file: " << _newIndexPath << "\n";
        return;
    }
    std::ifstream pageIn(_newPagePath, std::ios::binary);
    if (!pageIn.is_open()) {
        std::cerr << "PageLibPreprocessor: cannot open page file: " << _newPagePath << "\n";
        return;
    }

    int docId = 0; long long off = 0; long long len = 0;
    _totalDocs = 0;

    while (idxIn >> docId >> off >> len) {
        if (len <= 0) continue;
        std::string block;
        block.resize(static_cast<size_t>(len));
        pageIn.clear();
        pageIn.seekg(static_cast<std::streamoff>(off), std::ios::beg);
        pageIn.read(&block[0], static_cast<std::streamsize>(len));
        if (static_cast<long long>(pageIn.gcount()) != len) {
            std::cerr << "PageLibPreprocessor: short read for docId=" << docId << "\n";
            continue;
        }
        processDocument(block, docId);
        ++_totalDocs;
    }
}

void PageLibPreprocessor::processDocument(const std::string& content, int docId) {
    // 提取 <title> 与 <content>，若缺失则使用全文
    auto extractTag = [](const std::string& s, const char* tag) -> std::string {
        std::string open = std::string("<") + tag + ">";
        std::string close = std::string("</") + tag + ">";
        size_t cs = s.find(open);
        if (cs == std::string::npos) return "";
        cs += open.size();
        size_t ce = s.find(close, cs);
        if (ce == std::string::npos || ce <= cs) return "";
        return s.substr(cs, ce - cs);
    };

    std::string title = extractTag(content, "title");
    std::string body  = extractTag(content, "content");
    std::string text;
    if (!title.empty() || !body.empty()) {
        text.reserve(title.size() + 1 + body.size());
        text.append(title);
        if (!title.empty() && !body.empty()) text.push_back(' ');
        text.append(body);
    } else {
        text = content; // 兜底：全文参与分词
    }

    // 分词
    auto splitter = SplitToolCppJieba::getInstance();
    std::vector<std::string> words = splitter->cut(text);

    std::unordered_map<std::string, int> localCount;
    localCount.reserve(words.size());

    for (const auto& w : words) {
        if (w.empty()) continue;
        if (g_stopWords.find(w) != g_stopWords.end()) continue;
        bool allSpace = true;
        for (char c : w) { if (!std::isspace(static_cast<unsigned char>(c))) { allSpace = false; break; } }
        if (allSpace) continue;
        ++localCount[w];
        ++_Wordtotal[w];
    }

    int totalTerms = 0;
    for (const auto& kv : localCount) totalTerms += kv.second;
    _DocTotalTerms[docId] = totalTerms;

    for (const auto& kv : localCount) {
        const std::string& w = kv.first;
        int cnt = kv.second;
        ++_DocFreq[w];
        _WordinPageCount[w].insert(std::make_pair(docId, cnt));
    }
}

void PageLibPreprocessor::calculate() {
    if (_totalDocs <= 0) {
        std::cerr << "PageLibPreprocessor: no documents loaded" << "\n";
        return;
    }

    std::unordered_map<std::string, std::vector<std::pair<int,double>>> tmpWeights;
    std::unordered_map<int, double> docNorm2;

    tmpWeights.reserve(_WordinPageCount.size());

    for (const auto& wd : _WordinPageCount) {
        const std::string& word = wd.first;
        const auto& postings = wd.second; // set<docId, count>
        int df = 0;
        auto itdf = _DocFreq.find(word);
        df = (itdf == _DocFreq.end()) ? static_cast<int>(postings.size()) : itdf->second;
        if (df <= 0) continue;
        double idf = calculateIDF(_totalDocs, df);

        auto& vec = tmpWeights[word];
        vec.reserve(postings.size());

        for (const auto& p : postings) {
            int docId = p.first;
            int termCount = p.second;
            auto itd = _DocTotalTerms.find(docId);
            int totalTerms = (itd == _DocTotalTerms.end()) ? 0 : itd->second;
            if (totalTerms == 0) continue;
            double tf = calculateTF(termCount, totalTerms);
            double w = tf * idf;
            vec.emplace_back(docId, w);
            docNorm2[docId] += w * w;
        }
    }

    for (auto& kv : tmpWeights) {
        const std::string& word = kv.first;
        auto& vec = kv.second;
        for (auto& dw : vec) {
            int docId = dw.first;
            double w = dw.second;
            double norm = std::sqrt(docNorm2[docId]);
            if (norm > 0.0) w /= norm;
            _InvertIndexTable[word].insert(std::make_pair(docId, w));
        }
    }
}

double PageLibPreprocessor::calculateTF(int termCount, int totalTerms) {
    if (totalTerms <= 0) return 0.0;
    return static_cast<double>(termCount) / static_cast<double>(totalTerms);
}

double PageLibPreprocessor::calculateIDF(int totalDocs, int docsWithTerm) {
    if (docsWithTerm <= 0) return 0.0;
    return std::log((static_cast<double>(totalDocs) + 1.0) / (static_cast<double>(docsWithTerm) + 1.0)) + 1.0;
}

void PageLibPreprocessor::normalizeWeights() {
    // 已在 calculate 中一次性完成归一化，这里留空以兼容接口
}

void PageLibPreprocessor::store() {
    if (_invertIndexPath.empty() || _wordFreqPath.empty() || _wordInPagePath.empty()) {
        initPathsFromConfig();
    }

    std::ofstream invOut(_invertIndexPath);
    if (!invOut.is_open()) {
        std::cerr << "PageLibPreprocessor: cannot open invert index for write: " << _invertIndexPath << "\n";
        return;
    }
    for (const auto& kv : _InvertIndexTable) {
        const std::string& word = kv.first;
        const auto& postings = kv.second;
        invOut << word;
        for (const auto& p : postings) {
            invOut << ' ' << p.first << ':' << std::fixed << std::setprecision(6) << p.second;
        }
        invOut << '\n';
    }
    invOut.close();

    std::ofstream wfOut(_wordFreqPath);
    if (!wfOut.is_open()) {
        std::cerr << "PageLibPreprocessor: cannot open word frequency for write: " << _wordFreqPath << "\n";
        return;
    }
    for (const auto& kv : _Wordtotal) {
        wfOut << kv.first << ' ' << kv.second << '\n';
    }
    wfOut.close();

    std::ofstream wpOut(_wordInPagePath);
    if (!wpOut.is_open()) {
        std::cerr << "PageLibPreprocessor: cannot open word-in-page for write: " << _wordInPagePath << "\n";
        return;
    }
    for (const auto& kv : _WordinPageCount) {
        const std::string& word = kv.first;
        const auto& postings = kv.second;
        wpOut << word;
        for (const auto& p : postings) {
            wpOut << ' ' << p.first << ':' << p.second;
        }
        wpOut << '\n';
    }
    wpOut.close();
}
