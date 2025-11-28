/**
 * @file proc_control.cpp
 * @brief 进程控制实现
 */

#include "process/proc_control.h"
#include <iostream>
#include <signal.h> // Linux 信号处理库
#include <unistd.h> // for geteuid

ProcControl::ProcControl() {}
ProcControl::~ProcControl() {}

bool ProcControl::killProcess(int pid) {
    if (pid <= 1) {
        std::cerr << "[Warning] 不能杀掉系统核心进程 (PID <= 1)！" << std::endl;
        return false;
    }
    
    std::cout << "[System] 正在终止进程 PID: " << pid << " ..." << std::endl;
    // 发送 SIGKILL (9) 强制杀死
    if (kill(pid, SIGKILL) == 0) {
        return true;
    } else {
        perror("[Error] Kill failed");
        return false;
    }
}

bool ProcControl::lockProcess(int pid) {
    std::cout << "[System] 正在冻结(Lock) 进程 PID: " << pid << " ..." << std::endl;
    // 发送 SIGSTOP (19) 暂停进程
    if (kill(pid, SIGSTOP) == 0) {
        return true;
    } else {
        perror("[Error] Lock failed");
        return false;
    }
}

bool ProcControl::unlockProcess(int pid) {
    std::cout << "[System] 正在解冻(Unlock) 进程 PID: " << pid << " ..." << std::endl;
    // 发送 SIGCONT (18) 继续运行
    if (kill(pid, SIGCONT) == 0) {
        return true;
    } else {
        perror("[Error] Unlock failed");
        return false;
    }
}