/**
 * @file ai_engine.cpp
 * @brief AI 核心引擎 (物理隔离 + 专精Prompt版)
 */

#include "core/ai_engine.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <iomanip>
#include <memory>
#include <vector>
#include <chrono> // 用于 sleep

// ==========================================
//           工具函数区
// ==========================================

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

std::string AiEngine::extractJson(const std::string& jsonResponse) {
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
            else if (c == '"') break;
            else result += c;
        }
    }
    return result;
}

// 关键词匹配辅助
bool hasKey(const std::string& str, const std::string& key) {
    std::string s = str;
    std::string k = key;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    return s.find(k) != std::string::npos;
}

// ==========================================
//           初始化与通信
// ==========================================

AiEngine::AiEngine() {
    std::cout << "[Core] 加载模块: CPU | MEMORY | PROCESS" << std::endl;
    cpuMonitor = std::make_unique<CpuMonitor>();
    cpuControl = std::make_unique<CpuControl>();
    memMonitor = std::make_unique<MemMonitor>();
    memControl = std::make_unique<MemControl>();
    procMonitor = std::make_unique<ProcMonitor>();
    procControl = std::make_unique<ProcControl>();
    fileMonitor = std::make_unique<FileMonitor>(); // 新增：数据雷达模块
    fileControl = std::make_unique<FileControl>(); // 新增：文件控制模块
    fileCreator = std::make_unique<FileCreator>(); // 新增

    std::cout << "[Core] 系统就绪。请下达指令。" << std::endl;

    // 启动后台监控线程
    isMonitorRunning = false;
    keepRunning = false;

    std::cout << "[Core] 系统就绪。后台监控默认 [关闭]。" << std::endl;
}

// === 线程控制逻辑 ===

void AiEngine::startMonitor() {
    if (isMonitorRunning) {
        std::cout << ">>> [AI 哨兵] 已经在运行中，无需重复启动。" << std::endl;
        return;
    }

    std::cout << ">>> 正在启动后台监控线程..." << std::endl;
    
    // 重置状态
    // 为了防止 detectNewProcesses 刚启动就报一堆旧进程，我们先刷新一下快照但不打印
    procMonitor->detectNewProcesses(); 
    
    keepRunning = true;
    isMonitorRunning = true;
    
    // 创建新线程
    monitorThread = std::thread(&AiEngine::backgroundMonitorTask, this);
    std::cout << ">>> [AI 哨兵] 启动成功！现在我会盯着后台进程和异常。" << std::endl;
}

void AiEngine::stopMonitor() {
    if (!isMonitorRunning) {
        std::cout << ">>> [AI 哨兵] 已经是关闭状态。" << std::endl;
        return;
    }

    std::cout << ">>> 正在停止监控线程..." << std::endl;
    keepRunning = false; // 告诉线程循环该停了
    
    if (monitorThread.joinable()) {
        monitorThread.join(); // 等待线程彻底结束
    }
    
    isMonitorRunning = false;
    std::cout << ">>> [AI 哨兵] 已关闭。世界清静了。" << std::endl;
}

// 线程主体 (保持之前的逻辑，略微优化显示)
void AiEngine::backgroundMonitorTask() {
    while (keepRunning) {
        // 1. 新进程检测
        std::string newProcs = procMonitor->detectNewProcesses();
        if (!newProcs.empty()) {
            std::cout << "\r\033[K"; 
            std::cout << "\033[1;32m[AI 哨兵] 发现新活动:\033[0m\n" << newProcs << std::flush;
            std::cout << "Admin@AIOS:~$ " << std::flush; 
        }

        // 2. 异常检测
        std::string badProcs = procMonitor->detectAbnormalProcesses(90.0);
        if (!badProcs.empty()) {
            std::cout << "\r\033[K";
            std::cout << "\033[1;31m[AI 警告] 异常负载:\033[0m\n" << badProcs << std::flush;
            std::cout << "Admin@AIOS:~$ " << std::flush;
        }

        // 休眠 2 秒
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

AiEngine::~AiEngine() {
    // 停止监控线程
    stopMonitor();
    std::cout << "[Core] 系统已关闭。" << std::endl;
}

std::string AiEngine::callOllama(const std::string& promptText) {
    std::string safePrompt = jsonEscape(promptText);
    std::string jsonPayload = "{\"model\": \"" + modelName + "\", \"prompt\": \"" + safePrompt + "\", \"stream\": false}";
    std::string cmd = "curl -s -X POST " + ollamaUrl + " -d '" + jsonPayload + "'";
    
    std::string rawJson;
    char buffer[2048];
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer, 2048, pipe.get())) rawJson += buffer;
    return extractJson(rawJson);
}

