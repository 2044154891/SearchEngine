# 搜索引擎 Web 方案

## 系统架构

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Web 浏览器     │────▶│  API 网关       │────▶│  搜索引擎后端    │
│  (Vue.js 前端)   │     │  (Node.js)      │     │  (C++ Server)   │
│   localhost:3000 │     │  localhost:8889 │     │  192.168.147.131│
└─────────────────┘     └─────────────────┘     └─────────────────┘
                              (HTTP)                  (二进制协议)
```

## 目录结构

```
Search_Engine/
├── web/                    # Web 前端 (Vue.js)
│   ├── package.json       # 项目依赖配置
│   ├── vite.config.js      # Vite 配置
│   └── index.html         # 主页面
│
├── gateway/               # API 网关 (Node.js)
│   ├── package.json       # 项目依赖配置
│   └── server.js          # 网关服务
│
└── ...                    # 原有的搜索引擎代码
```

## 启动步骤

### 1. 启动搜索引擎后端

确保 C++ 搜索引擎服务已启动，监听 `192.168.147.131:8888`

```bash
cd /home/zhang/Search_Engine
# 启动搜索引擎服务 (需要先编译)
./bin/SearchEngine
```

### 2. 启动 API 网关

```bash
cd /home/zhang/Search_Engine/gateway

# 安装依赖
npm install

# 启动服务
npm start
```

网关将在 `http://localhost:8889` 启动

### 3. 启动 Web 前端

```bash
cd /home/zhang/Search_Engine/web

# 安装依赖
npm install

# 启动开发服务器
npm run dev
```

前端将在 `http://localhost:3000` 启动

## 服务管理

### 重启 API 网关

```bash
# 方式一：使用脚本重启（推荐）
/home/zhang/Search_Engine/gateway/restart.sh

# 方式二：手动重启
cd /home/zhang/Search_Engine/gateway
# 停止现有进程
pkill -f "node.*server.js"
# 重新启动
npm start &
```

### 重启 Web 前端

```bash
# 方式一：使用脚本重启（推荐）
/home/zhang/Search_Engine/web/restart.sh

# 方式二：手动重启
cd /home/zhang/Search_Engine/web
# 停止现有进程
pkill -f vite
# 重新启动
npm run dev &
```

> **注意**: 使用脚本重启前，请确保已执行过 `npm install` 安装依赖

## API 接口

| 接口 | 方法 | 说明 |
|------|------|------|
| `/api/keyword` | POST | 关键词推荐 |
| `/api/search` | POST | 网页搜索 |
| `/health` | GET | 健康检查 |

### 请求示例

**关键词推荐**
```bash
curl -X POST http://localhost:8889/api/keyword \
  -H "Content-Type: application/json" \
  -d '{"query":"搜索"}'
```

**网页搜索**
```bash
curl -X POST http://localhost:8889/api/search \
  -H "Content-Type: application/json" \
  -d '{"query":"搜索"}'
```

## 工作原理

### 请求流程

1. **Web 前端**发送 HTTP 请求到 API 网关
2. **API 网关**将 HTTP 请求转换为二进制协议：
   - 关键词推荐: msgId = 1
   - 网页搜索: msgId = 2
3. **搜索引擎后端**处理请求并返回结果
4. **API 网关**解析二进制响应，转换为 JSON 返回给前端

### 二进制协议格式

```
┌──────────────┬──────────────┬─────────────────┐
│  4 字节       │  4 字节       │   N 字节         │
│  消息长度     │   消息ID      │   消息内容       │
│  (大端序)     │  (大端序)     │   (UTF-8)       │
└──────────────┴──────────────┴─────────────────┘
```

- 消息ID 1 = 关键词推荐
- 消息ID 2 = 网页搜索

## 注意事项

1. 确保防火墙允许相应端口访问
2. 后端 IP 地址可在 `gateway/server.js` 中修改
3. 开发模式下前端会自动代理 API 请求到网关
