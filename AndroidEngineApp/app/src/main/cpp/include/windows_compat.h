#ifndef _H_WINDOWS_COMPAT_H
#define _H_WINDOWS_COMPAT_H

#ifdef _WIN32
    // 安全地定义 Windows 优化宏
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    
    // NOMINMAX 通常已由 C++ 标准库定义
    #include <windows.h>
    
    // 始终取消 byte 宏定义，避免与 std::byte 冲突
    #ifdef byte
        #undef byte
    #endif
    
    // 定义睡眠宏
    #ifndef SLEEP
        #define SLEEP(ms) Sleep(ms)
    #endif
    
    // Windows 错误码
    #ifndef LAST_ERROR
        #define LAST_ERROR() GetLastError()
    #endif
    
#else // Unix-like
    #include <unistd.h>
    #include <time.h>
    
    // 使用 nanosleep 替代 usleep
    inline void sleep_ms(unsigned int ms) {
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000;
        nanosleep(&ts, NULL);
    }
    
    #ifndef SLEEP
        #define SLEEP(ms) sleep_ms(ms)
    #endif
    
    #ifndef LAST_ERROR
        #define LAST_ERROR() errno
    #endif
#endif

#endif // WINDOWS_COMPAT_H
