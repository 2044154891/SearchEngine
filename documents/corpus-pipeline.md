# 两类语料的作用链路：`art` vs `人民网语料`

本项目采用“离线建库 + 在线检索”的架构，但不同语料会喂给不同的离线构建模块，最终进入不同的在线请求处理器。

- `yuliao/art`（中文原始语料目录）主要用于构建“分词/词典/关键词推荐”相关的数据结构（`dict.dat`、`dictindex.dat`），在线上被 `KeyRecommander` 使用。
- `yuliao/人民网语料`（XML/RSS 网页语料目录）主要用于构建“网页倒排索引 + 网页库 + 偏移索引”（`newripepage.dat`、`newoffset.dat`、`invertindex.dat`），在线上被 `WebPageSearcher` 使用。

最终返回结果也不同：

- 关键词推荐：返回 JSON 里的 `suggestions`（由 `KeyRecommander` 生成）
- 网页搜索：返回 JSON 里的 `results`（由 `WebPageSearcher` 生成）

---

## 1. `art` 语料如何作用到最终结果（关键词推荐）

### 1.1 原始数据

`conf/myconf.conf` 中：

- `chinese_corpus_path = /home/zhang/Search_Engine/yuliao/art`

目录里如 `C3-Art000x.txt` 的每个文件是“原始中文文本”。

### 1.2 清洗：`./bin/DataClean`（`src/DataClean.cc`）

入口：`./bin/DataClean`

清洗主要发生在 `Chinese_Clean()` / `process_directory()`：

- `process_directory(input_dir, output_dir, "chinese")` 会遍历 `chinese_corpus_path` 目录下的普通文件
- `Chinese_Clean()` 逐行读取文本并做“UTF-8 字节合法性过滤”（仅在检测到可能的 UTF-8 起始字节时保留对应长度的字节片段；对非法或不完整的 UTF-8 字节会跳过）
- 清洗后的内容会写出为 `*_cleaned.txt`
- 最后通过 `mv` 将 `*_cleaned.txt` 移到 `cleaned_corpus_path + "/chinese/"` 目录

典型输出：

- `/home/zhang/Search_Engine/yuliao/clean_data/chinese/*_cleaned.txt`

补充：当前实现中，每个源文件在后续建词典阶段可能只读取第一行（见 1.3）。

### 1.3 词典/词索引构建：`./bin/CreateDict`（`src/CreateDict.cc` -> `src/DictProducer.cc`）

入口：`./bin/CreateDict`

该阶段由 `DictProducer` 完成，关键处理方法包括：

1. 停用词加载
- 从 `conf/myconf.conf` 的 `chinese_stop_words` / `english_stop_words` 读取停用词集合，并额外加入 `" "` 作为停用词

2. 中文分词与词频统计
- 对 `cleaned_corpus_path + "/chinese"` 下的文件逐一处理
- 使用 `SplitToolCppJieba::cut(sentence)` 进行中文分词（Jieba）
- 过滤逻辑：分词结果若在停用词集合中则跳过；仅保留满足 `getByteNum_UTF8(word[0]) == 3` 的词条（即按实现假设“中文 3 字节 UTF-8”）
- 统计词频：使用 `mp[word]++`

3. 生成词典与子词索引
- 将词频统计结果排序后得到 `_dict`（词条和频次）
- `createIndex()` 把词条“切成子串”来构建倒排到词典 ID 的映射：遍历每个词条的 UTF-8 字节长度切片；对每个子串 `subWord`，把“词典 ID 集合”记录到 `_index[subWord]`
- `store()` 将 `_index` 写到 `dictionary_index_path`（配置为 `data/dictindex.dat`）；写入格式：每行是 `subWord id1 id2 id3 ...`

数据文件（由配置决定）：

- `dictionary_path = /home/zhang/Search_Engine/data/dict.dat`：`Lexicon` 用来加载词条词频
- `dictionary_index_path = /home/zhang/Search_Engine/data/dictindex.dat`：`Lexicon` 用来加载子词 -> 词典 ID 倒排映射

### 1.4 在线推荐：`KeyRecommander` -> `ProtocolParser` 返回 `suggestions`

在线入口在 `src/ProtocolParser.cc`：

- 关键词推荐任务使用 `TASK_RECOMMEND_KEYWORDS`（任务 ID 1）
- 请求处理函数为 `ProtocolParser::handleRecommendKeywordsAsync()` / `handleRecommendKeywords()`
- 返回 JSON 中的响应 ID：`RESPONSE_RECOMMEND_KEYWORDS`（文档中标注为 100）

`KeyRecommander` 的关键流程：

- 对用户输入 `query` 使用 Jieba 分词，取最后一个分词结果作为 `_queryWord`
- 将 `_queryWord` 按 UTF-8 字节长度切成子串（与词索引构建方式对应）
- 对每个子串：通过 `Lexicon::findPosting(sub)` 找到候选词典 ID 集合；取并集形成候选集合
- 排序/打分：
- 使用 `distance()` 计算候选词与 `_queryWord` 的编辑距离（相当于 Levenshtein 距离）
- 结合词条频次（词频越大越好）
- 生成建议：
- 以原始 query 前缀 + 候选词拼接，形成每个 suggestion 字符串

最终返回：

- JSON：`{ "id": 100, "query": "...", "suggestions": [ ... ] }`

---

## 2. `人民网语料`如何作用到最终结果（网页搜索）

### 2.1 原始数据

`conf/myconf.conf` 中：

- `rss_corpus_dir = /home/zhang/Search_Engine/yuliao/人民网语料`

目录下是多个 XML 文件（如 `game.xml`、`env.xml`、`culture.xml` 等），这些文件包含 RSS/XML 的 item 列表。

