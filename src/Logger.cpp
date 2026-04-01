#include "orion/Logger.h"
#include <iostream>
#include <vector>
#include <mutex>
#include <fstream>
#include <juce_core/juce_core.h>

namespace Orion {

    std::vector<LogMessage> Logger::entries;
    std::mutex Logger::mutex;
    std::ofstream Logger::logFile;

    void Logger::init(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex);
        if (logFile.is_open()) {
            logFile.close();
        }
        logFile.open(filename, std::ios::out | std::ios::app);
        if (logFile.is_open()) {
            auto now = juce::Time::getCurrentTime();
            logFile << "\n=== Session Started: " << now.toString(true, true) << " ===\n";
            logFile.flush();
        }
    }

    void Logger::log(const std::string& message, LogLevel level) {
        std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
        if (!lock.owns_lock()) return;

        std::string timestamp = juce::Time::getCurrentTime().toString(true, true).toStdString();
        entries.push_back({message, level, timestamp});


        if (entries.size() > 1000) {
            entries.erase(entries.begin());
        }

        std::string levelStr;
        std::string colorCode;

        switch(level) {
            case LogLevel::Info:    levelStr = "[INFO] ";    colorCode = "\033[0m"; break;
            case LogLevel::Warning: levelStr = "[WARN] ";    colorCode = "\033[33m"; break;
            case LogLevel::Error:   levelStr = "[ERROR] ";   colorCode = "\033[31m"; break;
            case LogLevel::Debug:   levelStr = "[DEBUG] ";   colorCode = "\033[36m"; break;
            case LogLevel::Success: levelStr = "[OK]    ";   colorCode = "\033[32m"; break;
        }

        std::string fullMsg = timestamp + " " + levelStr + message;


        std::cout << colorCode << fullMsg << "\033[0m" << std::endl;


        juce::Logger::writeToLog(fullMsg);


        if (logFile.is_open()) {
            logFile << fullMsg << std::endl;
            logFile.flush();
        }
    }

    void Logger::info(const std::string& message) {
        log(message, LogLevel::Info);
    }

    void Logger::warn(const std::string& message) {
        log(message, LogLevel::Warning);
    }

    void Logger::error(const std::string& message) {
        log(message, LogLevel::Error);
    }

    void Logger::debug(const std::string& message) {
        log(message, LogLevel::Debug);
    }

    std::vector<LogMessage> Logger::getEntries() {
        std::lock_guard<std::mutex> lock(mutex);
        return entries;
    }

    void Logger::clear() {
        std::lock_guard<std::mutex> lock(mutex);
        entries.clear();
    }

}