// ==========================================
//           核心路由 (Router)
// ==========================================
// 这里不做AI分析，只做物理分流，确保绝对不会串台
void AiEngine::routeAndProcess(const std::string& input) {
    
    // 1. 判断是否是 CPU 相关
    if (hasKey(input, "cpu") || hasKey(input, "频率") || hasKey(input, "温度") || 
        hasKey(input, "性能") || hasKey(input, "省电") || hasKey(input, "模式")) {
        runCpuModule(input);
        return;
    }

    // 2. 判断是否是 内存 相关
    if (hasKey(input, "内存") || hasKey(input, "mem") || hasKey(input, "ram") || 
        hasKey(input, "垃圾") || hasKey(input, "清理")) {
        runMemModule(input);
        return;
    }

    // === 新增：监控控制路由 ===
    if (hasKey(input, "监控") || hasKey(input, "哨兵") || 
        hasKey(input, "monitor") || hasKey(input, "watch") || 
        hasKey(input, "守护")) {
        runMonitorModule(input);
        return;
    }

    // 3. 判断是否是 进程 相关 (优先级最低，作为兜底)
    // 凡是说“杀”、“关”、“任务”、“top”都算进程
    if (hasKey(input, "杀") || hasKey(input, "关") || hasKey(input, "进程") || 
        hasKey(input, "任务") || hasKey(input, "top") || hasKey(input, "kill")) {
        runProcModule(input);
        return;
    }

    // 4. 文件创建 (新建/创建) -> 优先路由
    if (hasKey(input, "创建") || hasKey(input, "create") || 
        hasKey(input, "新建") || hasKey(input, "new") || hasKey(input, "touch")) {
        runFileCreateModule(input);
        return;
    }

    // 5. 判断是否是 文件控制 相关
    if (hasKey(input, "打开") || hasKey(input, "open") || 
        hasKey(input, "删除") || hasKey(input, "delete") || 
        hasKey(input, "搜索") || hasKey(input, "查找") || hasKey(input, "find")) {
        runFileControlModule(input); // 调用控制模块
        return;
    }
    
    // 6. 判断是否是 文件 相关
    if (hasKey(input, "文件") || hasKey(input, "file") || hasKey(input, "磁盘") || 
        hasKey(input, "找") || hasKey(input, "搜索") || hasKey(input, "大于")) {
        runFileModule(input);
        return;
    }

    // 6. 未识别
    std::cout << "[Core] 未识别指令领域 (请输入: CPU / 内存 / 进程 相关指令)" << std::endl;
}

// ==========================================
//           功能区 1: CPU 模块
// ==========================================

std::string AiEngine::buildCpuPrompt(const std::string& input) {
    // 这里的 Prompt 只有 CPU 的概念，AI 不可能回答杀进程
    return "用户指令: [" + input + "]。\n"
           "请分类为:\n"
           "1. [CHECK] (查询CPU状态)\n"
           "2. [BOOST] (高性能/游戏模式)\n"
           "3. [RESTORE] (省电/默认模式)\n"
           "只回复标签。";
}

void AiEngine::runCpuModule(const std::string& input) {
    std::cout << "[CPU模块] 处理中..." << std::endl;
    std::string resp = callOllama(buildCpuPrompt(input));
    
    if (resp.find("CHECK") != std::string::npos) {
        double temp = cpuMonitor->getCpuTemperature();
        std::cout << ">>> CPU 使用率: " << cpuMonitor->getSystemCpuUsage() << "%" << std::endl;
        std::cout << ">>> CPU 主频  : " << cpuMonitor->getCpuFrequency() << " MHz" << std::endl;
        std::cout << ">>> CPU 温度  : " << (temp > 0 ? std::to_string(temp) + "C" : "N/A") << std::endl;
    }
    else if (resp.find("BOOST") != std::string::npos) {
        std::cout << ">>> 正在开启高性能模式..." << std::endl;
        // 直接调用底层，不搞虚假的模拟
        bool ok = cpuControl->boostPerformance();
        if (ok) std::cout << ">>> 成功。" << std::endl;
        else std::cout << ">>> 失败: 请使用 sudo 运行，或确认系统支持 cpufreq。" << std::endl;
    }
    else if (resp.find("RESTORE") != std::string::npos) {
        std::cout << ">>> 正在恢复默认模式..." << std::endl;
        cpuControl->restoreDefault();
        std::cout << ">>> 已执行。" << std::endl;
    }
    else {
        std::cout << ">>> (CPU模块) 无法理解的具体操作。" << std::endl;
    }
    std::cout << std::endl;
}

