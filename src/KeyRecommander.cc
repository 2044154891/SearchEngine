#include "KeyRecommander.h"
#include "Lexicon.h"

#include <algorithm>
#include <vector>
#include <set>

// Helpers
static size_t getByteNum_UTF8(const char byte)
{
    int byteNum = 0;
    for (size_t i = 0; i < 6; ++i)
    {
        if (byte & (1 << (7 - i)))
            ++byteNum;
        else
            break;
    }
    return byteNum == 0 ? 1 : byteNum;
}

static std::vector<uint32_t> utf8ToCodepoints(const std::string &s)
{
    std::vector<uint32_t> out;
    size_t i = 0, n = s.size();
    while (i < n)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80)
        {
            out.push_back(c);
            i += 1;
        }
        else if ((c >> 5) == 0x6 && i + 1 < n)
        {
            uint32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
            out.push_back(cp);
            i += 2;
        }
        else if ((c >> 4) == 0xE && i + 2 < n)
        {
            uint32_t cp = ((c & 0x0F) << 12)
                        | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6)
                        |  (static_cast<unsigned char>(s[i + 2]) & 0x3F);
            out.push_back(cp);
            i += 3;
        }
        else if ((c >> 3) == 0x1E && i + 3 < n)
        {
            uint32_t cp = ((c & 0x07) << 18)
                        | ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12)
                        | ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6)
                        |  (static_cast<unsigned char>(s[i + 3]) & 0x3F);
            out.push_back(cp);
            i += 4;
        }
        else
        {
            // Invalid leading byte, skip
            i += 1;
        }
    }
    return out;
}

KeyRecommander::KeyRecommander(const std::string &query)
    : _originalQuery(query)
{
    auto split = SplitToolCppJieba::getInstance()->cut(query);
    _queryWord = split.empty() ? query : split.back();
}

void KeyRecommander::execute()
{
    queryIndexTable();
    buildSuggestions();
}

void KeyRecommander::queryIndexTable()
{
    std::set<int> indexSet;
    const std::string &w = _queryWord;
    for (size_t idx = 0; idx < w.size();)
    {
        size_t clen = getByteNum_UTF8(w[idx]);
        std::string sub = w.substr(idx, clen);
        idx += clen;

        const std::vector<int> *posting = Lexicon::instance().findPosting(sub);
        if (posting)
        {
            for (int id : *posting) indexSet.insert(id);
        }
    }
    statistic(indexSet);
}

void KeyRecommander::statistic(std::set<int> &iset)
{
    const auto &dict = Lexicon::instance().dict();
    const size_t K = 5;

    for (int id : iset)
    {
        if (id < 0 || static_cast<size_t>(id) >= dict.size()) continue;
        const auto &entry = dict[id];

        MyResult r;
        r.word = entry.first;
        r.frequency = entry.second;
        r.distance = distance(entry.first);

        if (_resultQue.size() < K) {
            _resultQue.push(r);
        } else {
            const MyResult &worst = _resultQue.top();
            bool better = (r.distance < worst.distance) ||
                          (r.distance == worst.distance && r.frequency > worst.frequency) ||
                          (r.distance == worst.distance && r.frequency == worst.frequency && r.word < worst.word);
            if (better) { _resultQue.pop(); _resultQue.push(r); }
        }
    }
}

int KeyRecommander::distance(const std::string &rhs) const
{
    auto A = utf8ToCodepoints(_queryWord);
    auto B = utf8ToCodepoints(rhs);
    if (A.empty()) return static_cast<int>(B.size());
    if (B.empty()) return static_cast<int>(A.size());
    if (A.size() < B.size()) std::swap(A, B);

    std::vector<int> prev(B.size() + 1), curr(B.size() + 1);
    for (size_t j = 0; j <= B.size(); ++j) prev[j] = static_cast<int>(j);

    for (size_t i = 1; i <= A.size(); ++i)
    {
        curr[0] = static_cast<int>(i);
        for (size_t j = 1; j <= B.size(); ++j)
        {
            int cost = (A[i - 1] == B[j - 1]) ? 0 : 1;
            curr[j] = std::min({ prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost });
        }
        prev.swap(curr);
    }
    return prev.back();
}

void KeyRecommander::buildSuggestions()
{
    _fullSuggestions.clear();
    size_t pos = _originalQuery.size();
    if (!_queryWord.empty()) {
        size_t found = _originalQuery.rfind(_queryWord);
        if (found != std::string::npos) pos = found;
    }
    std::vector<MyResult> res;
    res.reserve(_resultQue.size());
    while (!_resultQue.empty()) { res.push_back(_resultQue.top()); _resultQue.pop(); }
    for (const auto &r : res) {
        std::string s = _originalQuery.substr(0, pos);
        s.append(r.word);
        _fullSuggestions.push_back(std::move(s));
    }
}
