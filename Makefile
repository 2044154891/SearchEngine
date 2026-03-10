# 默认使用glog日志库
USE_GLOG = 1

ifeq ($(USE_GLOG),1)
    # -UNDEBUG 确保glog的DEBUG日志可用
    GLOG_FLAGS = -DUSE_GLOG -UNDEBUG
    GLOG_LIBS = -lglog
else
    GLOG_FLAGS =
    GLOG_LIBS =
endif

All: bin bin/DataClean bin/CreateDict bin/SearchEngine bin/BuildIndex

bin:
	mkdir -p bin
	mkdir -p log

# DataClean 目标
bin/DataClean: bin/DataClean.o
	g++ -g $(GLOG_FLAGS) -o bin/DataClean bin/DataClean.o $(GLOG_LIBS)

bin/DataClean.o: src/DataClean.cc
	g++ -g $(GLOG_FLAGS) -c src/DataClean.cc -o bin/DataClean.o -I include

# CreateDict 目标
bin/CreateDict: bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o
	g++ -g $(GLOG_FLAGS) -o bin/CreateDict bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o $(GLOG_LIBS)

bin/CreateDict.o: src/CreateDict.cc
	g++ -g $(GLOG_FLAGS) -c src/CreateDict.cc -o bin/CreateDict.o -I include

bin/DictProducer.o: src/DictProducer.cc include/DictProducer.h
	g++ -g $(GLOG_FLAGS) -c src/DictProducer.cc -o bin/DictProducer.o -I include

bin/SplitToolCppJieba.o: src/SplitToolCppJieba.cc include/SplitToolCppJieba.h
	g++ -g $(GLOG_FLAGS) -c src/SplitToolCppJieba.cc -o bin/SplitToolCppJieba.o -I include

bin/SplitTool.o: src/SplitTool.cc include/SplitTool.h
	g++ -g $(GLOG_FLAGS) -c src/SplitTool.cc -o bin/SplitTool.o -I include

# 其它所有 .o
bin/InetAddress.o: src/InetAddress.cc
	g++ -g $(GLOG_FLAGS) -c src/InetAddress.cc -o bin/InetAddress.o -I include

bin/ProtocolParser.o: src/ProtocolParser.cc include/ProtocolParser.h
	g++ -g $(GLOG_FLAGS) -c src/ProtocolParser.cc -o bin/ProtocolParser.o -I include

bin/Configuration.o: src/Configuration.cc include/Configuration.h
	g++ -g $(GLOG_FLAGS) -c src/Configuration.cc -o bin/Configuration.o -I include

bin/SearchEngineServer.o: src/SearchEngineServer.cc include/SearchEngineServer.h
	g++ -g $(GLOG_FLAGS) -c src/SearchEngineServer.cc -o bin/SearchEngineServer.o -I include

bin/Timestamp.o: src/Timestamp.cc include/Timestamp.h
	g++ -g $(GLOG_FLAGS) -c src/Timestamp.cc -o bin/Timestamp.o -I include

bin/Thread.o: src/Thread.cc include/Thread.h
	g++ -g $(GLOG_FLAGS) -c src/Thread.cc -o bin/Thread.o -I include

bin/TcpServer.o: src/TcpServer.cc include/TcpServer.h
	g++ -g $(GLOG_FLAGS) -c src/TcpServer.cc -o bin/TcpServer.o -I include

bin/TcpConnection.o: src/TcpConnection.cc include/TcpConnection.h
	g++ -g $(GLOG_FLAGS) -c src/TcpConnection.cc -o bin/TcpConnection.o -I include

bin/Socket.o: src/Socket.cc include/Socket.h
	g++ -g $(GLOG_FLAGS) -c src/Socket.cc -o bin/Socket.o -I include

bin/Poller.o: src/Poller.cc include/Poller.h
	g++ -g $(GLOG_FLAGS) -c src/Poller.cc -o bin/Poller.o -I include

# Logger.cc 不再需要编译 (使用glog)
# bin/Logger.o: src/Logger.cc include/Logger.h
# 	g++ -g $(GLOG_FLAGS) -c src/Logger.cc -o bin/Logger.o -I include

bin/EventLoopThreadPool.o: src/EventLoopThreadPool.cc include/EventLoopThreadPool.h
	g++ -g $(GLOG_FLAGS) -c src/EventLoopThreadPool.cc -o bin/EventLoopThreadPool.o -I include

bin/EventLoopThread.o: src/EventLoopThread.cc include/EventLoopThread.h
	g++ -g $(GLOG_FLAGS) -c src/EventLoopThread.cc -o bin/EventLoopThread.o -I include

bin/EventLoop.o: src/EventLoop.cc include/EventLoop.h
	g++ -g $(GLOG_FLAGS) -c src/EventLoop.cc -o bin/EventLoop.o -I include

bin/EPollPoller.o: src/EPollPoller.cc include/EPollPoller.h
	g++ -g $(GLOG_FLAGS) -c src/EPollPoller.cc -o bin/EPollPoller.o -I include

bin/DefaultPoller.o: src/DefaultPoller.cc
	g++ -g $(GLOG_FLAGS) -c src/DefaultPoller.cc -o bin/DefaultPoller.o -I include

bin/CurrentThread.o: src/CurrentThread.cc include/CurrentThread.h
	g++ -g $(GLOG_FLAGS) -c src/CurrentThread.cc -o bin/CurrentThread.o -I include

