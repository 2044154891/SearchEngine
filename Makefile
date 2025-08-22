All: bin bin/DataClean bin/CreateDict bin/SearchEngine

bin:
	mkdir -p bin

# DataClean 目标
bin/DataClean: bin/DataClean.o
	g++ -g -o bin/DataClean bin/DataClean.o

bin/DataClean.o: src/DataClean.cc
	g++ -g -c src/DataClean.cc -o bin/DataClean.o -I include

# CreateDict 目标
bin/CreateDict: bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o
	g++ -g -o bin/CreateDict bin/CreateDict.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o

bin/CreateDict.o: src/CreateDict.cc
	g++ -g -c src/CreateDict.cc -o bin/CreateDict.o -I include

bin/DictProducer.o: src/DictProducer.cc include/DictProducer.h
	g++ -g -c src/DictProducer.cc -o bin/DictProducer.o -I include

bin/SplitToolCppJieba.o: src/SplitToolCppJieba.cc include/SplitToolCppJieba.h
	g++ -g -c src/SplitToolCppJieba.cc -o bin/SplitToolCppJieba.o -I include

bin/SplitTool.o: src/SplitTool.cc include/SplitTool.h
	g++ -g -c src/SplitTool.cc -o bin/SplitTool.o -I include

# 其它所有 .o
bin/InetAddress.o: src/InetAddress.cc
	g++ -g -c src/InetAddress.cc -o bin/InetAddress.o -I include

bin/ProtocolParser.o: src/ProtocolParser.cc include/ProtocolParser.h
	g++ -g -c src/ProtocolParser.cc -o bin/ProtocolParser.o -I include

bin/Configuration.o: src/Configuration.cc include/Configuration.h
	g++ -g -c src/Configuration.cc -o bin/Configuration.o -I include

bin/SearchEngineServer.o: src/SearchEngineServer.cc include/SearchEngineServer.h
	g++ -g -c src/SearchEngineServer.cc -o bin/SearchEngineServer.o -I include

bin/Timestamp.o: src/Timestamp.cc include/Timestamp.h
	g++ -g -c src/Timestamp.cc -o bin/Timestamp.o -I include

bin/Thread.o: src/Thread.cc include/Thread.h
	g++ -g -c src/Thread.cc -o bin/Thread.o -I include

bin/TcpServer.o: src/TcpServer.cc include/TcpServer.h
	g++ -g -c src/TcpServer.cc -o bin/TcpServer.o -I include

bin/TcpConnection.o: src/TcpConnection.cc include/TcpConnection.h
	g++ -g -c src/TcpConnection.cc -o bin/TcpConnection.o -I include

bin/Socket.o: src/Socket.cc include/Socket.h
	g++ -g -c src/Socket.cc -o bin/Socket.o -I include

bin/Poller.o: src/Poller.cc include/Poller.h
	g++ -g -c src/Poller.cc -o bin/Poller.o -I include

bin/Logger.o: src/Logger.cc include/Logger.h
	g++ -g -c src/Logger.cc -o bin/Logger.o -I include

bin/EventLoopThreadPool.o: src/EventLoopThreadPool.cc include/EventLoopThreadPool.h
	g++ -g -c src/EventLoopThreadPool.cc -o bin/EventLoopThreadPool.o -I include

bin/EventLoopThread.o: src/EventLoopThread.cc include/EventLoopThread.h
	g++ -g -c src/EventLoopThread.cc -o bin/EventLoopThread.o -I include

bin/EventLoop.o: src/EventLoop.cc include/EventLoop.h
	g++ -g -c src/EventLoop.cc -o bin/EventLoop.o -I include

bin/EPollPoller.o: src/EPollPoller.cc include/EPollPoller.h
	g++ -g -c src/EPollPoller.cc -o bin/EPollPoller.o -I include

bin/DefaultPoller.o: src/DefaultPoller.cc
	g++ -g -c src/DefaultPoller.cc -o bin/DefaultPoller.o -I include

bin/CurrentThread.o: src/CurrentThread.cc include/CurrentThread.h
	g++ -g -c src/CurrentThread.cc -o bin/CurrentThread.o -I include

bin/Channel.o: src/Channel.cc include/Channel.h
	g++ -g -c src/Channel.cc -o bin/Channel.o -I include

bin/Buffer.o: src/Buffer.cc include/Buffer.h
	g++ -g -c src/Buffer.cc -o bin/Buffer.o -I include

bin/Acceptor.o: src/Acceptor.cc include/Acceptor.h
	g++ -g -c src/Acceptor.cc -o bin/Acceptor.o -I include

bin/Lexicon.o: src/Lexicon.cc include/Lexicon.h
	g++ -g -c src/Lexicon.cc -o bin/Lexicon.o -I include

bin/KeyRecommander.o: src/KeyRecommander.cc include/KeyRecommander.h
	g++ -g -c src/KeyRecommander.cc -o bin/KeyRecommander.o -I include

# SearchEngine 可执行文件（排除含有 main 的 DataClean.o、CreateDict.o）
bin/SearchEngine: bin/InetAddress.o bin/ProtocolParser.o bin/Configuration.o bin/SearchEngineServer.o bin/Timestamp.o bin/Thread.o bin/TcpServer.o bin/TcpConnection.o bin/Socket.o bin/Poller.o bin/Logger.o bin/EventLoopThreadPool.o bin/EventLoopThread.o bin/EventLoop.o bin/EPollPoller.o bin/DefaultPoller.o bin/CurrentThread.o bin/Channel.o bin/Buffer.o bin/Acceptor.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o bin/Lexicon.o bin/KeyRecommander.o
	g++ -g -o bin/SearchEngine bin/InetAddress.o bin/ProtocolParser.o bin/Configuration.o bin/SearchEngineServer.o bin/Timestamp.o bin/Thread.o bin/TcpServer.o bin/TcpConnection.o bin/Socket.o bin/Poller.o bin/Logger.o bin/EventLoopThreadPool.o bin/EventLoopThread.o bin/EventLoop.o bin/EPollPoller.o bin/DefaultPoller.o bin/CurrentThread.o bin/Channel.o bin/Buffer.o bin/Acceptor.o bin/DictProducer.o bin/SplitToolCppJieba.o bin/SplitTool.o bin/Lexicon.o bin/KeyRecommander.o -pthread

clean:
	rm -f bin/DataClean bin/CreateDict bin/SearchEngine
	rm -f bin/*.o

rebuild: clean All

.PHONY: All clean rebuild