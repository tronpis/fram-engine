#include "Logger.h"

#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>

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

void Logger::init(const LoggerConfig& config) {
    if (s_instance && s_instance->initialized) {
        return;  // Already initialized
    }
    
    s_instance = std::make_unique<LoggerImpl>();
    s_instance->config = config;
    
    // Open log file if file output is enabled
    if (config.fileOutput) {
        s_instance->fileStream.open(config.logFile, std::ios::out | std::ios::app);
        if (!s_instance->fileStream.is_open()) {
            std::cerr << "[FarmEngine] Failed to open log file: " << config.logFile << std::endl;
        }
    }
    
    s_instance->initialized = true;
    
    // Log initialization message
    logMessage(LogLevel::Info, "Logger initialized");
}

void Logger::shutdown() {
    if (!s_instance) {
        return;
    }
    
    logMessage(LogLevel::Info, "Logger shutting down");
    
    // Flush and close file
    if (s_instance->fileStream.is_open()) {
        s_instance->fileStream.flush();
        s_instance->fileStream.close();
    }
    
    s_instance.reset();
}

void Logger::setLevel(LogLevel level) {
    if (s_instance) {
        s_instance->config.level = level;
    }
}

LogLevel Logger::getLevel() {
    if (s_instance) {
        return s_instance->config.level;
    }
    return LogLevel::Info;
}

template<typename... Args>
void Logger::log(LogLevel level, const std::string& format, Args&&... args) {
    if (!s_instance || !s_instance->initialized) {
        return;
    }
    
    if (level < s_instance->config.level) {
        return;  // Below minimum log level
    }
    
    // Simple formatting (in production, use fmt library)
    std::string message = format;
    
    // For now, just pass through the format string
    // A real implementation would use fmt::format or similar
    
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
    if (!s_instance || !s_instance->initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(s_instance->mutex);
    
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
    localtime_s(&timeInfo, &time);
    oss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
#else
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

} // namespace farm