bin/Channel.o: src/Channel.cc include/Channel.h
	g++ -g $(GLOG_FLAGS) -c src/Channel.cc -o bin/Channel.o -I include

bin/Buffer.o: src/Buffer.cc include/Buffer.h
	g++ -g $(GLOG_FLAGS) -c src/Buffer.cc -o bin/Buffer.o -I include

bin/Acceptor.o: src/Acceptor.cc include/Acceptor.h
	g++ -g $(GLOG_FLAGS) -c src/Acceptor.cc -o bin/Acceptor.o -I include

bin/Lexicon.o: src/Lexicon.cc include/Lexicon.h
	g++ -g $(GLOG_FLAGS) -c src/Lexicon.cc -o bin/Lexicon.o -I include

bin/KeyRecommander.o: src/KeyRecommander.cc include/KeyRecommander.h
	g++ -g $(GLOG_FLAGS) -c src/KeyRecommander.cc -o bin/KeyRecommander.o -I include

bin/PageLibPreprocessor.o: src/PageLibPreprocessor.cc include/PageLibPreprocessor.h
	g++ -g $(GLOG_FLAGS) -c src/PageLibPreprocessor.cc -o bin/PageLibPreprocessor.o -I include

bin/WebPageSearcher.o: src/WebPageSearcher.cc include/WebPageSearcher.h
	g++ -g $(GLOG_FLAGS) -c src/WebPageSearcher.cc -o bin/WebPageSearcher.o -I include -I include/redis++

bin/RedisCache.o: src/RedisCache.cc include/RedisCache.h
	g++ -g $(GLOG_FLAGS) -c src/RedisCache.cc -o bin/RedisCache.o -I include -I include/redis++ -I /usr/local/include

# BuildIndex 目标 - 独立的索引构建程序（包含SimHash去重）
bin/BuildIndex: bin/BuildIndex.o bin/RssReader.o bin/tinyxml2.o bin/PageLibPreprocessor.o bin/SplitToolCppJieba.o bin/SplitTool.o bin/Configuration.o bin/DictProducer.o bin/Lexicon.o bin/Timestamp.o bin/Deduplication.o
	g++ -g $(GLOG_FLAGS) -o bin/BuildIndex bin/BuildIndex.o bin/RssReader.o bin/tinyxml2.o bin/PageLibPreprocessor.o bin/SplitToolCppJieba.o bin/SplitTool.o bin/Configuration.o bin/DictProducer.o bin/Lexicon.o bin/Timestamp.o bin/Deduplication.o -pthread $(GLOG_LIBS)

bin/BuildIndex.o: src/BuildIndex.cc
	g++ -g $(GLOG_FLAGS) -c src/BuildIndex.cc -o bin/BuildIndex.o -I include -I RSS -I simhash

bin/Deduplication.o: simhash/Deduplication.cc simhash/Deduplication.h
	g++ -g $(GLOG_FLAGS) -c simhash/Deduplication.cc -o bin/Deduplication.o -I include -I simhash

bin/RssReader.o: RSS/rss_reader.cpp RSS/rss_reader.h
	g++ -g $(GLOG_FLAGS) -c RSS/rss_reader.cpp -o bin/RssReader.o -I include -I RSS

bin/tinyxml2.o: RSS/tinyxml2.cpp RSS/tinyxml2.h
	g++ -g $(GLOG_FLAGS) -c RSS/tinyxml2.cpp -o bin/tinyxml2.o -I RSS

# SearchEngine 可执行文件（排除含有 main 的 DataClean.o、CreateDict.o、Logger.o）
bin/SearchEngine: bin/InetAddress.o bin/ProtocolParser.o bin/Configuration.o bin/SearchEngineServer.o bin/Timestamp.o bin/Thread.o bin/TcpServer.o bin/TcpConnection.o bin/Socket.o bin/Poller.o bin/EventLoopThreadPool.o bin/EventLoopThread.o bin/EventLoop.o bin/EPollPoller.o bin/DefaultPoller.o bin/CurrentThread.o bin/Channel.o bin/Buffer.o bin/Acceptor.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o bin/Lexicon.o bin/KeyRecommander.o bin/PageLibPreprocessor.o bin/WebPageSearcher.o bin/RedisCache.o
	g++ -g $(GLOG_FLAGS) -o bin/SearchEngine bin/InetAddress.o bin/ProtocolParser.o bin/Configuration.o bin/SearchEngineServer.o bin/Timestamp.o bin/Thread.o bin/TcpServer.o bin/TcpConnection.o bin/Socket.o bin/Poller.o bin/EventLoopThreadPool.o bin/EventLoopThread.o bin/EventLoop.o bin/EPollPoller.o bin/DefaultPoller.o bin/CurrentThread.o bin/Channel.o bin/Buffer.o bin/Acceptor.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o bin/Lexicon.o bin/KeyRecommander.o bin/PageLibPreprocessor.o bin/WebPageSearcher.o bin/RedisCache.o -pthread -L/usr/local/lib -lhiredis -lredis++ -Wl,-rpath,/usr/local/lib $(GLOG_LIBS)

clean:
	rm -f bin/DataClean bin/CreateDict bin/SearchEngine bin/BuildIndex
	rm -f bin/*.o
	rm -rf log/*

rebuild: clean All

.PHONY: All clean rebuild
