#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace rv::util {

Logger& Logger::Instance() {
    static Logger logger;
    return logger;
}

void Logger::Info(const std::string& message) {
    Log("INFO", message);
}

void Logger::Warn(const std::string& message) {
    Log("WARN", message);
}

void Logger::Error(const std::string& message) {
    Log("ERROR", message);
}

void Logger::Log(const char* level, const std::string& message) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &nowTime);
#else
    localTime = *std::localtime(&nowTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%H:%M:%S") << " [" << level << "] " << message;

    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << oss.str() << std::endl;
}

} // namespace rv::util
