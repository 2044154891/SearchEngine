#!/bin/bash

# 网关服务重启脚本

echo "正在重启 API 网关服务..."

# 查找并杀死现有的 node 进程 (gateway server.js)
PID=$(ps -ef | grep "node.*server.js" | grep -v grep | awk '{print $2}')

if [ -n "$PID" ]; then
    echo "正在停止现有进程 (PID: $PID)..."
    kill $PID
    sleep 1
else
    echo "未发现运行中的网关进程"
fi

# 启动服务
echo "正在启动网关服务..."
cd /home/zhang/Search_Engine/gateway
npm start &

echo "网关服务已重启，监听端口 8889"
