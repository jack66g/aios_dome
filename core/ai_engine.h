/**
 * @file ai_engine.h
 * @brief AI 核心引擎头文件
 */

#ifndef AI_ENGINE_H
#define AI_ENGINE_H

#include <string>
#include <memory>
#include <vector>

// 引入各硬件模块头文件
#include "modules/cpu/cpu_monitor.h"
#include "modules/cpu/cpu_control.h"
#include "modules/memory/mem_monitor.h"
#include "modules/memory/mem_control.h"
#include "process/proc_monitor.h"
#include "process/proc_control.h"

// 定义指令结构
struct AICommand {
    std::string action; 
};

class AiEngine {
public:
    AiEngine();
    ~AiEngine();

    // 启动主循环
    void start();

private:
    // === 硬件模块指针 ===
    std::unique_ptr<CpuMonitor> cpuMonitor;
    std::unique_ptr<CpuControl> cpuControl;
    std::unique_ptr<MemMonitor> memMonitor;
    std::unique_ptr<MemControl> memControl;
    std::unique_ptr<ProcMonitor> procMonitor;
    std::unique_ptr<ProcControl> procControl;

    // 配置
    const std::string modelName = "qwen2.5-coder:1.5b"; 
    const std::string ollamaUrl = "http://localhost:11434/api/generate";

    // === 核心流程 ===
    void processInput(const std::string& input);
    AICommand think(const std::string& userInput);
    std::string callOllama(const std::string& prompt);
    void executeCommand(const AICommand& cmd);

    // === 辅助功能 ===
    std::string getSystemSummary();      // 获取硬件摘要
    std::string buildPrompt(const std::string& context, const std::string& input); // 生成提示词

    // === 业务逻辑封装 (解耦) ===
    void handleCpuCommand(const std::string& action);
    void handleMemCommand(const std::string& action);
    void handleProcCommand(const std::string& action);
};

#endif // AI_ENGINE_H