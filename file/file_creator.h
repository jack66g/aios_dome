/**
 * @file file_creator.h
 * @brief 文件创建模块 (负责新建文件、写入内容)
 */

#ifndef FILE_CREATOR_H
#define FILE_CREATOR_H

#include <string>

class FileCreator {
public:
    FileCreator();
    ~FileCreator();

    /**
     * @brief 创建 .txt 文件并写入内容
     * @param filePath 文件完整路径 (必须以 .txt 结尾)
     * @param content 要写入的初始内容
     * @return 是否创建成功
     */
    bool createTxtFile(const std::string& filePath, const std::string& content);

private:
    // 简单的后缀检查
    bool isTxtExtension(const std::string& filePath);
};

#endif // FILE_CREATOR_H