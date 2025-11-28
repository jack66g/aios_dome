/**
 * @file ai_engine.cpp
 * @brief AI 核心引擎实现 (最终重构版)
 */

#include "core/ai_engine.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <iomanip>
#include <memory>

// ==========================================
//           工具函数区
// ==========================================

// JSON 转义：处理用户输入中的特殊字符
std::string jsonEscape(const std::string& input) {
    std::string output;
    for (char c : input) {
        if (c == '"') output += "\\\"";
        else if (c == '\\') output += "\\\\";
        else if (c == '\n') output += " ";
        else output += c;
    }
    return output;
}

// JSON 提取：从 Ollama 的回复中提取 response 字段 (防止崩溃的核心)
std::string extractJsonValue(const std::string& jsonResponse) {
    std::string key = "\"response\":\"";
    size_t startPos = jsonResponse.find(key);
    if (startPos == std::string::npos) return ""; 

    startPos += key.length();
    std::string result;
    bool escape = false;
    for (size_t i = startPos; i < jsonResponse.length(); ++i) {
        char c = jsonResponse[i];
        if (escape) { result += c; escape = false; }
        else {
            if (c == '\\') escape = true;
            else if (c == '"') break; // 遇到结束引号停止
            else result += c;
        }
    }
    return result;
}

// ==========================================
//           引擎核心逻辑
// ==========================================

AiEngine::AiEngine() {
    std::cout << "[系统] 初始化全系统模块..." << std::endl;
    cpuMonitor = std::make_unique<CpuMonitor>();
    cpuControl = std::make_unique<CpuControl>();
    memMonitor = std::make_unique<MemMonitor>();
    memControl = std::make_unique<MemControl>();
    procMonitor = std::make_unique<ProcMonitor>();
    procControl = std::make_unique<ProcControl>();
    cpuMonitor->getSystemCpuUsage(); // 预热
    std::cout << "[系统] 引擎就绪。" << std::endl;
}

AiEngine::~AiEngine() {}

// 获取系统状态摘要
std::string AiEngine::getSystemSummary() {
    double cpuUsage = cpuMonitor->getSystemCpuUsage();
    MemoryStatus memStat = memMonitor->getMemoryStatus();
    std::stringstream ss;
    ss << "CPU:" << std::fixed << std::setprecision(1) << cpuUsage << "%, "
       << "Mem:" << std::fixed << std::setprecision(1) << memStat.usagePercent << "%";
    return ss.str();
}

// === 提示词生成器 (Prompt 独立封装) ===
// === 修改 core/ai_engine.cpp 中的 buildPrompt 函数 ===
std::string AiEngine::buildPrompt(const std::string& context, const std::string& userInput) {
    // 我们把 Prompt 设计得更像一个“多选题”，并给出明确的映射表
    return 
        "你是一个严格的指令分类器。用户输入: \"" + userInput + "\"。\n"
        "请根据以下规则，返回且仅返回一个标签：\n"
        "1. 如果用户输入 'CPU', 'cpu', 'CPU状态', '主频', '温度' -> 返回 [CHECK_CPU]\n"
        "2. 如果用户输入 '内存', 'memory', 'RAM', '存储' -> 返回 [CHECK_MEM]\n"
        "3. 如果用户输入 '清理', '加速', 'clean' -> 返回 [CLEAN_MEM]\n"
        "4. 如果用户输入 '进程', '任务', '谁在卡', 'top' -> 返回 [LIST_PROC]\n"
        "5. 如果用户输入 '杀掉', '结束', '关闭' + 软件名 -> 返回 [KILL_PROC:软件名]\n"
        "   (必须提取软件名，如: 杀掉火狐 -> [KILL_PROC:firefox], 杀掉谷歌 -> [KILL_PROC:chrome])\n"
        "6. 如果用户输入 '高性能', '游戏模式' -> 返回 [BOOST_MODE]\n"
        "7. 如果用户输入 '省电', '默认模式' -> 返回 [RESTORE_MODE]\n"
        "8. 如果用户输入其他无关内容、标点符号或无法理解 -> 必须返回 [UNKNOWN]\n"
        "注意：不要解释，不要废话，只返回括号内的标签。";
}

// 调用 Ollama
std::string AiEngine::callOllama(const std::string& promptText) {
    std::string safePrompt = jsonEscape(promptText);
    std::string jsonPayload = "{\"model\": \"" + modelName + "\", \"prompt\": \"" + safePrompt + "\", \"stream\": false}";
    std::string cmd = "curl -s -X POST " + ollamaUrl + " -d '" + jsonPayload + "'";

    std::string rawJson;
    char buffer[1024];
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer, 1024, pipe.get())) rawJson += buffer;

    // 使用提取器处理 JSON，防止 Shell 语法错误
    return extractJsonValue(rawJson);
}

// 思考逻辑
AICommand AiEngine::think(const std::string& userInput) {
    AICommand cmd;
    cmd.action = "UNKNOWN";

    std::string context = getSystemSummary();
    std::string systemPrompt = buildPrompt(context, userInput); // 调用封装好的提示词

    std::cout << "[AI] 分析中..." << std::endl;
    std::string cleanResponse = callOllama(systemPrompt);

    if (cleanResponse.empty()) {
        std::cerr << "[Error] Ollama 未响应或解析失败。" << std::endl;
        return cmd;
    }

    // 清洗换行符
    cleanResponse.erase(std::remove(cleanResponse.begin(), cleanResponse.end(), '\n'), cleanResponse.end());
    
    // 转大写用于匹配
    std::string upper = cleanResponse;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper.find("CHECK_CPU") != std::string::npos) cmd.action = "CHECK_CPU";
    else if (upper.find("CHECK_MEM") != std::string::npos) cmd.action = "CHECK_MEM";
    else if (upper.find("CLEAN_MEM") != std::string::npos) cmd.action = "CLEAN_MEM";
    else if (upper.find("BOOST_MODE") != std::string::npos) cmd.action = "BOOST_MODE";
    else if (upper.find("RESTORE_MODE") != std::string::npos) cmd.action = "RESTORE_MODE";
    else if (upper.find("LIST_PROC") != std::string::npos) cmd.action = "LIST_PROC";
    else if (upper.find("KILL_PROC") != std::string::npos) cmd.action = cleanResponse; // 保留原始回复提取名字
    
    return cmd;
}

