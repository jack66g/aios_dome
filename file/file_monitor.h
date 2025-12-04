/**
 * @file file_monitor.h
 * @brief 文件监控模块 (纯净版：专注大文件扫描)
 */

#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H

#include <string>
#include <vector>
#include <filesystem>

struct FileInfo {
    std::string path;
    std::string name;
    uintmax_t sizeBytes;
    std::string sizeStr;
};

class FileMonitor {
public:
    FileMonitor();
    ~FileMonitor();

    std::string getCurrentRoot();

    /**
     * @brief 扫描并建立大文件索引
     * @return 扫描到的文件数量
     */
    int scanDirectory(const std::string& rootPath);

    /**
     * @brief 获取大于指定阈值的文件
     */
    std::vector<FileInfo> getLargeFiles(double sizeMB, int limit = 50);

private:
    std::string currentRootPath;
    std::vector<FileInfo> fileIndex; 
    std::string formatSize(uintmax_t bytes);
};

#endif // FILE_MONITOR_H