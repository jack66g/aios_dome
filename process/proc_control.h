/**
 * @file proc_control.h
 * @brief 进程控制模块
 */

#ifndef PROC_CONTROL_H
#define PROC_CONTROL_H

#include <string>

class ProcControl {
public:
    ProcControl();
    ~ProcControl();

    /**
     * @brief 杀掉进程 (kill)
     * @param pid 进程ID
     * @return 是否成功
     */
    bool killProcess(int pid);

    /**
     * @brief 锁定/暂停进程 (SIGSTOP)
     * @details 进程会被冻结，不再占用 CPU，直到被解锁
     * @param pid 进程ID
     */
    bool lockProcess(int pid);

    /**
     * @brief 解锁/恢复进程 (SIGCONT)
     * @param pid 进程ID
     */
    bool unlockProcess(int pid);
};

#endif // PROC_CONTROL_H