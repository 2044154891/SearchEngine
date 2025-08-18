#ifndef _FILEUTILS_H
#define _FILEUTILS_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <dirent.h>
#include <sys/stat.h>

using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::cout;
using std::cerr;

// 检查文件是否为普通文件
bool is_regular_file(const string& path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        return false;
    }
    return S_ISREG(path_stat.st_mode);
}

// 获取文件扩展名
string get_file_extension(const string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != string::npos) {
        return filename.substr(pos + 1);
    }
    return "";
}

// 获取文件名
string get_file_name(const string& path) {
    size_t pos = path.find_last_of('/');
    if (pos != string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}


// 读取配置文件的函数
string read_config_value(const string& config_file, const string& key) {
    ifstream config(config_file);
    if (!config) {
        cerr << "Error: Cannot open config file: " << config_file << "\n";
        return "";
    }
    
    string line;
    while (getline(config, line)) {
        // 跳过注释行和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 查找等号
        size_t pos = line.find('=');
        if (pos != string::npos) {
            string config_key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            
            // 去除前后空格
            config_key.erase(0, config_key.find_first_not_of(" \t"));
            config_key.erase(config_key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (config_key == key) {
                config.close();
                return value;
            }
        }
    }
    
    config.close();
    return "";
}


#endif //_FILEUTILS_H