All: bin bin/DataClean bin/CreateDict bin/SearchEngine

bin:
	mkdir -p bin

# DataClean 目标
bin/DataClean: bin/DataClean.o
	g++ -o bin/DataClean bin/DataClean.o

bin/DataClean.o: src/DataClean.cc
	g++ -c src/DataClean.cc -o bin/DataClean.o -I include

# CreateDict 目标
bin/CreateDict: bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o
	g++ -o bin/CreateDict bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o

bin/CreateDict.o: src/CreateDict.cc
	g++ -c src/CreateDict.cc -o bin/CreateDict.o -I include

bin/DictProducer.o: src/DictProducer.cc include/DictProducer.h
	g++ -c src/DictProducer.cc -o bin/DictProducer.o -I include

bin/SplitToolCppJieba.o: src/SplitToolCppJieba.cc include/SplitToolCppJieba.h
	g++ -c src/SplitToolCppJieba.cc -o bin/SplitToolCppJieba.o -I include

bin/SplitTool.o: src/SplitTool.cc include/SplitTool.h
	g++ -c src/SplitTool.cc -o bin/SplitTool.o -I include

# 其它所有 .o
bin/InetAddress.o: src/InetAddress.cc
	g++ -c src/InetAddress.cc -o bin/InetAddress.o -I include

bin/ProtocolParser.o: src/ProtocolParser.cc include/ProtocolParser.h
	g++ -c src/ProtocolParser.cc -o bin/ProtocolParser.o -I include

bin/Configuration.o: src/Configuration.cc include/Configuration.h
	g++ -c src/Configuration.cc -o bin/Configuration.o -I include

bin/SearchEngineServer.o: src/SearchEngineServer.cc include/SearchEngineServer.h
	g++ -c src/SearchEngineServer.cc -o bin/SearchEngineServer.o -I include

bin/Timestamp.o: src/Timestamp.cc include/Timestamp.h
	g++ -c src/Timestamp.cc -o bin/Timestamp.o -I include

bin/Thread.o: src/Thread.cc include/Thread.h
	g++ -c src/Thread.cc -o bin/Thread.o -I include

bin/TcpServer.o: src/TcpServer.cc include/TcpServer.h
	g++ -c src/TcpServer.cc -o bin/TcpServer.o -I include

bin/TcpConnection.o: src/TcpConnection.cc include/TcpConnection.h
	g++ -c src/TcpConnection.cc -o bin/TcpConnection.o -I include

bin/Socket.o: src/Socket.cc include/Socket.h
	g++ -c src/Socket.cc -o bin/Socket.o -I include

bin/Poller.o: src/Poller.cc include/Poller.h
	g++ -c src/Poller.cc -o bin/Poller.o -I include

bin/Logger.o: src/Logger.cc include/Logger.h
	g++ -c src/Logger.cc -o bin/Logger.o -I include

bin/EventLoopThreadPool.o: src/EventLoopThreadPool.cc include/EventLoopThreadPool.h
	g++ -c src/EventLoopThreadPool.cc -o bin/EventLoopThreadPool.o -I include

bin/EventLoopThread.o: src/EventLoopThread.cc include/EventLoopThread.h
	g++ -c src/EventLoopThread.cc -o bin/EventLoopThread.o -I include

bin/EventLoop.o: src/EventLoop.cc include/EventLoop.h
	g++ -c src/EventLoop.cc -o bin/EventLoop.o -I include

bin/EPollPoller.o: src/EPollPoller.cc include/EPollPoller.h
	g++ -c src/EPollPoller.cc -o bin/EPollPoller.o -I include

bin/DefaultPoller.o: src/DefaultPoller.cc
	g++ -c src/DefaultPoller.cc -o bin/DefaultPoller.o -I include

bin/CurrentThread.o: src/CurrentThread.cc include/CurrentThread.h
	g++ -c src/CurrentThread.cc -o bin/CurrentThread.o -I include

bin/Channel.o: src/Channel.cc include/Channel.h
	g++ -c src/Channel.cc -o bin/Channel.o -I include

bin/Buffer.o: src/Buffer.cc include/Buffer.h
	g++ -c src/Buffer.cc -o bin/Buffer.o -I include

bin/Acceptor.o: src/Acceptor.cc include/Acceptor.h
	g++ -c src/Acceptor.cc -o bin/Acceptor.o -I include

bin/Lexicon.o: src/Lexicon.cc include/Lexicon.h
	g++ -c src/Lexicon.cc -o bin/Lexicon.o -I include

bin/KeyRecommander.o: src/KeyRecommander.cc include/KeyRecommander.h
	g++ -c src/KeyRecommander.cc -o bin/KeyRecommander.o -I include

# SearchEngine 可执行文件（排除含有 main 的 DataClean.o、CreateDict.o）
bin/SearchEngine: bin/InetAddress.o bin/ProtocolParser.o bin/Configuration.o bin/SearchEngineServer.o bin/Timestamp.o bin/Thread.o bin/TcpServer.o bin/TcpConnection.o bin/Socket.o bin/Poller.o bin/Logger.o bin/EventLoopThreadPool.o bin/EventLoopThread.o bin/EventLoop.o bin/EPollPoller.o bin/DefaultPoller.o bin/CurrentThread.o bin/Channel.o bin/Buffer.o bin/Acceptor.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o
	g++ -o bin/SearchEngine bin/InetAddress.o bin/ProtocolParser.o bin/Configuration.o bin/SearchEngineServer.o bin/Timestamp.o bin/Thread.o bin/TcpServer.o bin/TcpConnection.o bin/Socket.o bin/Poller.o bin/Logger.o bin/EventLoopThreadPool.o bin/EventLoopThread.o bin/EventLoop.o bin/EPollPoller.o bin/DefaultPoller.o bin/CurrentThread.o bin/Channel.o bin/Buffer.o bin/Acceptor.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o bin/Lexicon.o bin/KeyRecommander.o -pthread

clean:
	rm -f bin/DataClean bin/CreateDict bin/SearchEngine
	rm -f bin/*.o

rebuild: clean All

.PHONY: All clean rebuild