#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <fstream>
#include <mutex>
#include <fmt/format.h>

namespace farm {

/**
 * @brief Log levels for different severity of messages
 */
enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

/**
 * @brief Logger configuration
 */
struct LoggerConfig {
    LogLevel level = LogLevel::Info;
    bool consoleOutput = true;
    bool fileOutput = true;
    std::string logFile = "farmengine.log";
    bool coloredOutput = true;
    bool timestamp = true;
};

/**
 * @brief Internal implementation class for Logger
 * 
 * This class holds the actual logger state and is defined here to allow
 * the template log() function in Logger to access its members safely.
 */
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

/**
 * @brief Core logging system for FarmEngine
 * 
 * Provides thread-safe, multi-output logging with configurable levels.
 * Supports console output with colors and file output.
 * 
 * Usage:
 * @code
 * Logger::init();
 * FARM_LOG_INFO("Engine started");
 * FARM_LOG_ERROR("Something went wrong: {}", error);
 * @endcode
 */
class Logger {
public:
    /**
     * @brief Initialize the logger system
     * @param config Configuration options
     */
    static void init(const LoggerConfig& config = LoggerConfig());
    
    /**
     * @brief Shutdown the logger and flush all buffers
     */
    static void shutdown();
    
    /**
     * @brief Set the minimum log level
     */
    static void setLevel(LogLevel level);
    
    /**
     * @brief Get current log level
     */
    static LogLevel getLevel();
    
    /**
     * @brief Check if logger is initialized
     */
    static bool isInitialized();
    
    /**
     * @brief Log a message with format
     * @param level Log level
     * @param format Format string (fmt-like)
     * @param args Arguments to format
     */
    template<typename... Args>
    static void log(LogLevel level, const std::string& format, Args&&... args) {
        // Thread-safe check: don't log if not initialized
        if (!s_initialized.load() || !s_instance) {
            return;
        }
        
        // Check log level - capture level value while holding no lock for performance
        // s_instance is safe to access here because we checked s_initialized and s_instance
        // is only destroyed after s_initialized is set to false. However, config.level
        // could be modified by setLevel() concurrently, so we use a local copy approach.
        // The worst case is a stale read which just means one extra log call, not UB.
        std::lock_guard<std::mutex> lock(s_instance->mutex);
        LoggerImpl* instance = s_instance.get();
        if (!instance || !instance->initialized) {
            return;
        }
        LogLevel currentLevel = instance->config.level;
        if (level < currentLevel) {
            return;  // Below minimum log level
        }
        
        // Double-check initialized flag after accessing instance
        if (!instance->initialized) {
            return;
        }
        
        // Use fmt::format for proper variadic formatting
        std::string message;
        try {
            message = fmt::format(format, std::forward<Args>(args)...);
        } catch (const fmt::format_error& e) {
            // Fallback: use format string as-is if formatting fails
            message = format + std::string(" [format error: ") + e.what() + "]";
        }
        
        logMessage(level, message);
    }
    
    /**
     * @brief Log a trace message
     */
    template<typename... Args>
    static void trace(const std::string& format, Args&&... args) {
        log(LogLevel::Trace, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log a debug message
     */
    template<typename... Args>
    static void debug(const std::string& format, Args&&... args) {
        log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log an info message
     */
    template<typename... Args>
    static void info(const std::string& format, Args&&... args) {
        log(LogLevel::Info, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log a warning message
     */
    template<typename... Args>
    static void warn(const std::string& format, Args&&... args) {
        log(LogLevel::Warn, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log an error message
     */
    template<typename... Args>
    static void error(const std::string& format, Args&&... args) {
        log(LogLevel::Error, format, std::forward<Args>(args)...);
    }
    
    /**
     * @brief Log a fatal message
     */
    template<typename... Args>
    static void fatal(const std::string& format, Args&&... args) {
        log(LogLevel::Fatal, format, std::forward<Args>(args)...);
    }
    
private:
    static void logMessage(LogLevel level, const std::string& message);
    static std::string formatLevel(LogLevel level);
    static std::string getTimestamp();
    static std::string getColorCode(LogLevel level);
    
    static std::unique_ptr<class LoggerImpl> s_instance;
    static std::atomic<bool> s_initialized;
};

} // namespace farm

// Convenience macros
#define FARM_LOG_TRACE(...) ::farm::Logger::trace(__VA_ARGS__)
#define FARM_LOG_DEBUG(...) ::farm::Logger::debug(__VA_ARGS__)
#define FARM_LOG_INFO(...) ::farm::Logger::info(__VA_ARGS__)
#define FARM_LOG_WARN(...) ::farm::Logger::warn(__VA_ARGS__)
#define FARM_LOG_ERROR(...) ::farm::Logger::error(__VA_ARGS__)
#define FARM_LOG_FATAL(...) ::farm::Logger::fatal(__VA_ARGS__)