// ==========================================
//           功能区 2: 内存模块
// ==========================================

std::string AiEngine::buildMemPrompt(const std::string& input) {
    // 这里的 Prompt 只有内存的概念
    return "用户指令: [" + input + "]。\n"
           "请分类为:\n"
           "1. [CHECK] (查询内存)\n"
           "2. [CLEAN] (清理/释放内存)\n"
           "只回复标签。";
}

void AiEngine::runMemModule(const std::string& input) {
    std::cout << "[内存模块] 处理中..." << std::endl;
    std::string resp = callOllama(buildMemPrompt(input));

    if (resp.find("CHECK") != std::string::npos) {
        auto ms = memMonitor->getMemoryStatus();
        std::cout << ">>> 总内存: " << ms.totalMB << " MB" << std::endl;
        std::cout << ">>> 已用  : " << ms.usedMB << " MB (" << ms.usagePercent << "%)" << std::endl;
        std::cout << ">>> 可用  : " << ms.availableMB << " MB" << std::endl;
    }
    else if (resp.find("CLEAN") != std::string::npos) {
        std::cout << ">>> 正在清理缓存..." << std::endl;
        bool ok = memControl->dropCache();
        if (ok) {
            auto ms = memMonitor->getMemoryStatus();
            std::cout << ">>> 清理完成。当前可用: " << ms.availableMB << " MB" << std::endl;
        } else {
            std::cout << ">>> 失败: 权限不足 (必须 sudo)。" << std::endl;
        }
    }
    std::cout << std::endl;
}

// ==========================================
//           功能区 3: 监控控制 (Monitor)
// ==========================================

std::string AiEngine::buildMonitorPrompt(const std::string& input) {
    return "用户指令: [" + input + "]。\n"
           "这是一个系统监控开关任务。请分类：\n"
           "1. 开启监控/打开哨兵 -> [START_MONITOR]\n"
           "2. 关闭监控/停止哨兵 -> [STOP_MONITOR]\n"
           "3. 查询监控状态 -> [STATUS_MONITOR]\n"
           "只回复标签。";
}

void AiEngine::runMonitorModule(const std::string& input) {
    std::string resp = callOllama(buildMonitorPrompt(input));

    if (resp.find("START_MONITOR") != std::string::npos) {
        startMonitor();
    } 
    else if (resp.find("STOP_MONITOR") != std::string::npos) {
        stopMonitor();
    }
    else if (resp.find("STATUS_MONITOR") != std::string::npos) {
        if (isMonitorRunning) std::cout << ">>> [状态] 监控正在运行 (Active)。" << std::endl;
        else std::cout << ">>> [状态] 监控处于关闭状态 (Inactive)。" << std::endl;
    }
    else {
        std::cout << ">>> 未识别的监控指令。" << std::endl;
    }
}

// ==========================================
//           功能区 3: 进程模块
// ==========================================

std::string AiEngine::buildProcPrompt(const std::string& input) {
    // 专精于进程名转换，这是小模型最容易晕的地方，单独训练它
    return "用户指令: [" + input + "]。\n"
           "如果是查询，回复 [LIST]。\n"
           "如果是杀进程，回复 [KILL:进程英文名]。\n"
           "翻译规则：\n"
           "- 火狐 -> firefox\n"
           "- 谷歌/Chrome -> chrome\n"
           "- 代码/VSCode -> code\n"
           "- 终端 -> gnome-terminal\n"
           "- 文本 -> gedit\n"
           "只回复标签。";
}

