# Search Engine 技术总览

## 1. 文档说明

本文件基于当前项目代码整理，作为项目的统一技术入口，覆盖系统架构、核心模块、通信协议、构建运行与配置说明。

当前 `documents/` 目录下的说明文档包括：

- `technical-guide.md`：项目技术总览，覆盖架构、模块、协议、构建运行与配置。
- `corpus-pipeline.md`：说明 `yuliao/art` 与 `yuliao/人民网语料` 两类语料如何分别进入关键词推荐和网页搜索链路。
- `cppjieba-and-llm.md`：说明当前项目中 CppJieba 的用法，并对比传统搜索中的词项/权重与大模型中的 token/向量。

---

## 2. 系统架构

项目采用“离线索引构建 + 在线检索服务 + 网关 + Web 前端”架构：

```text
Web (Vue + Vite, :3000)
    ↓ HTTP/JSON
Gateway (Node.js + Express, :8889)
    ↓ TCP 二进制帧协议
SearchEngine (C++ Server, server_ip:server_port)
    ↓ 读取
倒排索引 / 网页偏移索引 / 网页库文件
```

核心实现：
- 在线服务：`src/SearchEngineServer.cc`
- 协议分发：`src/ProtocolParser.cc`
- 搜索模块：`src/WebPageSearcher.cc`
- 网关服务：`gateway/server.js`

---

## 3. 模块与目录

### 3.1 C++ 核心

- `src/`
  - 网络层：`EventLoop`、`TcpServer`、`TcpConnection`、`EPollPoller`
  - 协议层：`ProtocolParser`
  - 搜索层：`WebPageSearcher`、`KeyRecommander`
  - 索引构建：`BuildIndex`、`PageLibPreprocessor`
- `include/`：头文件与第三方头文件（cppjieba、nlohmann/json、redis++）
- `conf/myconf.conf`：路径与服务配置
- `data/`：词典、网页库、偏移索引、倒排索引等产物

### 3.2 Web 相关

- `gateway/`：Node.js API 网关（`server.js`）
- `web/`：Vue 3 前端（Vite）

---

## 4. 离线索引构建

入口：`src/BuildIndex.cc`

流程：
1. RSS/XML 解析，生成原始网页库（`raw_page_store_path`）
2. SimHash 去重（海明距离阈值 3），生成去重网页库（`dedup_page_store_path`）
3. `PageLibPreprocessor` 构建倒排索引（TF-IDF + L2 归一化）

关键产物（`conf/myconf.conf`）：
- `raw_page_store_path` / `raw_page_offset_path`
- `dedup_page_store_path` / `dedup_page_offset_path`
- `web_inverted_index_path`

详细数据说明见：`documents/offline-index-data.md`

---

## 5. 在线检索服务

入口：`src/SearchEngineServer.cc`

### 5.1 线程模型

- `TcpServer` 工作线程数：4（`_tcpServer.setThreadNum(4)`）
- `ProtocolParser` 内部计算线程池：4
- 处理链路：IO 收包 → 协议解析 → 线程池计算 → 回 IO 线程发包

### 5.2 任务类型

请求任务 ID：
- `1`：关键词推荐（`TASK_RECOMMEND_KEYWORDS`）
- `2`：网页搜索（`TASK_SEARCH_WEBPAGES`）

响应 ID：
- `100`：关键词推荐响应
- `200`：网页搜索响应

---

## 6. 通信协议

C++ 服务二进制帧格式：

```text
[4字节 bodyLength][4字节 msgId][body(UTF-8)]
```

- 字节序：大端序
- C++ 解析位置：`ProtocolParser::tryParse`
- 网关封包位置：`gateway/server.js`

---

## 7. 搜索与推荐实现

### 7.1 网页搜索（`WebPageSearcher`）

- 启动加载：倒排索引 + 网页偏移
- 查询流程：分词 → 倒排打分 → `nth_element + sort` 取 Top-K → 按偏移读取正文
- 输出字段：`title`、`url`、`summary`

### 7.2 关键词推荐（`KeyRecommander`）

- 基于词典和编辑距离排序候选词
- 协议层异步执行并返回 JSON

---

## 8. Redis 缓存（当前实现）

- 初始化位置：`ProtocolParser` 线程池初始化回调
- 连接参数：`127.0.0.1:6379`（当前写在 `src/ProtocolParser.cc`）
- 使用场景：搜索结果缓存（`setSearchResult(content, response, 1800)`）

---

## 9. 网关与前端

### 9.1 API 网关（`gateway/server.js`）

- 监听端口：`8889`
- 后端目标：`192.168.147.131:8888`（可按部署修改）
- 接口：
  - `POST /api/search`
  - `POST /api/keyword`
  - `GET /health`

### 9.2 Web 前端（`web/package.json`）

- 开发：`npm run dev`
- 构建：`npm run build`
- 预览：`npm run preview`

---

## 10. 构建与运行

### 10.1 C++ 构建

当前项目以 CMake 作为推荐构建入口，同时保留根目录手写 `Makefile` 作为旧构建方式：

- `CMakeLists.txt`、`src/CMakeLists.txt`、`RSS/CMakeLists.txt`、`simhash/CMakeLists.txt`：推荐构建入口，执行 out-of-source 构建，最终可执行文件输出到项目根目录 `bin/`。
- `Makefile`：项目原有构建入口，仅作为兼容/排障备用。

CMake 构建会生成这些可执行目标：

- `DataClean`
- `CreateDict`
- `BuildIndex`
- `SearchEngine`

使用 CMake：

```bash
cmake -S . -B build
cmake --build build -j4
```

从 0 重新配置并构建：

```bash
cmake -S . -B build --fresh
cmake --build build -j4
```

### 10.2 离线流程

```bash
./bin/DataClean
./bin/CreateDict
./bin/BuildIndex
```

### 10.3 在线服务

```bash
./bin/SearchEngine
```

### 10.4 网关与前端

```bash
cd gateway && npm install && npm start
cd web && npm install && npm run dev
```

---

## 11. 配置清单

配置文件：`conf/myconf.conf`

重点配置项：
- 语料路径：`chinese_corpus_path`、`english_corpus_path`
- 停用词：`chinese_stop_words`、`english_stop_words`
- 字典：`dictionary_path`、`dictionary_index_path`
- 网页库与索引：`raw_page_store_path`、`dedup_page_store_path`、`web_inverted_index_path`
- 服务地址：`server_ip`、`server_port`

> 当前配置大量使用绝对路径，换机器部署前需统一调整。
