# ==========================================
# 公共源文件 (被多个目标使用)
# ==========================================
set(COMMON_SOURCES
    Acceptor.cc
    Buffer.cc
    Channel.cc
    Configuration.cc
    CurrentThread.cc
    DictProducer.cc
    EventLoop.cc
    EventLoopThread.cc
    EventLoopThreadPool.cc
    InetAddress.cc
    KeyRecommander.cc
    Lexicon.cc
    PageLibPreprocessor.cc
    Poller.cc
    ProtocolParser.cc
    RedisCache.cc
    SearchEngineServer.cc
    Socket.cc
    SplitTool.cc
    SplitToolCppJieba.cc
    TcpConnection.cc
    TcpServer.cc
    Thread.cc
    Timestamp.cc
    WebPage.cc
    WebPageSearcher.cc
    EPollPoller.cc
    DefaultPoller.cc
)

# ==========================================
# 1. DataClean - 数据清洗
# ==========================================
add_executable(DataClean DataClean.cc)
target_link_libraries(DataClean
    ${GLOG_LIBRARY}
    Threads::Threads
)

# ==========================================
# 2. CreateDict - 创建词典
# ==========================================
add_executable(CreateDict 
    CreateDict.cc
    ${CMAKE_SOURCE_DIR}/src/DictProducer.cc
    ${CMAKE_SOURCE_DIR}/src/SplitTool.cc
    ${CMAKE_SOURCE_DIR}/src/SplitToolCppJieba.cc
)
target_link_libraries(CreateDict
    ${GLOG_LIBRARY}
    Threads::Threads
)

# ==========================================
# 3. BuildIndex - 构建索引 (包含RSS解析和SimHash去重)
# ==========================================
add_executable(BuildIndex 
    BuildIndex.cc
    ${CMAKE_SOURCE_DIR}/RSS/rss_reader.cpp
    ${CMAKE_SOURCE_DIR}/RSS/tinyxml2.cpp
    ${CMAKE_SOURCE_DIR}/simhash/Deduplication.cc
    ${CMAKE_SOURCE_DIR}/src/PageLibPreprocessor.cc
    ${CMAKE_SOURCE_DIR}/src/SplitToolCppJieba.cc
    ${CMAKE_SOURCE_DIR}/src/SplitTool.cc
    ${CMAKE_SOURCE_DIR}/src/Configuration.cc
    ${CMAKE_SOURCE_DIR}/src/DictProducer.cc
    ${CMAKE_SOURCE_DIR}/src/Lexicon.cc
    ${CMAKE_SOURCE_DIR}/src/Timestamp.cc
)
target_link_libraries(BuildIndex
    ${GLOG_LIBRARY}
    Threads::Threads
)

# ==========================================
# 4. SearchEngine - 主服务器
# ==========================================
add_executable(SearchEngine
    ${COMMON_SOURCES}
)
target_link_libraries(SearchEngine
    ${GLOG_LIBRARY}
    ${HIREDIS_LIBRARY}
    ${REDIS_PLUS_PLUS_LIBRARY}
    Threads::Threads
)

# 设置rpath
set_target_properties(SearchEngine PROPERTIES
    BUILD_RPATH "/usr/local/lib"
    INSTALL_RPATH "/usr/local/lib"
)
