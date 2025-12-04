/**
 * @file file_creator.cpp
 * @brief 文件创建实现
 */

#include "file/file_creator.h"
#include <iostream>
#include <fstream>
#include <algorithm>

FileCreator::FileCreator() {}
FileCreator::~FileCreator() {}

bool FileCreator::isTxtExtension(const std::string& filePath) {
    if (filePath.length() < 4) return false;
    std::string ext = filePath.substr(filePath.length() - 4);
    // 转小写比较
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".txt");
}

bool FileCreator::createTxtFile(const std::string& filePath, const std::string& content) {
    // 1. 检查后缀限制
    if (!isTxtExtension(filePath)) {
        std::cerr << "[FileCreator] 错误：目前仅支持创建 .txt 文件。" << std::endl;
        return false;
    }

    // 2. 尝试创建并写入
    std::ofstream outfile(filePath);
    if (outfile.is_open()) {
        outfile << content;
        outfile.close();
        return true;
    } else {
        std::cerr << "[FileCreator] 错误：无法创建文件 (路径不存在或权限不足)。" << std::endl;
        return false;
    }
}