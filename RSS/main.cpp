#include "rss_reader.h"
#include "../include/FileUtils.h"
#include "../include/Config.h"

#include <iostream>


int main(int argc, char* argv[])
{
    std::string conf = Config::configFilePath;
    std::string ripedir = read_config_value(conf, "webpage_path");
    std::string storedir = read_config_value(conf, "old_webpage_path");
    std::string offsetfile = read_config_value(conf, "old_webpage_offset_path");
    
    RssReader rssReader(ripedir, storedir, offsetfile);
    rssReader.processAll();
    
    return 0;
}