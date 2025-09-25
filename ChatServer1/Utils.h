#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// TODO: 调用，LinkERROR
// 获取符合格式的当前时间字符串 "YYYY-MM-DD HH:MM:SS"
static std::string getDateTimeStr() {
    auto now = std::chrono::system_clock::now();             // 获取当前时间点
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now); // 转成 time_t
    std::tm tm_now;

    #if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm_now, &now_time_t); // Windows 安全函数
    #else
        localtime_r(&now_time_t, &tm_now); // Linux/Unix
    #endif

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");     // 格式化
    return oss.str();
}

static std::string getDateStr() {
    auto now = std::chrono::system_clock::now();             // 获取当前时间点
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now); // 转成 time_t
    std::tm tm_now;

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm_now, &now_time_t); // Windows 安全函数
#else
    localtime_r(&now_time_t, &tm_now); // Linux/Unix
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d");     // 格式化
    return oss.str();
}