void AiEngine::runProcModule(const std::string& input) {
    std::cout << "[进程模块] 处理中..." << std::endl;
    
    // 特殊优化：如果用户只输入 "top" 或 "ps"，直接列出，不用 AI 思考
    if (input == "top" || input == "ps" || input == "进程") {
        std::cout << ">>> 快速列表:" << std::endl;
        auto procs = procMonitor->getTopCpuProcesses(5);
        std::cout << "PID\tCPU%\tNAME" << std::endl;
        for (auto& p : procs) std::cout << p.pid << "\t" << p.cpuPercent << "\t" << p.name << std::endl;
        std::cout << std::endl;
        return;
    }

    // 复杂指令才调用 AI
    std::string resp = callOllama(buildProcPrompt(input));

    if (resp.find("LIST") != std::string::npos) {
        auto procs = procMonitor->getTopCpuProcesses(5);
        std::cout << "PID\tCPU%\tNAME" << std::endl;
        for (auto& p : procs) std::cout << p.pid << "\t" << p.cpuPercent << "\t" << p.name << std::endl;
    }
    else if (resp.find("KILL") != std::string::npos) {
        // 提取 [KILL:xxxx]
        std::string name = "";
        size_t s = resp.find(":");
        size_t e = resp.find("]");
        if (s != std::string::npos && e != std::string::npos) name = resp.substr(s+1, e-s-1);
        
        // 去空格
        name.erase(0, name.find_first_not_of(" "));
        name.erase(name.find_last_not_of(" ") + 1);

        if (name.empty()) {
            std::cout << ">>> AI 未能识别进程名。" << std::endl;
        } else {
            std::cout << ">>> 目标锁定: " << name << std::endl;
            int pid = procMonitor->findPidByName(name);
            if (pid > 0) {
                // 实机保护：不杀 PID < 1000
                if (pid < 1000) std::cout << ">>> 警告: 系统进程，禁止查杀。" << std::endl;
                else {
                    if (procControl->killProcess(pid)) std::cout << ">>> 进程已终止。" << std::endl;
                    else std::cout << ">>> 终止失败 (权限不足?)。" << std::endl;
                }
            } else {
                std::cout << ">>> 未找到运行中的进程: " << name << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

// ==========================================
//           功能区 4: 文件模块 (DataRadar)
// ==========================================

// 内部辅助：提取用户口中的数字 (例如 "1G" -> 1024, "500M" -> 500)
double _getFileSizeFromInput(const std::string& input) {
    std::string numStr;
    bool foundDigit = false;
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (isdigit(c) || c == '.') {
            numStr += c;
            foundDigit = true;
        } else if (foundDigit) {
            // 数字结束，看单位
            char unit = toupper(c);
            if (isspace(c)) continue; // 跳过空格
            try {
                double val = std::stod(numStr);
                if (unit == 'G') return val * 1024.0;
                if (unit == 'M') return val;
                break; // 其他单位或汉字，停止解析
            } catch (...) { break; }
        }
    }
    // 没单位的情况 (默认视为MB，或者处理 G 在末尾的情况)
    if (!numStr.empty()) {
        try {
            double val = std::stod(numStr);
            // 简单粗暴检查字符串里有没有 G
            std::string s = input;
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            if (s.find("G") != std::string::npos) return val * 1024.0;
            return val;
        } catch (...) {}
    }
    return 0.0;
}

std::string AiEngine::buildFilePrompt(const std::string& input) {
    return "用户指令: [" + input + "]。\n"
           "请分类：\n"
           "1. [FIND_LARGE] (找大文件，如：大于1G，找文件，清理磁盘)\n"
           "2. [SCAN_DISK] (强制重新扫描，建立索引)\n"
           "只回复标签。";
}

void AiEngine::runFileModule(const std::string& input) {
    std::cout << "[DataRadar] 解析指令..." << std::endl;
    std::string resp = callOllama(buildFilePrompt(input));

    // 1. C++ 强行介入：检查用户是否指定了大小
    double userSize = _getFileSizeFromInput(input);
    bool hasSizeRequest = (userSize > 0);

    // 2. 逻辑分支
    // 情况 A：用户明确指定了大小 (例如 "大于1G") -> 必须执行查找，无视 AI 的 SCAN
    if (hasSizeRequest || resp.find("FIND_LARGE") != std::string::npos) {
        double threshold = (userSize > 0) ? userSize : 100.0; // 有指定用指定的，没指定默认100M
        
        std::cout << ">>> 正在检索大于 " << threshold << " MB 的文件..." << std::endl;
        
        auto files = fileMonitor->getLargeFiles(threshold, 50);
        
        // 自动补救：如果没找到且索引为空，触发扫描
        if (files.empty()) {
            std::cout << ">>> (索引为空，正在自动全盘扫描...)" << std::endl;
            fileMonitor->scanDirectory(fileMonitor->getCurrentRoot());
            files = fileMonitor->getLargeFiles(threshold, 50);
        }

        if (files.empty()) {
            std::cout << ">>> 未找到大于 " << threshold << " MB 的文件。" << std::endl;
        } else {
            std::cout << "\n[大小]\t\t[路径] (Top " << files.size() << ")" << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            for (const auto& f : files) {
                std::cout << "[" << f.sizeStr << "]\t" << f.path << std::endl;
            }
            std::cout << "----------------------------------------\n" << std::endl;
        }
    }
    // 情况 B：纯扫描 (用户只说了 "扫描" 且没提数字)
    else if (resp.find("SCAN_DISK") != std::string::npos) {
        std::cout << ">>> 启动全盘扫描 (根目录: " << fileMonitor->getCurrentRoot() << ")..." << std::endl;
        int count = fileMonitor->scanDirectory(fileMonitor->getCurrentRoot());
        std::cout << ">>> 扫描完成! 发现 " << count << " 个大文件 (>10MB)。" << std::endl;
        
        // 顺手展示一下最大的
        auto files = fileMonitor->getLargeFiles(100.0, 5);
        if (!files.empty()) {
            std::cout << ">>> 最大的 5 个文件:" << std::endl;
            for (const auto& f : files) std::cout << "[" << f.sizeStr << "]\t" << f.path << std::endl;
        }
    }
    else {
        // 兜底
        std::cout << ">>> 指令模糊，默认列出 >100MB 文件:" << std::endl;
        auto files = fileMonitor->getLargeFiles(100.0, 20);
        for (const auto& f : files) std::cout << "[" << f.sizeStr << "]\t" << f.path << std::endl;
    }
    std::cout << std::endl;
}

// ==========================================
//      功能区 5: 文件控制 (File Control)
// ==========================================

std::string AiEngine::buildFileControlPrompt(const std::string& input) {
    return "用户指令: [" + input + "]。\n"
           "这是一个文件操作任务。请分类：\n"
           "1. 搜索/查找文件 -> [SEARCH:文件名]\n"
           "2. 打开/运行文件 -> [OPEN:文件名]\n"
           "3. 删除/移除文件 -> [DELETE:文件名]\n"
           "只回复标签。";
}

void AiEngine::runFileControlModule(const std::string& input) {
    std::cout << "[FileControl] 处理操作指令..." << std::endl;
    std::string resp = callOllama(buildFileControlPrompt(input));

    // 提取文件名
    std::string targetName = "";
    if (resp.find(":") != std::string::npos) {
        size_t s = resp.find(":");
        size_t e = resp.find("]");
        if (s != std::string::npos && e != std::string::npos) targetName = resp.substr(s + 1, e - s - 1);
        
        // 清理空格
        targetName.erase(0, targetName.find_first_not_of(" "));
        targetName.erase(targetName.find_last_not_of(" ") + 1);
    }

    if (targetName.empty()) {
        std::cout << ">>> AI 无法识别文件名，请说清楚点。" << std::endl;
        return;
    }

    // === 逻辑：先搜索，再操作 ===
    
    // 1. 搜索
    if (resp.find("SEARCH") != std::string::npos) {
        std::cout << ">>> 正在搜索: " << targetName << " ..." << std::endl;
        auto results = fileControl->searchFile(targetName);
        if (results.empty()) std::cout << ">>> 未找到。" << std::endl;
        else {
            std::cout << ">>> 找到 " << results.size() << " 个文件:" << std::endl;
            for (const auto& path : results) std::cout << " - " << path << std::endl;
        }
    }
    // 2. 打开
    else if (resp.find("OPEN") != std::string::npos) {
        std::cout << ">>> 正在定位: " << targetName << " ..." << std::endl;
        auto results = fileControl->searchFile(targetName);
        
        if (results.empty()) {
            std::cout << ">>> 找不到文件，无法打开。" << std::endl;
        } else if (results.size() == 1) {
            std::cout << ">>> 打开: " << results[0] << std::endl;
            fileControl->openFile(results[0]);
        } else {
            std::cout << ">>> 找到多个文件，请指定全名:" << std::endl;
            for (const auto& path : results) std::cout << " - " << path << std::endl;
        }
    }
    // 3. 删除
    else if (resp.find("DELETE") != std::string::npos) {
        std::cout << ">>> [危险] 正在定位: " << targetName << " ..." << std::endl;
        auto results = fileControl->searchFile(targetName);
        
        if (results.empty()) {
            std::cout << ">>> 文件不存在。" << std::endl;
        } else if (results.size() == 1) {
            std::cout << ">>> 目标: " << results[0] << std::endl;
            std::cout << ">>> 确认删除? (输入 yes): ";
            std::string confirm;
            std::getline(std::cin, confirm);
            if (confirm == "yes") {
                if (fileControl->deleteFile(results[0])) std::cout << ">>> 已删除。" << std::endl;
                else std::cout << ">>> 删除失败。" << std::endl;
            } else {
                std::cout << ">>> 已取消。" << std::endl;
            }
        } else {
            std::cout << ">>> 找到多个文件，无法模糊删除:" << std::endl;
            for (const auto& path : results) std::cout << " - " << path << std::endl;
        }
    }
}

// ==========================================
//           功能区 6: 文件创建 (FileCreator)
// ==========================================

std::string AiEngine::buildFileCreatePrompt(const std::string& input) {
    return "用户指令: [" + input + "]。\n"
           "这是一个创建文件的任务。\n"
           "请提取用户想要创建的文件路径或文件名。\n"
           "格式: [CREATE:文件名]\n"
           "如果用户没指定后缀，默认加上 .txt\n"
           "只回复标签。";
}

void AiEngine::runFileCreateModule(const std::string& input) {
    std::cout << "[FileCreator] 解析创建指令..." << std::endl;
    std::string resp = callOllama(buildFileCreatePrompt(input));

    std::string fileName = "";
    if (resp.find("CREATE:") != std::string::npos) {
        size_t s = resp.find(":");
        size_t e = resp.find("]");
        if (s != std::string::npos && e != std::string::npos) fileName = resp.substr(s + 1, e - s - 1);
        // 清理空格
        fileName.erase(0, fileName.find_first_not_of(" "));
        fileName.erase(fileName.find_last_not_of(" ") + 1);
    }

    if (fileName.empty()) {
        std::cout << ">>> AI 没听懂你想创建什么文件名，请重试。" << std::endl;
        return;
    }

    // 强制 .txt 检查 (虽然 FileCreator 也会检查，这里做个预判更好)
    if (fileName.find(".txt") == std::string::npos) {
        fileName += ".txt";
        std::cout << ">>> (自动添加 .txt 后缀)" << std::endl;
    }

    std::cout << ">>> 准备创建文件: " << fileName << std::endl;
    
    // === 交互环节 ===
    std::cout << ">>> 是否需要 AI 自动生成一些日志/内容写入该文件? (yes/no): ";
    std::string choice;
    std::getline(std::cin, choice);

    std::string contentToWrite = "";

    if (choice == "yes" || choice == "y") {
        std::cout << ">>> AI 正在生成日志内容..." << std::endl;
        // 让 AI 生成一段假日志
        std::string logPrompt = "请生成一段简短的、看起来很专业的系统运行日志，包含时间戳，3行左右。不要包含其他解释。";
        contentToWrite = callOllama(logPrompt);
        
        // 清理一下 AI 回复里可能带的引号
        contentToWrite.erase(std::remove(contentToWrite.begin(), contentToWrite.end(), '"'), contentToWrite.end());
        
        std::cout << ">>> 生成内容预览:\n" << contentToWrite << std::endl;
    } else {
        std::cout << ">>> 已跳过内容生成，将创建一个空文件。" << std::endl;
    }

    // 执行创建
    if (fileCreator->createTxtFile(fileName, contentToWrite)) {
        std::cout << ">>> [成功] 文件已创建。" << std::endl;
    } else {
        std::cout << ">>> [失败] 创建过程出错。" << std::endl;
    }
}

// ==========================================
//           主循环
// ==========================================

void AiEngine::start() {
    std::cout << "\n=== AIOS Dome v0.8 (物理分块版) ===" << std::endl;
    std::cout << "输入 'exit' 退出。" << std::endl;
    
    std::string input;
    while (true) {
        std::cout << "Admin@AIOS:~$ ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        if (input.empty()) continue;
        
        // 核心入口：先分流，再处理
        routeAndProcess(input);
    }
}