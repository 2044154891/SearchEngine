# 离线网页索引数据说明

离线网页索引由 `./bin/BuildIndex` 生成，主入口是 `src/BuildIndex.cc`。它把 `rss_corpus_dir` 中的 RSS/XML 语料解析成网页库，再经过 SimHash 去重，最后构建供在线搜索使用的倒排索引。

## 处理流程

1. RSS/XML 解析
- `RssReader` 遍历 `rss_corpus_dir` 下的 XML/RSS 文件。
- 每个 `<item>` 会被整理成一个 `<doc>...</doc>` 文档块，包含 `docid`、`title`、`link`、`content`。
- 输出原始网页库和原始偏移表。

2. SimHash 去重
- `performDeduplication()` 读取原始网页库，抽取 `title + content` 计算 SimHash。
- 使用海明距离阈值 3 去重。
- 输出去重后的网页库和偏移表。
- `src/BuildIndex.cc` 中的主流程会把去重后 docId 从 1 开始重新编号。

3. 倒排索引构建
- `PageLibPreprocessor` 读取去重网页库。
- 对 `title + content` 分词、过滤停用词，统计 TF、DF、IDF。
- 权重计算为 `TF * IDF`，随后按文档向量做 L2 归一化。
- 输出倒排索引、全局词频、词在文档内的出现次数。

## 配置键与数据内容

| 配置键 | 当前文件/目录 | 数据内容 |
| --- | --- | --- |
| `rss_corpus_dir` | `yuliao/人民网语料` | RSS/XML 原始网页语料目录。 |
| `raw_page_store_path` | `data/ripepage.dat` | RSS/XML 解析后的原始网页库，每篇文档是一个 `<doc>...</doc>` 块。 |
| `raw_page_offset_path` | `data/oldoffset.dat` | 原始网页库偏移表，每行 `docId offset length`。 |
| `dedup_page_store_path` | `data/newripepage.dat` | SimHash 去重后的网页库，供倒排索引构建和在线搜索读取。 |
| `dedup_page_offset_path` | `data/newoffset.dat` | 去重网页库偏移表，每行 `docId offset length`。 |
| `web_inverted_index_path` | `data/invertindex.dat` | 倒排索引，每行 `word docId:weight docId:weight ...`。 |
| `web_word_frequency_path` | `data/wordfrequence.dat` | 离线统计的全局词频，每行 `word total_count`。 |
| `web_word_in_page_path` | `data/wordinpage.dat` | 每个词在各文档中的出现次数，格式 `word docId:count docId:count ...`。 |

## 在线读取关系

`WebPageSearcher` 在线阶段只读取去重后的网页数据：

- `dedup_page_store_path`：按偏移随机读取网页内容。
- `dedup_page_offset_path`：建立 `docId -> offset/length` 映射。
- `web_inverted_index_path`：建立 `word -> [(docId, weight)]` 倒排表。

`raw_page_store_path` 和 `raw_page_offset_path` 只服务于离线去重阶段，在线搜索不会读取。
