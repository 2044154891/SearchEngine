#include "FileUtils.h"
#include "Config.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <dirent.h>
#include <sys/stat.h>

using std::cin;
using std::cout;
using std::string;
using std::cerr;
using std::ifstream;
using std::ofstream;
using std::system;
using std::exception;

// 函数声明
void process_directory(const string& input_dir, const string& output_dir, const string& mode);

void English_Clean(string filename){
    ifstream in(filename);
    if (!in){
        cout << "Error: " << filename << " not found" << "\n";
        return;
    }
    string output_filename = filename.substr(0, filename.find_last_of(".")) + "_cleaned.txt";
    ofstream out(output_filename);
    if (!out){
        cout << "Error: " << output_filename << " not found" << "\n";
        return;
    }
    string line;
    while (getline(in, line)){
        string cleaned_line;
        //处理每一行
        for (char c : line) {
            if (isalpha(c)) {
                cleaned_line += tolower(c);
            } else if (isdigit(c)) {
                cleaned_line += c;
            } else {
                cleaned_line += ' ';
            }
        }
        out << cleaned_line << '\n';
    }

    in.close();
    out.close();
    cout << "Cleaned file saved to: " << output_filename << "\n";
}

void Chinese_Clean(string filename){
    ifstream in(filename);
    if (!in){
        cout << "Error: " << filename << " not found" << "\n";
        return;
    }
    
    // 创建输出文件名
    string output_filename = filename.substr(0, filename.find_last_of('.')) + "_cleaned.txt";
    ofstream out(output_filename);
    if (!out){
        cout << "Error: Cannot create output file: " << output_filename << "\n";
        return;
    }
    
    string line;
    while (getline(in, line)) {
        // 使用更安全的方法去除换行符
        string cleaned_line = "";
        size_t i = 0;
        while (i < line.length()) {
            // 检查是否是UTF-8字符的开始
            unsigned char c = static_cast<unsigned char>(line[i]);
            
            if (c == '\r' || c == '\n') {
                // 单独的\r或\n，直接跳过
                i++;
            } else if ((c & 0xE0) == 0xC0) {
                // 2字节UTF-8字符
                if (i + 1 < line.length()) {
                    cleaned_line += line.substr(i, 2);
                    i += 2;
                } else {
                    // 不完整的字符，跳过
                    i++;
                }
            } else if ((c & 0xF0) == 0xE0) {
                // 3字节UTF-8字符（包括中文）
                if (i + 2 < line.length()) {
                    cleaned_line += line.substr(i, 3);
                    i += 3;
                } else {
                    // 不完整的字符，跳过
                    i++;
                }
            } else if ((c & 0xF8) == 0xF0) {
                // 4字节UTF-8字符
                if (i + 3 < line.length()) {
                    cleaned_line += line.substr(i, 4);
                    i += 4;
                } else {
                    // 不完整的字符，跳过
                    i++;
                }
            } else if ((c & 0x80) == 0x00) {
                // ASCII字符
                cleaned_line += line[i];
                i++;
            } else {
                // 无效的UTF-8字节，跳过
                i++;
            }
        }
        
        // 如果清理后的行不为空，写入输出文件
        if (!cleaned_line.empty()) {
            out << cleaned_line;
        }
    }
    
    in.close();
    out.close();
    cout << "Chinese file cleaned successfully. Output saved to: " << output_filename << "\n";
}


// 处理目录下的所有文件
void process_directory(const string& input_dir, const string& output_dir, const string& mode) {
    DIR* dir = opendir(input_dir.c_str());
    if (!dir) {
        cerr << "Error: Cannot open directory: " << input_dir << "\n";
        return;
    }
    
    struct dirent* entry;
    int processed_count = 0;
    int total_count = 0;
    
    while ((entry = readdir(dir)) != nullptr) {
        string filename = entry->d_name;
        
        // 跳过 . 和 .. 目录
        if (filename == "." || filename == "..") {
            continue;
        }
        
        string full_path = input_dir + "/" + filename;
        
        // 只处理普通文件
        if (!is_regular_file(full_path)) {
            continue;
        }
        
        // 跳过隐藏文件
        if (filename[0] == '.') {
            continue;
        }
        
        total_count++;
        cout << "Processing: " << filename << "\n";
        
        try {
            if (mode == "chinese") {
                Chinese_Clean(full_path);
            } else if (mode == "english") {
                English_Clean(full_path);
            }
            
            // 移动清理后的文件到输出目录
            string cleaned_filename = filename.substr(0, filename.find_last_of('.')) + "_cleaned.txt";
            string source_path = full_path.substr(0, full_path.find_last_of('.')) + "_cleaned.txt";
            string dest_path = output_dir + "/" + cleaned_filename;
            
            // 检查源文件是否存在
            ifstream check_file(source_path);
            if (!check_file) {
                cerr << "  -> Error: Cleaned file not found: " << source_path << "\n";
                continue;
            }
            check_file.close();
            
            // 移动文件
            string move_cmd = "mv \"" + source_path + "\" \"" + dest_path + "\"";
            int move_result = system(move_cmd.c_str());
            
            if (move_result == 0) {
                processed_count++;
                cout << "  -> Successfully processed and moved to: " << dest_path << "\n";
            } else {
                cerr << "  -> Error moving file to: " << dest_path << "\n";
            }
            
        } catch (const exception& e) {
            cerr << "  -> Error processing " << filename << ": " << e.what() << "\n";
        }
    }
    
    closedir(dir);
    
    cout << "Directory processing completed: " << processed_count << "/" << total_count << " files processed successfully.\n";
}

// 读取配置文件的函数

int main(int argc, char *argv[]){
    // 读取配置文件
    string config_file = Config::configFilePath;
    
    const string chinese_corpus_path = read_config_value(config_file, "chinese_corpus_path");
    const string english_corpus_path = read_config_value(config_file, "english_corpus_path");
    const string cleaned_corpus_path = read_config_value(config_file, "cleaned_corpus_path");
    
    // 检查是否成功读取配置
    if (chinese_corpus_path.empty() || english_corpus_path.empty() || cleaned_corpus_path.empty()) {
        cerr << "Error: Failed to read configuration from " << config_file << "\n";
        return 1;
    }
    
    cout << "Configuration loaded successfully:" << "\n";
    cout << "Chinese corpus path: " << chinese_corpus_path << "\n";
    cout << "English corpus path: " << english_corpus_path << "\n";
    cout << "Cleaned corpus path: " << cleaned_corpus_path << "\n";
    
    // 创建输出目录（如果不存在）
    string mkdir_cmd = "mkdir -p " + cleaned_corpus_path;
    system(mkdir_cmd.c_str());
    
    cout << "\nReady to process files..." << "\n";
    
    const string cleaned_corpus_path_chinese = cleaned_corpus_path + "/chinese";
    const string cleaned_corpus_path_english = cleaned_corpus_path + "/english";

    // 创建输出目录（如果不存在）
    mkdir_cmd = "mkdir -p " + cleaned_corpus_path_chinese;
    system(mkdir_cmd.c_str());
    mkdir_cmd = "mkdir -p " + cleaned_corpus_path_english;
    system(mkdir_cmd.c_str());
    
    // 处理中文文件
    cout << "\nProcessing Chinese files..." << "\n";
    process_directory(chinese_corpus_path, cleaned_corpus_path_chinese, "chinese");
    
    // 处理英文文件
    cout << "\nProcessing English files..." << "\n";
    process_directory(english_corpus_path, cleaned_corpus_path_english, "english");
    
    cout << "\nAll files processed successfully!" << "\n";
    
    return 0;
}