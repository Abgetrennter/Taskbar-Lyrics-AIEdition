#include "logger.hpp"
#include <fstream>
#include <cstdarg>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <vector>
#include <Windows.h>

namespace Logger {
    static std::ofstream logFile;
    static std::mutex logMutex;
    static std::wstring currentLogPath;

    void Init(const std::wstring& logDir) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        // Ensure directory exists
        CreateDirectory(logDir.c_str(), NULL);

        currentLogPath = logDir + L"\\taskbar-lyrics.log";
        logFile.open(currentLogPath, std::ios::out | std::ios::app);
    }

    std::wstring GetLogPath() {
        return currentLogPath;
    }

    void WriteLog(const char* level, const char* format, va_list args) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (!logFile.is_open()) return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm;
        localtime_s(&tm, &time);

        char timeBuf[32];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);

        logFile << "[" << timeBuf << "." << std::setfill('0') << std::setw(3) << ms.count() << "] [" << level << "] ";
        
        // Determine buffer size
        va_list argsCopy;
        va_copy(argsCopy, args);
        int len = vsnprintf(nullptr, 0, format, argsCopy);
        va_end(argsCopy);

        if (len > 0) {
            std::vector<char> buf(len + 1);
            vsnprintf(buf.data(), len + 1, format, args);
            logFile << buf.data();
        }
        
        logFile << std::endl;
    }

    void Info(const char* format, ...) {
        va_list args;
        va_start(args, format);
        WriteLog("INFO", format, args);
        va_end(args);
    }

    void Error(const char* format, ...) {
        va_list args;
        va_start(args, format);
        WriteLog("ERROR", format, args);
        va_end(args);
    }
}