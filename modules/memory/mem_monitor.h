/**
 * @file mem_monitor.h
 * @brief 内存监控模块头文件
 */

#ifndef MEM_MONITOR_H
#define MEM_MONITOR_H

#include <string>

// 定义一个结构体，用来一次性返回所有内存状态
struct MemoryStatus {
    double totalMB;     // 总物理内存 (MB)
    double availableMB; // 可用内存 (MB) - 注意：Linux下 Available 比 Free 更准确反映可用量
    double usedMB;      // 已用内存 (MB)
    double usagePercent;// 内存占用百分比 (%)
    
    double swapTotalMB; // 交换空间总大小
    double swapUsedMB;  // 交换空间已用
};

class MemMonitor {
public:
    MemMonitor();
    ~MemMonitor();

    /**
     * @brief 获取当前系统内存详细状态
     * @return MemoryStatus 结构体
     */
    MemoryStatus getMemoryStatus();
};

#endif // MEM_MONITOR_H