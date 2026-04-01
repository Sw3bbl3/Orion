#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <fstream>

namespace Orion {

    enum class LogLevel {
        Info,
        Warning,
        Error,
        Debug,
        Success
    };

    struct LogMessage {
        std::string message;
        LogLevel level;
        std::string timestamp;
    };

    class Logger {
    public:
        static void init(const std::string& filename);
        static void log(const std::string& message, LogLevel level = LogLevel::Info);

        static void info(const std::string& message);
        static void warn(const std::string& message);
        static void error(const std::string& message);
        static void debug(const std::string& message);

        static std::vector<LogMessage> getEntries();
        static void clear();

    private:
        static std::vector<LogMessage> entries;
        static std::mutex mutex;
        static std::ofstream logFile;
    };

}
