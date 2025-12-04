cmake_minimum_required(VERSION 3.10)

# 项目名称
project(aios_dome)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录
# 这样你在代码中可以使用 #include "modules/cpu/cpu_monitor.h"
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/modules
)

# 查找所有源文件
# 目前包含 main.cpp 和 cpu_monitor.cpp
# 注意：随着你开发 memory 和 process 模块，需要在这里添加对应的 .cpp 文件
set(SOURCES
    core/main.cpp
    core/ai_engine.cpp
    modules/cpu/cpu_monitor.cpp
    modules/cpu/cpu_control.cpp
    modules/memory/mem_monitor.cpp
    modules/memory/mem_control.cpp
    process/proc_monitor.cpp
    process/proc_control.cpp
    file/file_monitor.cpp
    file/file_control.cpp
    file/file_creator.cpp
    # 未来添加:
    # modules/cpu/cpu_control.cpp
    # modules/memory/mem_monitor.cpp
    # ...
)

# 生成可执行文件
add_executable(aios_dome ${SOURCES})

# 链接线程库 (如果是多线程开发通常需要)
find_package(Threads REQUIRED)
target_link_libraries(aios_dome Threads::Threads)