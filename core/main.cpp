/**
 * @file main.cpp
 * @brief 程序入口
 */

#include "core/ai_engine.h"
#include <iostream>
#include <unistd.h> // for geteuid

int main() {
    // 1. 权限检查
    // 如果不是 root 用户，警告用户 CPU 控制功能可能失效
    if (geteuid() != 0) {
        std::cerr << "\n[WARNING] You are not running as ROOT." << std::endl;
        std::cerr << "Hardware control (Boost/Restore) will fail." << std::endl;
        std::cerr << "Please run with: sudo ./aios_dome\n" << std::endl;
    }

    // 2. 实例化并启动
    try {
        AiEngine engine;
        engine.start();
    } catch (const std::exception& e) {
        std::cerr << "Critical Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}