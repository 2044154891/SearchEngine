const express = require('express');
const cors = require('cors');
const net = require('net');

const app = express();
const PORT = 8889;

// 搜索引擎后端配置
const BACKEND_HOST = '192.168.147.131';
const BACKEND_PORT = 8888;

// 任务ID常量
const TASK_RECOMMEND_KEYWORDS = 1;
const TASK_SEARCH_WEBPAGES = 2;
const RESPONSE_RECOMMEND_KEYWORDS = 100;
const RESPONSE_SEARCH_WEBPAGES = 200;

app.use(cors());
app.use(express.json());

// 创建TCP连接到搜索引擎后端
function createBackendConnection() {
  return new Promise((resolve, reject) => {
    const client = new net.Socket();
    
    client.connect(BACKEND_PORT, BACKEND_HOST, () => {
      console.log(`Connected to backend: ${BACKEND_HOST}:${BACKEND_PORT}`);
      resolve(client);
    });
    
    client.on('error', (err) => {
      console.error('Backend connection error:', err.message);
      reject(err);
    });
  });
}

// 发送二进制帧到后端并等待响应
function sendRequest(taskId, content) {
  return new Promise(async (resolve, reject) => {
    let client;
    try {
      client = await createBackendConnection();
      
      // 构建二进制帧
      // 格式: 4字节长度 + 4字节消息ID + 内容
      const contentBuffer = Buffer.from(content, 'utf8');
      const bodyLength = contentBuffer.length;
      
      // 分配缓冲区: 8字节头 + 内容
      const frame = Buffer.alloc(8 + bodyLength);
      
      // 写入长度 (大端序)
      frame.writeUInt32BE(bodyLength, 0);
      // 写入消息ID (大端序)
      frame.writeUInt32BE(taskId, 4);
      // 写入内容
      contentBuffer.copy(frame, 8);
      
      // 发送请求
      client.write(frame);
      console.log(`Sent request: taskId=${taskId}, content="${content}"`);
      
      // 等待响应
      const response = await new Promise((res, rej) => {
        let responseData = Buffer.alloc(0);
        
        const onData = (data) => {
          responseData = Buffer.concat([responseData, data]);
          
          // 至少需要8字节才能解析响应头
          if (responseData.length >= 8) {
            const respLen = responseData.readUInt32BE(0);
            const respId = responseData.readUInt32BE(4);
            
            // 检查是否接收完整
            if (responseData.length >= 8 + respLen) {
              client.removeListener('data', onData);
              client.end();
              
              const body = responseData.slice(8, 8 + respLen).toString('utf8');
              console.log(`Received response: respId=${respId}, body="${body.substring(0, 100)}..."`);
              res({ respId, body });
            }
          }
        };
        
        client.on('data', onData);
        client.on('error', rej);
        
        // 超时处理
        setTimeout(() => {
          client.removeListener('data', onData);
          client.end();
          rej(new Error('Request timeout'));
        }, 10000);
      });
      
      resolve(response);
    } catch (err) {
      if (client) client.end();
      reject(err);
    }
  });
}

// API: 关键词推荐
app.post('/api/keyword', async (req, res) => {
  try {
    const { query } = req.body;
    
    if (!query || !query.trim()) {
      return res.status(400).json({ 
        success: false, 
        message: '查询不能为空' 
      });
    }
    
    const response = await sendRequest(TASK_RECOMMEND_KEYWORDS, query);
    const data = JSON.parse(response.body);
    
    res.json({
      success: true,
      suggestions: data.suggestions || []
    });
  } catch (err) {
    console.error('Keyword API error:', err.message);
    res.status(500).json({ 
      success: false, 
      message: '服务错误: ' + err.message 
    });
  }
});

// API: 网页搜索
app.post('/api/search', async (req, res) => {
  try {
    const { query } = req.body;
    
    if (!query || !query.trim()) {
      return res.status(400).json({ 
        success: false, 
        message: '查询不能为空' 
      });
    }
    
    const response = await sendRequest(TASK_SEARCH_WEBPAGES, query);
    const data = JSON.parse(response.body);
    
    res.json({
      success: true,
      total: data.total || 0,
      results: data.results || []
    });
  } catch (err) {
    console.error('Search API error:', err.message);
    res.status(500).json({ 
      success: false, 
      message: '服务错误: ' + err.message 
    });
  }
});

// 健康检查
app.get('/health', (req, res) => {
  res.json({ status: 'ok' });
});

app.listen(PORT, () => {
  console.log(`API Gateway running on http://localhost:${PORT}`);
  console.log(`Backend: ${BACKEND_HOST}:${BACKEND_PORT}`);
  console.log(`Endpoints:`);
  console.log(`  - POST /api/keyword  (关键词推荐)`);
  console.log(`  - POST /api/search   (网页搜索)`);
});
