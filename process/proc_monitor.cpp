/**
 * @file proc_monitor.cpp
 * @brief 进程监控模块实现
 */

#include "process/proc_monitor.h"
#include <iostream>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <dirent.h> // 用于遍历 /proc 目录
#include <memory>
#include <fstream>
#include <cstring>

ProcMonitor::ProcMonitor() {
    // 初始化时，先获取一次当前所有 PID，作为基准
    std::vector<int> pids = getAllPids();
    // 存入 set 中
    lastPidSet.insert(pids.begin(), pids.end());
}

ProcMonitor::~ProcMonitor() {}

// 辅助：读取 /proc/[pid]/comm 获取进程名
std::string ProcMonitor::getProcessName(int pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream file(path);
    std::string name;
    if (file.is_open()) {
        std::getline(file, name);
        // 去除文件末尾可能存在的换行符
        if (!name.empty() && name.back() == '\n') name.pop_back();
        file.close();
    }
    return name.empty() ? "unknown" : name;
}

// 辅助：遍历 /proc 目录获取所有数字文件夹 (即 PID)
std::vector<int> ProcMonitor::getAllPids() {
    std::vector<int> pids;
    DIR* dir = opendir("/proc");
    if (!dir) return pids;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 如果目录名首字符是数字，说明是 PID 目录
        if (isdigit(entry->d_name[0])) {
            try {
                pids.push_back(std::stoi(entry->d_name));
            } catch (...) {}
        }
    }
    closedir(dir);
    return pids;
}

// 获取 Top CPU 进程 (使用 ps 命令)
std::vector<ProcessInfo> ProcMonitor::getTopCpuProcesses(int limit) {
    std::vector<ProcessInfo> processes;
    
    // ps 命令：-e (所有进程) -o (自定义输出) --sort (按CPU倒序)
    std::string cmd = "ps -eo pid,comm,%cpu,%mem --sort=-%cpu | head -n " + std::to_string(limit + 1);
    
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return processes;

    char buffer[256];
    // 读取并跳过第一行标题 (PID COMMAND %CPU %MEM)
    fgets(buffer, 256, pipe.get());

    while (fgets(buffer, 256, pipe.get()) != nullptr) {
        ProcessInfo p;
        std::stringstream ss(buffer);
        ss >> p.pid >> p.name >> p.cpuPercent >> p.memPercent;
        processes.push_back(p);
    }
    return processes;
}

// 根据名字查找 PID
int ProcMonitor::findPidByName(const std::string& targetName) {
    // 1. 优先尝试精确匹配 (-x)
    std::string cmd = "pgrep -x " + targetName + " | head -n 1";
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    char buffer[64];
    if (fgets(buffer, 64, pipe.get())) {
        return std::stoi(buffer);
    }
    
    // 2. 如果失败，尝试模糊匹配 (-f)
    cmd = "pgrep -f " + targetName + " | head -n 1";
    std::shared_ptr<FILE> pipe2(popen(cmd.c_str(), "r"), pclose);
    if (fgets(buffer, 64, pipe2.get())) {
        return std::stoi(buffer);
    }

    return -1;
}

// === 核心逻辑 1: 持续检测新进程 ===

std::string ProcMonitor::detectNewProcesses() {

    // 1. 获取当前的 PID 列表

    std::vector<int> currentPids = getAllPids();

    std::set<int> currentPidSet(currentPids.begin(), currentPids.end());

    

    std::string report = "";



    // 2. 遍历现在的 PID，如果之前的 set 里没有，说明是新启动的

    for (int pid : currentPidSet) {

        // 如果在旧集合里找不到这个 pid

        if (lastPidSet.find(pid) == lastPidSet.end()) {

            std::string name = getProcessName(pid);

            

            // 过滤掉极其短暂的系统命令进程 (如 ps, grep, sh)，避免刷屏

            if (name != "ps" && name != "grep" && name != "sh" && name != "pgrep") {

                report += " [新进程] " + name + " (PID:" + std::to_string(pid) + ")\n";

            }

        }

    }



    // 3. 更新基准，把现在的变成“旧的”，供下一次对比

    lastPidSet = currentPidSet;

    return report;

}



// === 核心逻辑 2: 检测异常高占用 ===

std::string ProcMonitor::detectAbnormalProcesses(double threshold) {

    // 获取前 3 名高占用

    auto procs = getTopCpuProcesses(3);

    std::string report = "";

    

    for (const auto& p : procs) {

        if (p.cpuPercent > threshold) {

            report += " [异常高负载] " + p.name + " (CPU:" + std::to_string(p.cpuPercent) + "%)\n";

        }

    }

    return report;

}