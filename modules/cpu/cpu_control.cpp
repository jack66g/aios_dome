/**
 * @file cpu_control.cpp
 * @brief CPU 控制模块实现文件
 * @details 实现对 /sys/devices/system/cpu 下文件的写入以及 sched_setaffinity 调用
 */

#include "modules/cpu/cpu_control.h"
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <sched.h>  // 包含 CPU 亲和性相关的系统调用
#include <unistd.h> // 包含 sysconf

/**
 * @brief 构造函数
 */
CpuControl::CpuControl() {
    // 可以在这里检查是否具有 root 权限，如果没有则打印警告
    if (geteuid() != 0) {
        std::cerr << "[Warning] CpuControl module requires ROOT privileges to modify system settings!" << std::endl;
    }
}

/**
 * @brief 析构函数
 */
CpuControl::~CpuControl() {}

/**
 * @brief 获取 CPU 核心数
 * @details 使用 std::thread::hardware_concurrency() 或者 sysconf
 * @return int 逻辑核心数量
 */
int CpuControl::getCpuCount() {
    return std::thread::hardware_concurrency();
}

/**
 * @brief 内部辅助函数：写入系统文件
 * @details 这是一个通用的文件写入函数，用于修改 /sys 下的配置
 * * @param filePath 完整文件路径
 * @param value 要写入的字符串
 * @return bool 写入成功返回 true，权限不足或路径不存在返回 false
 */
bool CpuControl::writeSysFile(const std::string& filePath, const std::string& value) {
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << value;
        if (file.fail()) {
            std::cerr << "[Error] Failed to write to " << filePath << std::endl;
            return false;
        }
        file.close();
        return true;
    } else {
        std::cerr << "[Error] Cannot open file (Permission denied?): " << filePath << std::endl;
        return false;
    }
}

/**
 * @brief 设置所有 CPU 核心的 Governor (调度模式)
 * @details 遍历 cpu0 到 cpuN，修改 scaling_governor 文件
 * * @param governor 模式名称 (如 "performance", "powersave")
 * @return bool 如果所有核心都设置成功，返回 true
 */
bool CpuControl::setAllCoresGovernor(const std::string& governor) {
    int coreCount = getCpuCount();
    bool allSuccess = true;

    for (int i = 0; i < coreCount; ++i) {
        // 拼凑路径: /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_governor";
        
        if (!writeSysFile(path, governor)) {
            allSuccess = false;
            // 继续尝试设置下一个核心，不立即中断
        }
    }

    if (allSuccess) {
        std::cout << "[Info] All cores set to governor: " << governor << std::endl;
    } else {
        std::cerr << "[Warning] Some cores failed to set governor. Check permissions." << std::endl;
    }

    return allSuccess;
}

/**
 * @brief 一键提升性能
 * @details 强制切换到 performance 模式，让 CPU 尽可能跑在最高频
 */
bool CpuControl::boostPerformance() {
    return setAllCoresGovernor("performance");
}

/**
 * @brief 恢复默认/节能模式
 * @details 切换回 schedutil (现代 Linux 通用默认) 或 powersave
 */
bool CpuControl::restoreDefault() {
    // Ubuntu 22.04 默认通常是 schedutil，如果不支持则尝试 powersave
    // 这里为了演示简单，直接设置 schedutil
    return setAllCoresGovernor("schedutil");
}

/**
 * @brief 将进程绑定到指定核心
 * @details 使用 sched_setaffinity 系统调用
 * * @param pid 进程 ID
 * @param coreId 核心索引 (0 ~ N-1)
 * @return bool 成功返回 true
 */
bool CpuControl::bindProcessToCore(pid_t pid, int coreId) {
    int maxCores = getCpuCount();
    if (coreId < 0 || coreId >= maxCores) {
        std::cerr << "[Error] Invalid Core ID: " << coreId << std::endl;
        return false;
    }

    // cpu_set_t 是一个位掩码结构
    cpu_set_t mask;
    CPU_ZERO(&mask);       // 清空集合
    CPU_SET(coreId, &mask); // 将指定核心加入集合

    // 设置亲和性
    // 第一个参数是 pid (0 代表当前进程)，第二个参数是集合大小，第三个是集合指针
    if (sched_setaffinity(pid, sizeof(mask), &mask) == -1) {
        perror("[Error] sched_setaffinity failed");
        return false;
    }

    std::cout << "[Info] Process " << pid << " bound to Core " << coreId << std::endl;
    return true;
}

/**
 * @brief 解除进程核心绑定
 * @details 将亲和性掩码设置为所有核心，允许操作系统自由调度
 */
bool CpuControl::unbindProcess(pid_t pid) {
    int maxCores = getCpuCount();
    cpu_set_t mask;
    CPU_ZERO(&mask);

    // 将所有核心都加入集合
    for (int i = 0; i < maxCores; i++) {
        CPU_SET(i, &mask);
    }

    if (sched_setaffinity(pid, sizeof(mask), &mask) == -1) {
        perror("[Error] Failed to unbind process");
        return false;
    }

    std::cout << "[Info] Process " << pid << " unbound (can run on any core)" << std::endl;
    return true;
}