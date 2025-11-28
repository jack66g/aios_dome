/**
 * @file cpu_control.h
 * @brief CPU 控制模块头文件
 * @details 负责调整 CPU 频率策略 (Governor)、设置频率限制以及绑定进程到指定核心
 * @note 需要 Root 权限运行
 */

#ifndef CPU_CONTROL_H
#define CPU_CONTROL_H

#include <string>
#include <vector>
#include <sys/types.h> // for pid_t

class CpuControl {
public:
    /**
     * @brief 构造函数
     */
    CpuControl();

    /**
     * @brief 析构函数
     */
    ~CpuControl();

    /**
     * @brief 设置所有 CPU 核心的调度模式 (Governor)
     * @details 常见的模式有:
     * - "performance": 始终保持最高频率 (高性能)
     * - "powersave": 始终保持最低频率 (省电)
     * - "schedutil": 根据负载动态调整 (Ubuntu 默认)
     * * @param governor 模式字符串 (e.g., "performance")
     * @return bool 设置成功返回 true，失败返回 false
     */
    bool setAllCoresGovernor(const std::string& governor);

    /**
     * @brief 将系统提升至“高性能模式”
     * @details 这是一个快捷函数，内部调用 setAllCoresGovernor("performance")
     * @return bool 成功 true
     */
    bool boostPerformance();

    /**
     * @brief 恢复系统为“节能/默认模式”
     * @details 这是一个快捷函数，内部调用 setAllCoresGovernor("schedutil" 或 "powersave")
     * @return bool 成功 true
     */
    bool restoreDefault();

    /**
     * @brief 将指定进程绑定到特定的 CPU 核心 (CPU Affinity)
     * @details 强制进程只在指定的 CPU 核心上运行，减少上下文切换，提高缓存命中率
     * * @param pid 目标进程 ID
     * @param coreId 目标核心 ID (例如 0, 1, 2...)
     * @return bool 绑定成功返回 true
     */
    bool bindProcessToCore(pid_t pid, int coreId);

    /**
     * @brief 解除进程的核心绑定
     * @details 允许进程在所有可用核心上调度
     * * @param pid 目标进程 ID
     * @return bool 成功 true
     */
    bool unbindProcess(pid_t pid);

private:
    /**
     * @brief 获取系统 CPU 核心数量
     * @return int 核心数
     */
    int getCpuCount();

    /**
     * @brief 内部辅助：写入字符串到文件
     * @param filePath 文件路径
     * @param value 要写入的内容
     * @return bool 写入成功 true
     */
    bool writeSysFile(const std::string& filePath, const std::string& value);
};

#endif // CPU_CONTROL_H