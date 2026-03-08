#!/bin/bash

# 前端服务重启脚本

echo "正在重启 Web 前端服务..."

# 查找并杀死现有的 vite 进程
PID=$(ps -ef | grep "vite" | grep -v grep | awk '{print $2}')

if [ -n "$PID" ]; then
    echo "正在停止现有进程 (PID: $PID)..."
    kill $PID
    sleep 1
else
    echo "未发现运行中的前端进程"
fi

# 启动服务
echo "正在启动前端服务..."
cd /home/zhang/Search_Engine/web
npm run dev &

echo "前端服务已重启，监听端口 3000"
