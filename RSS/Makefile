CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -I../include -I.
TARGET = rss_parser
SOURCES = main.cpp rss_reader.cpp tinyxml2.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# 默认目标
all: $(TARGET)

# 链接目标文件
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

# 编译源文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJECTS) $(TARGET) pagelib.txt offsetlib.txt

# 运行程序
run: $(TARGET)
	./$(TARGET)

# 扫描某目录并生成
run_dir: $(TARGET)
	./$(TARGET)

# 安装依赖（如果需要）
install:
	# 这里可以添加安装tinyxml2库的命令
	@echo "TinyXML2 library should be available in the project directory"

.PHONY: all clean run install