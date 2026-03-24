#include "Logger.h"

#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <ctime>
#include <format>

namespace farm {

// Forward declaration of implementation
class LoggerImpl {
public:
    LoggerConfig config;
    std::ofstream fileStream;
    std::mutex mutex;
    bool initialized = false;
    
    ~LoggerImpl() {
        if (fileStream.is_open()) {
            fileStream.close();
        }
    }
};

std::unique_ptr<LoggerImpl> Logger::s_instance;
std::atomic<bool> Logger::s_initialized{false};

void Logger::init(const LoggerConfig& config) {
    // Prevent double initialization
    bool expected = false;
    if (!s_initialized.compare_exchange_strong(expected, true)) {
        return;  // Already initialized or initializing
    }
    
    s_instance = std::make_unique<LoggerImpl>();
    s_instance->config = config;
    
    // Open log file if file output is enabled
    if (config.fileOutput) {
        s_instance->fileStream.open(config.logFile, std::ios::out | std::ios::app);
        if (!s_instance->fileStream.is_open()) {
            // Resource leak fix: clean up on failure
            s_instance.reset();
            s_initialized.store(false);
            std::cerr << "[FarmEngine] Failed to open log file: " << config.logFile << std::endl;
            return;
        }
    }
    
    s_instance->initialized = true;
    
    // Log initialization message (bypass normal logging to avoid recursion)
    logMessage(LogLevel::Info, "Logger initialized");
}

void Logger::shutdown() {
    // Check if already shut down using atomic
    bool expected = true;
    if (!s_initialized.compare_exchange_strong(expected, false)) {
        return;  // Already shut down or never initialized
    }
    
    // Log shutdown message before destroying instance
    if (s_instance && s_instance->initialized) {
        logMessage(LogLevel::Info, "Logger shutting down");
    }
    
    // Flush and close file
    if (s_instance && s_instance->fileStream.is_open()) {
        s_instance->fileStream.flush();
        s_instance->fileStream.close();
    }
    
    // Reset instance - no logging after this point
    s_instance.reset();
}

void Logger::setLevel(LogLevel level) {
    if (s_instance && s_initialized.load()) {
        s_instance->config.level = level;
    }
}

LogLevel Logger::getLevel() {
    if (s_instance && s_initialized.load()) {
        return s_instance->config.level;
    }
    return LogLevel::Info;
}

bool Logger::isInitialized() {
    return s_initialized.load();
}

template<typename... Args>
void Logger::log(LogLevel level, const std::string& format, Args&&... args) {
    // Thread-safe check: don't log if not initialized
    if (!s_initialized.load() || !s_instance || !s_instance->initialized) {
        return;
    }
    
    if (level < s_instance->config.level) {
        return;  // Below minimum log level
    }
    
    // Use std::format for proper variadic formatting (C++20)
    std::string message;
    try {
        message = std::vformat(format, std::make_format_args(args...));
    } catch (const std::format_error& e) {
        // Fallback: use format string as-is if formatting fails
        message = format + std::string(" [format error: ") + e.what() + "]";
    }
    
    logMessage(level, message);
}

template<typename... Args>
void Logger::trace(const std::string& format, Args&&... args) {
    log(LogLevel::Trace, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::debug(const std::string& format, Args&&... args) {
    log(LogLevel::Debug, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::info(const std::string& format, Args&&... args) {
    log(LogLevel::Info, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::warn(const std::string& format, Args&&... args) {
    log(LogLevel::Warn, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::error(const std::string& format, Args&&... args) {
    log(LogLevel::Error, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::fatal(const std::string& format, Args&&... args) {
    log(LogLevel::Fatal, format, std::forward<Args>(args)...);
}

void Logger::logMessage(LogLevel level, const std::string& message) {
    // Thread-safe check using atomic
    if (!s_initialized.load() || !s_instance || !s_instance->initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(s_instance->mutex);
    
    // Double-check after acquiring lock (in case shutdown happened)
    if (!s_instance || !s_instance->initialized) {
        return;
    }
    
    // Build the log line
    std::ostringstream oss;
    
    // Add timestamp if enabled
    if (s_instance->config.timestamp) {
        oss << getTimestamp() << " ";
    }
    
    // Add level with color if enabled
    if (s_instance->config.coloredOutput && s_instance->config.consoleOutput) {
        oss << getColorCode(level) << "[" << formatLevel(level) << "]" << "\033[0m";
    } else {
        oss << "[" << formatLevel(level) << "]";
    }
    
    oss << ": " << message;
    
    std::string logLine = oss.str();
    
    // Output to console if enabled
    if (s_instance->config.consoleOutput) {
        if (s_instance->config.coloredOutput) {
            std::cout << logLine << std::endl;
        } else {
            std::cout << "[" << formatLevel(level) << "]: " << message << std::endl;
        }
    }
    
    // Output to file if enabled
    if (s_instance->config.fileOutput && s_instance->fileStream.is_open()) {
        // Strip ANSI codes for file output
        std::string cleanLine = logLine;
        // Remove ANSI escape codes
        size_t pos = 0;
        while ((pos = cleanLine.find("\033[")) != std::string::npos) {
            size_t end = cleanLine.find("m", pos);
            if (end != std::string::npos) {
                cleanLine.erase(pos, end - pos + 1);
            } else {
                break;
            }
        }
        
        s_instance->fileStream << cleanLine << std::endl;
        s_instance->fileStream.flush();
    }
}

std::string Logger::formatLevel(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default:              return "UNKNOWN";
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    std::tm timeInfo;
#ifdef _WIN32
    // Thread-safe version on Windows
    localtime_s(&timeInfo, &time);
    oss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
#else
    // Thread-safe version on POSIX - use thread_local buffer
    // localtime_r is thread-safe as it writes to user-provided buffer
    localtime_r(&time, &timeInfo);
    oss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
#endif
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::getColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "\033[90m";  // Bright Black
        case LogLevel::Debug: return "\033[36m";  // Cyan
        case LogLevel::Info:  return "\033[32m";  // Green
        case LogLevel::Warn:  return "\033[33m";  // Yellow
        case LogLevel::Error: return "\033[31m";  // Red
        case LogLevel::Fatal: return "\033[95m";  // Bright Magenta
        default:              return "\033[0m";   // Reset
    }
}

// Explicit template instantiations for common types
template void Logger::log<std::string>(LogLevel, const std::string&, std::string&&);
template void Logger::log<const char*>(LogLevel, const std::string&, const char*&&);
template void Logger::log<int>(LogLevel, const std::string&, int&&);
template void Logger::log<double>(LogLevel, const std::string&, double&&);
template void Logger::log<float>(LogLevel, const std::string&, float&&);

template void Logger::trace<std::string>(const std::string&, std::string&&);
template void Logger::trace<const char*>(const std::string&, const char*&&);
template void Logger::trace<int>(const std::string&, int&&);

template void Logger::debug<std::string>(const std::string&, std::string&&);
template void Logger::debug<const char*>(const std::string&, const char*&&);
template void Logger::debug<int>(const std::string&, int&&);

template void Logger::info<std::string>(const std::string&, std::string&&);
template void Logger::info<const char*>(const std::string&, const char*&&);
template void Logger::info<int>(const std::string&, int&&);
template void Logger::info<double>(const std::string&, double&&);

template void Logger::warn<std::string>(const std::string&, std::string&&);
template void Logger::warn<const char*>(const std::string&, const char*&&);
template void Logger::warn<int>(const std::string&, int&&);

template void Logger::error<std::string>(const std::string&, std::string&&);
template void Logger::error<const char*>(const std::string&, const char*&&);
template void Logger::error<int>(const std::string&, int&&);

template void Logger::fatal<std::string>(const std::string&, std::string&&);
template void Logger::fatal<const char*>(const std::string&, const char*&&);
template void Logger::fatal<int>(const std::string&, int&&);

} // namespace farm
