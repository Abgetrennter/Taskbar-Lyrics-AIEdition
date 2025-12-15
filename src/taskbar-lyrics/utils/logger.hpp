#pragma once
#include <string>

namespace Logger {
    void Init(const std::wstring& logDir);
    void Info(const char* format, ...);
    void Error(const char* format, ...);
    std::wstring GetLogPath();
}