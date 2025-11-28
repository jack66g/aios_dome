/**
 * @file cpu_monitor.h
 * @brief CPU 监控模块头文件
 * @details 负责定义获取 CPU 负载、频率和温度的接口
 */

#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <string>
#include <vector>
#include <fstream>

class CpuMonitor {
public:
    /**
     * @brief 构造函数
     * @details 初始化 CPU 监控器，读取初始状态
     */
    CpuMonitor();

    /**
     * @brief 析构函数
     */
    ~CpuMonitor();

    /**
     * @brief 获取系统整体 CPU 占用率 (百分比)
     * @details 通过对比 /proc/stat 的两次快照计算瞬间使用率
     * @return double CPU 使用率 (0.0 - 100.0)
     */
    double getSystemCpuUsage();

    /**
     * @brief 获取当前 CPU 频率
     * @details 读取 /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
     * @return double 频率 (单位: MHz)
     */
    double getCpuFrequency();

    /**
     * @brief 获取 CPU 温度
     * @details 读取 /sys/class/thermal/thermal_zone0/temp
     * @return double 温度 (单位: 摄氏度)
     */
    double getCpuTemperature();

private:
    // 用于计算 CPU 使用率的结构体
    struct CpuStats {
        unsigned long long user;
        unsigned long long nice;
        unsigned long long system;
        unsigned long long idle;
        unsigned long long iowait;
        unsigned long long irq;
        unsigned long long softirq;
        unsigned long long steal;
    };

    CpuStats prevStats; // 上一次读取的 CPU 统计数据

    /**
     * @brief 从 /proc/stat 读取当前 CPU 原始数据
     * @return CpuStats 包含各状态的时间片数值
     */
    CpuStats readCpuStats();
};

#endif // CPU_MONITOR_H