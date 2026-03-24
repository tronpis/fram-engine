#include "Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <ctime>

namespace farm {

std::unique_ptr<LoggerImpl> Logger::s_instance;
std::atomic<bool> Logger::s_initialized{false};
std::mutex Logger::s_mutex;

void Logger::init(const LoggerConfig& config) {
    // Prevent double initialization with mutex protection
    std::lock_guard<std::mutex> lock(s_mutex);
    
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
    // Check if already shut down using atomic with mutex protection
    std::lock_guard<std::mutex> lock(s_mutex);
    
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
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_instance && s_initialized.load() && s_instance->initialized) {
        s_instance->config.level = level;
    }
}

LogLevel Logger::getLevel() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_instance && s_initialized.load() && s_instance->initialized) {
        return s_instance->config.level;
    }
    return LogLevel::Info;
}

bool Logger::isInitialized() {
    return s_initialized.load();
}

void Logger::logMessage(LogLevel level, const std::string& message) {
    // Thread-safe check using atomic and mutex
    // Note: s_mutex is already held by the caller (log template method)
    // so we don't need to acquire it again here
    
    // Double-check after acquiring lock (in case shutdown happened)
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

} // namespace farm
