/**
 * @file file_control.cpp
 * @brief 文件控制实现
 */

#include "file/file_control.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <unistd.h>
#include <pwd.h>
#include <cstdlib> // for system

namespace fs = std::filesystem;

FileControl::FileControl() {
    // 默认操作用户主目录
    const char* homeDir;
    if ((homeDir = getenv("HOME")) == NULL) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    currentRootPath = std::string(homeDir);
}

FileControl::~FileControl() {}

// 实时搜索文件
std::vector<std::string> FileControl::searchFile(const std::string& keyword) {
    std::vector<std::string> results;
    std::string keyLower = keyword;
    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);

    // 跳过权限不足的目录
    auto options = fs::directory_options::skip_permission_denied;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(currentRootPath, options)) {
            try {
                std::string fname = entry.path().filename().string();
                std::string fnameLower = fname;
                std::transform(fnameLower.begin(), fnameLower.end(), fnameLower.begin(), ::tolower);

                // 包含匹配
                if (fnameLower.find(keyLower) != std::string::npos) {
                    results.push_back(entry.path().string());
                    if (results.size() >= 10) break; // 限制返回10个，防止太多
                }
            } catch (...) { continue; }
        }
    } catch (...) {}
    return results;
}

// 打开文件 (使用 xdg-open)
bool FileControl::openFile(const std::string& filePath) {
    if (!fs::exists(filePath)) return false;
    
    // 构造命令：xdg-open "path" > /dev/null 2>&1 &
    // 后台运行，且屏蔽输出
    std::string cmd = "xdg-open \"" + filePath + "\" > /dev/null 2>&1 &";
    int ret = std::system(cmd.c_str());
    return (ret == 0);
}

// 删除文件
bool FileControl::deleteFile(const std::string& filePath) {
    try {
        if (fs::exists(filePath) && fs::is_regular_file(filePath)) {
            return fs::remove(filePath);
        }
    } catch (...) {
        return false;
    }
    return false;
}