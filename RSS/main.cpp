#include "rss_reader.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::string rssFile = "coolshell.xml";
    
    // 如果提供了命令行参数，使用提供的文件名
    if (argc > 1) {
        rssFile = argv[1];
    }
    
    std::cout << "RSS Parser using TinyXML2" << std::endl;
    std::cout << "Parsing RSS file: " << rssFile << std::endl;
    
    RssReader reader;
    
    try {
        // 解析RSS文件
        reader.parseRss(rssFile);
        
        // 输出到pagelib.txt
        reader.dump("pagelib.txt");
        
        std::cout << "RSS parsing completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}