#include "rss_reader.h"
#include "../include/FileUtils.h"
#include "../include/Config.h"

#include <iostream>


int main(int argc, char* argv[])
{
    std::string conf = Config::configFilePath;
    std::string rssCorpusDir = read_config_value(conf, "rss_corpus_dir");
    std::string rawPageStorePath = read_config_value(conf, "raw_page_store_path");
    std::string rawPageOffsetPath = read_config_value(conf, "raw_page_offset_path");
    
    RssReader rssReader(rssCorpusDir, rawPageStorePath, rawPageOffsetPath);
    rssReader.processAll();
    
    return 0;
}