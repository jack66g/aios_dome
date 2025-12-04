/**
 * @file proc_monitor.h
 * @brief 进程监控模块 (升级版：支持新进程检测与异常检测)
 */

#ifndef PROC_MONITOR_H
#define PROC_MONITOR_H

#include <string>
#include <vector>
#include <set> // 引入 set 用于快速查找 PID

// 定义进程信息的结构体
struct ProcessInfo {
    int pid;
    std::string name;
    double cpuPercent;
    double memPercent;
};

class ProcMonitor {
public:
    ProcMonitor();
    ~ProcMonitor();

    /**
     * @brief 获取当前 CPU 占用最高的 N 个进程
     * @param limit 返回的数量 (默认前5)
     * @return 进程列表
     */
    std::vector<ProcessInfo> getTopCpuProcesses(int limit = 5);

    /**
     * @brief 根据进程名查找 PID
     * @param processName 进程名 (如 "chrome")
     * @return PID，如果没找到返回 -1
     */
    int findPidByName(const std::string& processName);

   /**
     * @brief 检测是否有新启动的进程
     * @details 对比上一刻的 PID 列表，找出新增的 PID
     * @return 包含新进程信息的字符串报告
     */
    std::string detectNewProcesses();

    /**
     * @brief 检测异常高占用进程
     * @param threshold CPU占用阈值 (例如 80.0)
     * @return 异常进程报告
     */
    std::string detectAbnormalProcesses(double threshold = 80.0);

private:
    std::set<int> lastPidSet; // 使用 set 存储上一次的 PID，查询速度比 vector 快
    
    // 获取当前所有 PID 的辅助函数
    std::vector<int> getAllPids();
    
    // 获取进程名称的辅助函数 (读取 /proc/[pid]/comm)
    std::string getProcessName(int pid);
};

#endif // PROC_MONITOR_H