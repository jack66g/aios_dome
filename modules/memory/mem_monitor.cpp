/**
 * @file mem_monitor.cpp
 * @brief 内存监控实现
 */

#include "modules/memory/mem_monitor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

MemMonitor::MemMonitor() {}
MemMonitor::~MemMonitor() {}

MemoryStatus MemMonitor::getMemoryStatus() {
    MemoryStatus status = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::ifstream file("/proc/meminfo");
    
    if (!file.is_open()) {
        std::cerr << "[Error] Cannot open /proc/meminfo" << std::endl;
        return status;
    }

    std::string line, key;
    double value;
    std::string unit;
    
    // 使用 map 暂存读取到的关键值 (单位 kB)
    std::map<std::string, double> memInfo;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        // 读取格式例如: "MemTotal:        16303284 kB"
        iss >> key >> value >> unit;
        
        // 去掉冒号
        if (!key.empty() && key.back() == ':') {
            key.pop_back();
        }
        memInfo[key] = value;
    }
    file.close();

    // 提取我们需要的数据 (注意 map 如果找不到key会返回0，比较安全)
    double memTotal = memInfo["MemTotal"];
    double memAvailable = memInfo["MemAvailable"]; // 新版Linux通常用这个
    double memFree = memInfo["MemFree"];
    double buffers = memInfo["Buffers"];
    double cached = memInfo["Cached"];
    double swapTotal = memInfo["SwapTotal"];
    double swapFree = memInfo["SwapFree"];

    // 如果系统老旧没有 MemAvailable，手动计算: Free + Buffers + Cached
    if (memAvailable == 0) {
        memAvailable = memFree + buffers + cached;
    }

    // 计算结果 (转换为 MB)
    status.totalMB = memTotal / 1024.0;
    status.availableMB = memAvailable / 1024.0;
    status.usedMB = (memTotal - memAvailable) / 1024.0;
    
    if (status.totalMB > 0) {
        status.usagePercent = (status.usedMB / status.totalMB) * 100.0;
    }

    status.swapTotalMB = swapTotal / 1024.0;
    status.swapUsedMB = (swapTotal - swapFree) / 1024.0;

    return status;
}