// ==========================================
//           业务逻辑封装区 (完全独立)
// ==========================================

void AiEngine::handleCpuCommand(const std::string& action) {
    if (action == "CHECK_CPU") {
        double temp = cpuMonitor->getCpuTemperature();
        std::cout << "\n>>> [ CPU 报告 ] <<<\n"
                  << "使用率 : " << cpuMonitor->getSystemCpuUsage() << "%\n"
                  << "主频   : " << cpuMonitor->getCpuFrequency() << " MHz\n"
                  << "温度   : " << (temp > 0 ? std::to_string(temp) + " °C" : "N/A") << "\n"
                  << "--------------------\n" << std::endl;
    } 
    else if (action == "BOOST_MODE") {
        std::cout << "\n[AI] 启动高性能模式..." << std::endl;
        cpuControl->boostPerformance();
        std::cout << "\n";
    } 
    else if (action == "RESTORE_MODE") {
        std::cout << "\n[AI] 恢复默认模式..." << std::endl;
        cpuControl->restoreDefault();
        std::cout << "\n";
    }
}

void AiEngine::handleMemCommand(const std::string& action) {
    if (action == "CHECK_MEM") {
        MemoryStatus ms = memMonitor->getMemoryStatus();
        std::cout << "\n>>> [ 内存报告 ] <<<\n"
                  << "总量 : " << ms.totalMB << " MB\n"
                  << "已用 : " << ms.usedMB << " MB (" << ms.usagePercent << "%)\n"
                  << "可用 : " << ms.availableMB << " MB\n"
                  << "Swap : " << ms.swapUsedMB << " MB\n"
                  << "--------------------\n" << std::endl;
    } 
    else if (action == "CLEAN_MEM") {
        std::cout << "\n[AI] 执行内存清理..." << std::endl;
        if (memControl->dropCache()) {
            std::cout << ">>> 清理完成! 当前可用: " << memMonitor->getMemoryStatus().availableMB << " MB" << std::endl;
        } else {
            std::cout << ">>> 失败: 权限不足 (需sudo)" << std::endl;
        }
        std::cout << "\n";
    }
}

void AiEngine::handleProcCommand(const std::string& action) {
    if (action == "LIST_PROC") {
        auto procs = procMonitor->getTopCpuProcesses(5);
        std::cout << "\n>>> [ 进程 Top 5 ] <<<\nPID\tCPU%\tNAME" << std::endl;
        for (const auto& p : procs) {
            std::cout << p.pid << "\t" << p.cpuPercent << "\t" << p.name << std::endl;
        }
        std::cout << "--------------------\n" << std::endl;
    } 
    else if (action.find("KILL_PROC") != std::string::npos) {
        // 提取 [KILL_PROC:xxxx] 中的名字
        std::string targetName = "";
        size_t s = action.find(":");
        size_t e = action.find("]");
        if (s != std::string::npos && e != std::string::npos) {
            targetName = action.substr(s + 1, e - s - 1);
        }
        
        // 去除空格
        targetName.erase(0, targetName.find_first_not_of(" "));
        targetName.erase(targetName.find_last_not_of(" ") + 1);

        if (targetName == "UNKNOWN" || targetName.empty()) {
            std::cout << "[AI] 请指定进程名。" << std::endl;
        } else {
            std::cout << "[AI] 目标进程名: [" << targetName << "]" << std::endl;
            int pid = procMonitor->findPidByName(targetName);
            
            if (pid != -1) {
                // 安全检查
                if (pid <= 1000) std::cout << ">>> 警告: 系统关键进程，拒绝操作！" << std::endl;
                else {
                    if (procControl->killProcess(pid)) std::cout << ">>> 成功终止 (PID " << pid << ")。" << std::endl;
                }
            } else {
                std::cout << ">>> 未找到该进程。" << std::endl;
            }
        }
        std::cout << "\n";
    }
}

// === 总调度 ===
void AiEngine::executeCommand(const AICommand& cmd) {
    if (cmd.action.find("CPU") != std::string::npos || cmd.action.find("MODE") != std::string::npos) {
        handleCpuCommand(cmd.action);
    } 
    else if (cmd.action.find("MEM") != std::string::npos) {
        handleMemCommand(cmd.action);
    } 
    else if (cmd.action.find("PROC") != std::string::npos) {
        handleProcCommand(cmd.action);
    } 
    else {
        std::cout << "[AI] 指令无法识别，或与系统管理无关。" << std::endl;
    }
}

void AiEngine::processInput(const std::string& input) {
    AICommand cmd = think(input);
    executeCommand(cmd);
}

void AiEngine::start() {
    std::cout << "\n=== AIOS Dome v0.6 (重构+安全版) ===" << std::endl;
    std::string input;
    while (true) {
        std::cout << "Admin@AIOS:~$ ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        if (input.empty()) continue;
        processInput(input);
    }
}