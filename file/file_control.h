/**
 * @file file_control.h
 * @brief 文件控制模块 (负责搜索、打开、删除)
 */

#ifndef FILE_CONTROL_H
#define FILE_CONTROL_H

#include <string>
#include <vector>

class FileControl {
public:
    FileControl();
    ~FileControl();

    /**
     * @brief 精确/模糊搜索特定文件
     * @param keyword 文件名
     * @return 匹配的文件路径列表
     */
    std::vector<std::string> searchFile(const std::string& keyword);

    /**
     * @brief 调用系统命令打开文件
     */
    bool openFile(const std::string& filePath);

    /**
     * @brief 删除文件
     */
    bool deleteFile(const std::string& filePath);

private:
    std::string currentRootPath;
};

#endif // FILE_CONTROL_H