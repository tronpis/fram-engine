#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <atomic>

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
    static void log(LogLevel level, const std::string& format, Args&&... args);
    
    /**
     * @brief Log a trace message
     */
    template<typename... Args>
    static void trace(const std::string& format, Args&&... args);
    
    /**
     * @brief Log a debug message
     */
    template<typename... Args>
    static void debug(const std::string& format, Args&&... args);
    
    /**
     * @brief Log an info message
     */
    template<typename... Args>
    static void info(const std::string& format, Args&&... args);
    
    /**
     * @brief Log a warning message
     */
    template<typename... Args>
    static void warn(const std::string& format, Args&&... args);
    
    /**
     * @brief Log an error message
     */
    template<typename... Args>
    static void error(const std::string& format, Args&&... args);
    
    /**
     * @brief Log a fatal message
     */
    template<typename... Args>
    static void fatal(const std::string& format, Args&&... args);
    
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
