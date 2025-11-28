/**
 * @file proc_monitor.h
 * @brief 进程监控模块
 */

#ifndef PROC_MONITOR_H
#define PROC_MONITOR_H

#include <string>
#include <vector>
#include <map>

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
     * @brief 检测是否有新启动的进程 (模拟“用户打开一个进程就报告”)
     * @details 通过对比上一次的快照
     * @return 新增进程的描述字符串
     */
    std::string detectNewProcesses();

private:
    std::vector<int> lastPidList; // 上一次扫描到的所有 PID
    
    // 获取当前所有 PID 的辅助函数
    std::vector<int> getAllPids();
};

#endif // PROC_MONITOR_H