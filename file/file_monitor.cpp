/**
 * @file file_monitor.cpp
 * @brief 文件监控实现 (纯净版)
 */

#include "file/file_monitor.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <unistd.h>
#include <pwd.h>

namespace fs = std::filesystem;

FileMonitor::FileMonitor() {
    const char* homeDir;
    if ((homeDir = getenv("HOME")) == NULL) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
    currentRootPath = std::string(homeDir);
}

FileMonitor::~FileMonitor() {}

std::string FileMonitor::getCurrentRoot() {
    return currentRootPath;
}

std::string FileMonitor::formatSize(uintmax_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double dblBytes = static_cast<double>(bytes);
    while (dblBytes > 1024 && i < 4) {
        dblBytes /= 1024.0;
        i++;
    }
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", dblBytes, suffixes[i]);
    return std::string(buffer);
}

int FileMonitor::scanDirectory(const std::string& rootPath) {
    fileIndex.clear();
    currentRootPath = rootPath;
    int count = 0;
    auto options = fs::directory_options::skip_permission_denied;

    try {
        // 递归遍历，只索引 > 10MB 的文件
        for (const auto& entry : fs::recursive_directory_iterator(rootPath, options)) {
            try {
                if (fs::is_regular_file(entry)) {
                    uintmax_t size = entry.file_size();
                    if (size > 10 * 1024 * 1024) { 
                        FileInfo info;
                        info.path = entry.path().string();
                        info.name = entry.path().filename().string();
                        info.sizeBytes = size;
                        info.sizeStr = formatSize(size);
                        fileIndex.push_back(info);
                        count++;
                    }
                }
            } catch (...) { continue; }
        }
    } catch (...) {}

    // 按大小降序
    std::sort(fileIndex.begin(), fileIndex.end(), [](const FileInfo& a, const FileInfo& b) {
        return a.sizeBytes > b.sizeBytes;
    });

    return count;
}

std::vector<FileInfo> FileMonitor::getLargeFiles(double sizeMB, int limit) {
    std::vector<FileInfo> result;
    uintmax_t thresholdBytes = static_cast<uintmax_t>(sizeMB * 1024 * 1024);
    
    int count = 0;
    for (const auto& file : fileIndex) {
        if (file.sizeBytes >= thresholdBytes) {
            result.push_back(file);
            count++;
            if (limit != -1 && count >= limit) break;
        }
    }
    return result;
}