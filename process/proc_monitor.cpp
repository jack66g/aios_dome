/**
 * @file proc_monitor.cpp
 * @brief 进程监控实现
 */

#include "process/proc_monitor.h"
#include <iostream>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <set>
#include <dirent.h> // 遍历 /proc 目录用
#include <memory>

ProcMonitor::ProcMonitor() {
    // 初始化时先保存一份当前 PID 列表，作为基准
    lastPidList = getAllPids();
}

ProcMonitor::~ProcMonitor() {}

// 使用 ps 命令获取 Top 进程，因为计算准确的瞬间 CPU% 比较复杂，ps 是系统级标准
std::vector<ProcessInfo> ProcMonitor::getTopCpuProcesses(int limit) {
    std::vector<ProcessInfo> processes;
    
    // ps 命令：-e (所有进程) -o (输出格式) --sort (排序)
    // 格式：PID, 进程名, CPU%, MEM%
    std::string cmd = "ps -eo pid,comm,%cpu,%mem --sort=-%cpu | head -n " + std::to_string(limit + 1); // +1 是因为有标题行
    
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return processes;

    char buffer[256];
    // 跳过第一行标题
    fgets(buffer, 256, pipe.get());

    while (fgets(buffer, 256, pipe.get()) != nullptr) {
        ProcessInfo p;
        std::stringstream ss(buffer);
        ss >> p.pid >> p.name >> p.cpuPercent >> p.memPercent;
        processes.push_back(p);
    }
    return processes;
}

int ProcMonitor::findPidByName(const std::string& targetName) {
    // 简单的模糊匹配
    std::string cmd = "pgrep -f " + targetName + " | head -n 1";
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return -1;

    char buffer[128];
    if (fgets(buffer, 128, pipe.get()) != nullptr) {
        return std::stoi(buffer);
    }
    return -1;
}

// 获取当前系统中所有数字命名的目录 (即 PID)
std::vector<int> ProcMonitor::getAllPids() {
    std::vector<int> pids;
    DIR* dir = opendir("/proc");
    if (!dir) return pids;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 如果目录名是纯数字，就是 PID
        if (isdigit(entry->d_name[0])) {
            try {
                pids.push_back(std::stoi(entry->d_name));
            } catch (...) {}
        }
    }
    closedir(dir);
    std::sort(pids.begin(), pids.end());
    return pids;
}

// 对比新旧 PID 列表，找出新增的
std::string ProcMonitor::detectNewProcesses() {
    std::vector<int> currentPids = getAllPids();
    std::string report = "";

    // 使用 set 加速查找
    std::set<int> oldSet(lastPidList.begin(), lastPidList.end());

    for (int pid : currentPids) {
        if (oldSet.find(pid) == oldSet.end()) {
            // 发现新进程，尝试获取名字
            std::string name = "Unknown";
            std::string path = "/proc/" + std::to_string(pid) + "/comm";
            FILE* f = fopen(path.c_str(), "r");
            if (f) {
                char buffer[128];
                if (fgets(buffer, 128, f)) {
                    name = buffer;
                    // 去掉换行符
                    if (!name.empty() && name.back() == '\n') name.pop_back();
                }
                fclose(f);
            }
            report += "[新进程] PID: " + std::to_string(pid) + " (" + name + ")\n";
        }
    }

    // 更新基准
    lastPidList = currentPids;
    return report;
}