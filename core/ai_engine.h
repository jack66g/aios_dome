/**
 * @file ai_engine.h
 * @brief AI 核心引擎 (模块化隔离版)
 */

#ifndef AI_ENGINE_H
#define AI_ENGINE_H

#include <string>
#include <memory>
#include <vector>
#include <thread> // 新增: 线程库
#include <atomic> // 新增: 原子变量控制线程退出

// 引入硬件模块
#include "modules/cpu/cpu_monitor.h"
#include "modules/cpu/cpu_control.h"
#include "modules/memory/mem_monitor.h"
#include "modules/memory/mem_control.h"
#include "process/proc_monitor.h"
#include "process/proc_control.h"
#include "file/file_monitor.h"
#include "file/file_control.h"
#include "file/file_creator.h"

class AiEngine {
public:
    AiEngine();
    ~AiEngine();

    void start();

private:
    // 硬件指针
    std::unique_ptr<CpuMonitor> cpuMonitor;
    std::unique_ptr<CpuControl> cpuControl;
    std::unique_ptr<MemMonitor> memMonitor;
    std::unique_ptr<MemControl> memControl;
    std::unique_ptr<ProcMonitor> procMonitor;
    std::unique_ptr<ProcControl> procControl;
    std::unique_ptr<FileMonitor> fileMonitor; // 新增：数据雷达
    std::unique_ptr<FileControl> fileControl;  // FileOps (新增这个!)
    std::unique_ptr<FileCreator> fileCreator; // 新增指针

    const std::string modelName = "qwen2.5-coder:1.5b"; 
    const std::string ollamaUrl = "http://localhost:11434/api/generate";

    // === 后台监控相关 ===
    std::atomic<bool> isMonitorRunning; // 标记当前是否正在运行
    std::atomic<bool> keepRunning; // 控制线程开关
    std::thread monitorThread;     // 监控线程对象
    void backgroundMonitorTask();  // 线程要执行的具体函数
    void startMonitor();          // 启动线程 (封装)
    void stopMonitor();           // 停止线程 (封装)
    
    // === 核心路由 ===
    // 负责判断用户是在说哪个领域的话
    void routeAndProcess(const std::string& input);
   

    // === 三大独立功能区 (提示词 + 执行 闭环封装) ===
    void runCpuModule(const std::string& input);
    void runMemModule(const std::string& input);
    void runProcModule(const std::string& input);
    void runMonitorModule(const std::string& input);
    void runFileModule(const std::string& input); // 新增处理函数
    void runFileControlModule(const std::string& input); // 新增功能区 (负责搜索/打开/删除)
    void runFileCreateModule(const std::string& input); // 新增处理函数

    // === 独立提示词生成器 ===
    std::string buildCpuPrompt(const std::string& input);
    std::string buildMemPrompt(const std::string& input);
    std::string buildProcPrompt(const std::string& input);
    std::string buildMonitorPrompt(const std::string& input);
    std::string buildFilePrompt(const std::string& input); // 新增 Prompt
    std::string buildFileControlPrompt(const std::string& input); // 新增 Prompt
    std::string buildFileCreatePrompt(const std::string& input); // 新增 Prompt

    // === 通用工具 ===
    std::string callOllama(const std::string& prompt);
    std::string extractJson(const std::string& json);
};

#endif // AI_ENGINE_H