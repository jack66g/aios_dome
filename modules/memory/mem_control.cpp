/**
 * @file mem_control.cpp
 * @brief 内存控制实现
 */

#include "modules/memory/mem_control.h"
#include <iostream>
#include <fstream>
#include <cstdlib> // for system()
#include <unistd.h> // for geteuid

MemControl::MemControl() {}
MemControl::~MemControl() {}

bool MemControl::dropCache() {
    // 检查权限
    if (geteuid() != 0) {
        std::cerr << "[Error] Memory cleaning requires ROOT privileges (sudo)." << std::endl;
        return false;
    }

    std::cout << "[System] Syncing filesystem before drop..." << std::endl;
    // 1. 同步数据到磁盘 (防止数据丢失)
    int syncRet = std::system("sync");
    if (syncRet != 0) {
        std::cerr << "[Warning] 'sync' command failed." << std::endl;
    }

    // 2. 写入 drop_caches
    // 1=PageCache, 2=dentries/inodes, 3=both
    // 我们写入 3 以最大程度释放
    std::ofstream file("/proc/sys/vm/drop_caches");
    if (file.is_open()) {
        file << "3";
        if (file.fail()) {
            std::cerr << "[Error] Failed to write to drop_caches." << std::endl;
            return false;
        }
        file.close();
        std::cout << "[Success] Memory cache dropped (RAM freed)." << std::endl;
        return true;
    } else {
        std::cerr << "[Error] Cannot open /proc/sys/vm/drop_caches." << std::endl;
        return false;
    }
}