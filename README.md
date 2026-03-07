# 搜索引擎项目 (Search Engine)

## 项目简介

这是一个基于C++开发的全文搜索引擎，支持中文和英文语料的处理、索引构建和关键词推荐等功能。

## 项目结构

```
Search_Engine/
├── bin/                    # 编译生成的可执行文件目录
├── client/                 # 客户端代码
│   └── client.cc          # 测试客户端
├── conf/                   # 配置文件
│   └── myconf.conf        # 主配置文件
├── data/                   # 数据文件目录（索引、网页库等）
├── include/                # 头文件目录
│   ├── cppjieba/          # 中文分词库
│   └── nlohmann/          # JSON库
├── log/                    # 日志目录
├── RSS/                    # RSS订阅源解析模块
├── simhash/                # SimHash去重模块
├── src/                    # 源代码目录
├── test/                   # 测试文件目录
├── yuliao/                 # 语料库目录
│   ├── art/                # 中文语料
│   ├── clean_data/         # 清洗后的语料
│   ├── english/            # 英文语料
│   └── 人民网语料/          # 网页语料
├── Makefile                # 编译配置
└── README.md               # 本文件
```

## 核心功能模块

### 1. 语料处理 (DataClean)
- 读取原始语料
- 去除噪音数据（HTML标签、特殊字符等）
- 生成清洗后的语料库

### 2. 词典构建 (CreateDict)
- 使用CppJieba进行中文分词
- 去除停用词
- 生成词典和词典索引

### 3. 索引构建 (BuildIndex)
- **RSS解析**: 解析XML网页源，生成原始网页库
- **SimHash去重**: 基于海明距离（阈值=3）去除重复网页
- **倒排索引**: TF-IDF权重计算，构建倒排索引
- 完整流程：RSS → 去重 → 索引

### 4. 搜索引擎服务器 (SearchEngineServer)
- 基于Muduo网络库实现的TCP服务器
- 支持多线程
- 帧协议通信

### 5. 网页搜索 (WebPageSearcher)
- 加载倒排索引和网页偏移索引
- 接收查询请求，返回Top-K搜索结果

### 6. 关键词推荐 (KeyRecommander)
- 基于编辑距离的智能推荐
- 结合词频和相关性排序

### 7. SimHash去重 (simhash/)
- 基于海明距离的文档去重
- 已集成到BuildIndex流程中

## 编译指南

### 编译整个项目

```bash
cd /home/zhang/Search_Engine
make
```

### 编译各个模块

```bash
# 编译语料清洗模块
make bin/DataClean

# 编译词典构建模块
make bin/CreateDict

# 编译索引构建模块（包含去重）
make bin/BuildIndex

# 编译搜索引擎服务器
make bin/SearchEngine
```

### 清理编译文件

```bash
make clean
```

### 重新编译

```bash
make rebuild
```

## 运行指南

### 第一步：语料清洗

```bash
./bin/DataClean
```

### 第二步：构建词典

```bash
./bin/CreateDict
```

### 第三步：构建索引（包含去重）

```bash
./bin/BuildIndex
```

此步骤完成：
- RSS解析XML网页源
- SimHash去重（基于海明距离阈值=3）
- TF-IDF权重计算
- 构建倒排索引

### 第四步：启动服务器

```bash
./bin/SearchEngine
```

服务器默认配置：
- IP: 192.168.147.131 (可配置)
- 端口: 8888 (可配置)

### 第五步：运行客户端测试

```bash
cd client
./client
```

或重新编译客户端：

```bash
cd client
make
```

## 配置文件说明

配置文件位于 `conf/myconf.conf`，主要配置项：

```ini
# 语料路径配置
chinese_corpus_path = /home/zhang/Search_Engine/yuliao/art
english_corpus_path = /home/zhang/Search_Engine/yuliao/english
cleaned_corpus_path = /home/zhang/Search_Engine/yuliao/clean_data

# 停用词配置
chinese_stop_words = /home/zhang/Search_Engine/yuliao/stop_words_zh.txt
english_stop_words = /home/zhang/Search_Engine/yuliao/stop_words_eng.txt

# 索引数据路径
dictionary_path = /home/zhang/Search_Engine/data/dict.dat
dictionary_index_path = /home/zhang/Search_Engine/data/dictindex.dat
invertindex_path = /home/zhang/Search_Engine/data/invertindex.dat

# 服务器配置
server_ip = 192.168.147.131
server_port = 8888
```

## 通信协议

客户端与服务器采用帧协议通信：

### 请求格式
```
[4字节消息长度][4字节任务ID][消息内容]
```

### 任务ID
- `1` (TASK_RECOMMEND_KEYWORDS): 关键词推荐
- `2` (TASK_SEARCH_WEBPAGES): 网页搜索

### 响应格式
```
[4字节消息长度][4字节响应ID][响应内容]
```

## 依赖项

- C++11 及以上
- CMake (可选，用于部分模块)
- CppJieba (已包含在项目中)
- Muduo 网络库 (已包含在项目中)

## 注意事项

1. 首次运行需要先执行语料清洗和词典构建
2. 确保配置文件中的路径正确
3. 服务器需要稳定的网络环境
4. 客户端连接前确保服务器已启动

## 开发者

项目托管于 GitHub: https://github.com/2044154891/SearchEngine

## 许可证

MIT License
