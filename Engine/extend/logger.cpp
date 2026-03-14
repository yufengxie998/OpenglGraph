#include <extend/logger.h>

#ifndef PLATFORM_ANDROID

// 静态成员定义
std::ofstream Logger::logFile;
std::mutex Logger::logMutex;
bool Logger::initialized = false;

bool Logger::init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (initialized) {
        return true;
    }
    
    // 以追加模式打开日志文件
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        return false;
    }
    
    initialized = true;
    
    // 写入日志文件头
    time_t now = time(nullptr);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    logFile << "\n========== Log started at " << timestamp << " ==========\n" << std::flush;
    
    return true;
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        time_t now = time(nullptr);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        logFile << "========== Log ended at " << timestamp << " ==========\n\n" << std::flush;
        logFile.close();
    }
    initialized = false;
}

void Logger::write(const char* level, const char* tag, const char* message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!initialized) {
        // 如果未初始化，尝试使用默认文件名初始化
        if (!init()) {
            return;
        }
    }
    
    if (logFile.is_open()) {
        // 获取当前时间
        time_t now = time(nullptr);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        // 写入日志文件
        logFile << "[" << timestamp << "][" << level << "][" << tag << "] " << message << std::endl;
        logFile.flush();  // 立即刷新到文件，确保日志不丢失
    }
}

#endif // PLATFORM_ANDROID