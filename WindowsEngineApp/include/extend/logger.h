#ifndef _H_LOGGER_H
#define _H_LOGGER_H

// 平台检测和日志定义
#ifdef PLATFORM_ANDROID
    #include <android/log.h>
    
    #define LOG_TAG_DEFAULT "Engine"
    
    #define LOGV(tag, ...) __android_log_print(ANDROID_LOG_VERBOSE, tag, __VA_ARGS__)
    #define LOGD(tag, ...) __android_log_print(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
    #define LOGI(tag, ...) __android_log_print(ANDROID_LOG_INFO, tag, __VA_ARGS__)
    #define LOGW(tag, ...) __android_log_print(ANDROID_LOG_WARN, tag, __VA_ARGS__)
    #define LOGE(tag, ...) __android_log_print(ANDROID_LOG_ERROR, tag, __VA_ARGS__)
    
    // 便捷宏，使用默认标签
    #define LOGI_DEFAULT(...) LOGI(LOG_TAG_DEFAULT, __VA_ARGS__)
    #define LOGE_DEFAULT(...) LOGE(LOG_TAG_DEFAULT, __VA_ARGS__)
    
#else
    #include <iostream>
    #include <cstdio>
    #include <fstream>
    #include <mutex>
    #include <string>

    // 日志文件管理类 - 前向声明
    class Logger {
    private:
        static std::ofstream logFile;
        static std::mutex logMutex;
        static bool initialized;
        
    public:
        static bool init(const std::string& filename = "engine.log");
        static void close();
        static void write(const char* level, const char* tag, const char* message);
    };
    
    // Windows/Linux 平台的日志宏 - 同时输出到控制台和文件
    #define LOGV(tag, format, ...) do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
        printf("[V][%s] %s\n", tag, buffer); \
        Logger::write("V", tag, buffer); \
    } while(0)
    
    #define LOGD(tag, format, ...) do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
        printf("[D][%s] %s\n", tag, buffer); \
        Logger::write("D", tag, buffer); \
    } while(0)
    
    #define LOGI(tag, format, ...) do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
        printf("[I][%s] %s\n", tag, buffer); \
        Logger::write("I", tag, buffer); \
    } while(0)
    
    #define LOGW(tag, format, ...) do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
        printf("[W][%s] %s\n", tag, buffer); \
        Logger::write("W", tag, buffer); \
    } while(0)
    
    #define LOGE(tag, format, ...) do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
        fprintf(stderr, "[E][%s] %s\n", tag, buffer); \
        Logger::write("E", tag, buffer); \
    } while(0)
    
    // 便捷宏，使用默认标签
    #define LOGI_DEFAULT(format, ...) LOGI("Engine", format, ##__VA_ARGS__)
    #define LOGE_DEFAULT(format, ...) LOGE("Engine", format, ##__VA_ARGS__)
    
#endif // PLATFORM_ANDROID

#endif // LOGGER_H