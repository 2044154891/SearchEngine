# 搜索引擎项目（Search Engine）

## 项目简介

这是一个基于 C++ 的全文搜索引擎，支持中文/英文语料处理、索引构建、关键词推荐与网页搜索。

项目当前技术文档已统一整理，请优先阅读：

- `documents/technical-guide.md`
- `documents/offline-index-data.md`

---

## 快速开始

### 1) 编译（CMake）

```bash
cmake -S . -B build
cmake --build build -j4
```

构建产物会输出到项目根目录 `bin/`。

如果需要重新从 0 构建：

```bash
cmake -S . -B build --fresh
cmake --build build -j4
```

### 2) 离线数据处理

```bash
./bin/DataClean
./bin/CreateDict
./bin/BuildIndex
```

### 3) 启动在线服务

```bash
./bin/SearchEngine
```

### 4) 启动网关与前端（可选）

```bash
cd gateway && npm install && npm start
cd web && npm install && npm run dev
```

---

## 项目目录（简版）

```text
Search_Engine/
├── conf/                    # 配置文件
├── data/                    # 索引与网页库数据
├── include/                 # 头文件与第三方头
├── src/                     # C++ 核心源码
├── gateway/                 # Node.js API 网关
├── web/                     # Vue 前端
├── documents/               # 项目文档
│   └── technical-guide.md   # 统一技术文档
├── CMakeLists.txt
├── Makefile                 # 旧构建入口，日常优先使用 CMake
└── README.md
```

---

## 核心能力

- 离线索引构建：RSS 解析 + SimHash 去重 + 倒排索引
- 在线检索服务：多线程 TCP 服务 + 二进制帧协议
- 查询能力：网页搜索、关键词推荐
- Web 接入：Node.js 网关 + Vue 前端

---

## 配置说明

主要配置文件：`conf/myconf.conf`

包含语料路径、停用词路径、索引路径、服务地址等关键项。部署到新机器前请先检查并调整路径/IP。

---

## 详细文档入口

完整架构、协议、模块职责、构建运行与配置说明请见：

- `documents/technical-guide.md`
- `documents/offline-index-data.md`
