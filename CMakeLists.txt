cmake_minimum_required(VERSION 3.10)
project(SearchEngine VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 输出目录设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 查找线程库
find_package(Threads REQUIRED)

# ==========================================
# 第三方库设置
# ==========================================

# 1. glog (Google日志库)
find_library(GLOG_LIBRARY glog REQUIRED)
message(STATUS "Found glog: ${GLOG_LIBRARY}")

# 2. hiredis (Redis C客户端)
find_library(HIREDIS_LIBRARY hiredis REQUIRED)
message(STATUS "Found hiredis: ${HIREDIS_LIBRARY}")

# 3. redis++ (Redis C++客户端)
# 注意: redis++ 需要手动指定头文件和库
set(REDIS_PLUS_PLUS_INCLUDE_DIR "/usr/local/include")
set(REDIS_PLUS_PLUS_LIBRARY "/usr/local/lib/libredis++.so")
if(NOT EXISTS ${REDIS_PLUS_PLUS_LIBRARY})
    set(REDIS_PLUS_PLUS_LIBRARY "/usr/local/lib/libredis++.a")
endif()
message(STATUS "Using redis++: ${REDIS_PLUS_PLUS_LIBRARY}")

# 4. nlohmann/json (头文件已在include目录)
# 5. cppjieba (头文件已在include目录)
# 6. tinyxml2 (在RSS目录)

# 添加项目头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/cppjieba
    ${CMAKE_SOURCE_DIR}/include/cppjieba/limonp
    ${CMAKE_SOURCE_DIR}/include/nlohmann
    ${CMAKE_SOURCE_DIR}/include/redis++
    ${CMAKE_SOURCE_DIR}/RSS
    ${CMAKE_SOURCE_DIR}/simhash
    ${REDIS_PLUS_PLUS_INCLUDE_DIR}
)

# 库目录
link_directories(/usr/local/lib)

# ==========================================
# 子目录
# ==========================================
add_subdirectory(src)
add_subdirectory(RSS)
add_subdirectory(simhash)

# 创建log目录
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/log)
