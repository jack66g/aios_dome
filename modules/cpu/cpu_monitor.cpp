/**
 * @file cpu_monitor.cpp
 * @brief CPU 监控模块实现文件
 * @details 实现读取 Linux 系统文件 (/proc, /sys) 来获取 CPU 信息
 */

#include "modules/cpu/cpu_monitor.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <numeric>

/**
 * @brief 构造函数实现
 * @details 初始化时立即读取一次 CPU 状态，作为计算占用率的基准
 */
CpuMonitor::CpuMonitor() {
    this->prevStats = readCpuStats();
}

/**
 * @brief 析构函数实现
 */
CpuMonitor::~CpuMonitor() {}

/**
 * @brief 内部辅助函数：读取 /proc/stat
 * @details 解析 /proc/stat 文件第一行（即 "cpu" 开头的总行），获取各项时间片
 * @return CpuStats 结构体
 */
CpuMonitor::CpuStats CpuMonitor::readCpuStats() {
    CpuStats stats = {0};
    std::ifstream file("/proc/stat");
    
    if (file.is_open()) {
        std::string line;
        std::getline(file, line); // 读取第一行
        std::istringstream iss(line);
        std::string cpuLabel;

        // 格式: cpu  user nice system idle iowait irq softirq steal guest guest_nice
        iss >> cpuLabel 
            >> stats.user 
            >> stats.nice 
            >> stats.system 
            >> stats.idle 
            >> stats.iowait 
            >> stats.irq 
            >> stats.softirq 
            >> stats.steal;
        
        file.close();
    } else {
        std::cerr << "[Error] Cannot open /proc/stat" << std::endl;
    }
    
    return stats;
}

/**
 * @brief 获取系统整体 CPU 占用率
 * @details 
 * 计算公式: 
 * Total = User + Nice + System + Idle + IOWait + IRQ + SoftIRQ + Steal
 * Idle = Idle + IOWait
 * Usage = (Total_diff - Idle_diff) / Total_diff
 * * @return double CPU 使用率 (0.0 - 100.0)
 */
double CpuMonitor::getSystemCpuUsage() {
    // 读取当前的统计数据
    CpuStats currentStats = readCpuStats();

    // 计算上一时刻的总时间
    unsigned long long prevIdle = prevStats.idle + prevStats.iowait;
    unsigned long long prevTotal = prevStats.user + prevStats.nice + prevStats.system + 
                                   prevIdle + prevStats.irq + prevStats.softirq + prevStats.steal;

    // 计算当前时刻的总时间
    unsigned long long currentIdle = currentStats.idle + currentStats.iowait;
    unsigned long long currentTotal = currentStats.user + currentStats.nice + currentStats.system + 
                                      currentIdle + currentStats.irq + currentStats.softirq + currentStats.steal;

    // 计算差值
    unsigned long long totalDiff = currentTotal - prevTotal;
    unsigned long long idleDiff = currentIdle - prevIdle;

    double usage = 0.0;
    
    // 防止除以0
    if (totalDiff > 0) {
        usage = (double)(totalDiff - idleDiff) / totalDiff * 100.0;
    }

    // 更新“上一次”的状态，以便下一次调用时对比
    prevStats = currentStats;

    return usage;
}

/**
 * @brief 获取 CPU 0 的当前频率
 * @details 读取 /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
 * 注意：有些极简系统可能没有 cpufreq 驱动，通常 Ubuntu 桌面/服务器版都有。
 * * @return double 频率 (单位: MHz)
 */
double CpuMonitor::getCpuFrequency() {
    double frequency = 0.0;
    // 读取 CPU0 的频率作为参考。如果需要所有核心平均值，需要遍历 cpu0, cpu1...
    std::ifstream file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    
    if (file.is_open()) {
        double freqKHz;
        file >> freqKHz;
        frequency = freqKHz / 1000.0; // 转换为 MHz
        file.close();
    } else {
        // 备选方案：尝试从 /proc/cpuinfo 获取 (解析较为复杂，这里作为简单的错误处理)
        // std::cerr << "[Warning] Cannot read cpu frequency from sysfs." << std::endl;
    }

    return frequency;
}

/**
 * @brief 获取 CPU 温度
 * @details 读取 /sys/class/thermal/thermal_zone0/temp
 * 系统通常将 thermal_zone0 作为 ACPI CPU 热区。
 * 文件中的数值通常是千分之一摄氏度。
 * * @return double 温度 (单位: 摄氏度)
 */
double CpuMonitor::getCpuTemperature() {
    double temperature = 0.0;
    // thermal_zone0 通常是 CPU 或 x86_pkg_temp
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    
    if (file.is_open()) {
        double tempRaw;
        file >> tempRaw;
        temperature = tempRaw / 1000.0; // 转换为摄氏度
        file.close();
    } else {
        // 如果读取失败（例如虚拟机环境可能没有温度传感器）
        // std::cerr << "[Warning] Cannot read thermal zone." << std::endl;
        temperature = -1.0; // 返回 -1 表示无法获取
    }

    return temperature;
}