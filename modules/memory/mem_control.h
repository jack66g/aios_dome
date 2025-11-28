/**
 * @file mem_control.h
 * @brief 内存控制模块头文件
 * @details 负责内存清理与回收
 */

#ifndef MEM_CONTROL_H
#define MEM_CONTROL_H

#include <string>

class MemControl {
public:
    MemControl();
    ~MemControl();

    /**
     * @brief 释放系统缓存 (Drop Caches)
     * @details 写入 /proc/sys/vm/drop_caches
     * 注意：这是破坏性的性能操作，但在内存紧缺时有用。
     * @return bool 是否成功 (需要 Root 权限)
     */
    bool dropCache();
};

#endif // MEM_CONTROL_H