### 2.2 倒排索引构建：`./bin/BuildIndex`（`src/BuildIndex.cc`）

入口：`./bin/BuildIndex`

该程序内部明确包含 3 步：

1. 解析 RSS/XML -> 生成原始网页库 + 偏移索引
2. SimHash 去重 -> 生成去重后的网页库 + 偏移索引
3. `PageLibPreprocessor` -> 计算 TF-IDF、做 L2 归一化 -> 写出倒排索引

#### 2.2.1 Step 1：RSS/XML 解析（`RSS/rss_reader.cpp`）

`RssReader` 会遍历 `rss_corpus_dir` 下的 XML/RSS 文件：

- 对每个 `<item>`：
- 取 `<title>` 作为 title
- 取 `<link>` 作为 link
- 取 `<content:encoded>` 或 `<content>`，没有则退化到 `<description>`
- 通过 `removeHtmlTags()` 去掉 HTML 标签与实体
- 写入网页库文件（`old_rss_corpus_dir`）
- 以 `<doc> ... </doc>` 形式流式写入
- 同时写入偏移文件（`raw_page_offset_path`）：每行 `docId offset length`

因此，Step 1 的关键产物是：

- `old_rss_corpus_dir`：原始网页库（例如 `data/ripepage.dat`）
- `raw_page_offset_path`：原始偏移索引（例如 `data/oldoffset.dat`）

#### 2.2.2 Step 2：SimHash 去重

在 `performDeduplication()` 中：

- 对每个 doc 的 `extractText()` 内容计算 SimHash
- 使用 `Deduplication::deduplicateFeatures(features, 3)` 做海明距离阈值为 3 的去重
- 将保留的 doc 写入新的网页库文件（`new_rss_corpus_dir`）
- 同时重写新的偏移文件（`dedup_page_offset_path`）

去重后 docId 从 1 开始重新编号，因此你会看到新 offset 文件与 old offset 不再一一对应。

对应配置文件产物：

- `new_rss_corpus_dir`：例如 `data/newripepage.dat`
- `dedup_page_offset_path`：例如 `data/newoffset.dat`

#### 2.2.3 Step 3：`PageLibPreprocessor` 构建倒排索引（TF-IDF + L2）

在 `PageLibPreprocessor::processDocument()`：

- 通过 `newoffset.dat` 的 `offset/length` 从网页库中读取 `<doc>` 块
- 从内容块中抽取 `<title>` 与 `<content>`：
- 若 title 与 content 都存在，则拼成 `title + ' ' + content`
- 若缺失，则用全文兜底
- 使用 `SplitToolCppJieba::cut(text)` 做分词
- 过滤：
- 在停用词集合中则跳过
- 统计每篇文档的：
- TF：`termCount / totalTerms`
- IDF：`log((totalDocs + 1) / (docsWithTerm + 1)) + 1`
- 权重：`w = tf * idf`
- 最后对每个文档向量做 L2 归一化：`w /= sqrt(sum(w^2))`

`store()` 会把结果写入多个文件：

- `web_inverted_index_path`（倒排索引）：例如 `data/invertindex.dat`
- 行格式：`word docId:weight docId:weight ...`
- `web_word_frequency_path`（词频）
- `web_word_in_page_path`（词在文档内的出现计数）

### 2.3 在线搜索：`WebPageSearcher` -> `ProtocolParser` 返回 `results`

在线入口在 `src/ProtocolParser.cc`：

- 网页搜索任务使用 `TASK_SEARCH_WEBPAGES`（任务 ID 2）
- 搜索处理函数为 `ProtocolParser::handleSearchWebpagesAsync()`
- 返回 JSON 中的响应 ID：`RESPONSE_SEARCH_WEBPAGES`（文档中标注为 200）

`WebPageSearcher` 的关键流程：

1. 服务启动时加载索引
- `loadIndex()` 会读取：`invertindex.dat`（`_invertIndex[word] = vector<pair<docId, weight>>`）和 `newoffset.dat`（`_pageOffset[docId] = (offset, length)`）
- 并以二进制方式打开 `new_rss_corpus_dir`

2. 查询时分词并打分
- `cutQuery(query)` 使用 Jieba 分词
- 对每个查询词 `word`：若存在倒排条目，则对每个 `docId` 累加权重
- 形成 `docScores[docId] = score`

3. Top-K 选择
- 当候选数量超过 `topK` 时使用 `nth_element` 做部分排序
- 再对 Top-K 结果做 `sort` 保证顺序稳定

4. 取回网页并构造输出字段
- 对每个 Top-K doc：`getPageContent(docId)` 根据 `(offset,length)` 从网页库中读取正文块，并取 `<title>` / `<link>` / `<content>` 来生成字段；`generateSummary()` 按 UTF-8 字符生成查询词相关片段，并只在实际截断时加 `...`

最终返回：

- JSON：`{ "id": 200, "query": "...", "total": N, "results": [ { "docId":..., "score":..., "title":..., "url":..., "summary":... }, ... ] }`

---

## 3. 为什么两类语料会分工不同

简要总结对应关系：

- `art` -> 清洗 -> 构建“词典/子词索引” -> `KeyRecommander` 用编辑距离与词频做候选排序 -> 返回 `suggestions`
- `人民网语料` -> RSS/XML 解析 + SimHash 去重 -> `PageLibPreprocessor` 计算 TF-IDF 倒排 -> `WebPageSearcher` 按倒排权重打分 -> 返回 `results`

因此它们并不是“同一套索引的不同输入文件”，而是分别服务于两个在线能力：关键词推荐与网页搜索。

