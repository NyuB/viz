#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "lib.hpp"
using namespace nyub;

//! Cycle through log levels
LogLevel next(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::DEBUG:
        return LogLevel::INFO;
    case LogLevel::INFO:
        return LogLevel::ERROR;
    case LogLevel::ERROR:
        return LogLevel::DEBUG;
    }
    return static_cast<LogLevel>(-1);
}

int main(int argc, char **argv) {
    size_t msgCount = (argc < 2) ? 7 : std::stoi(argv[1]);
    LogLevel lvl = LogLevel::DEBUG;
    std::vector<LogMessage> messages;
    for (size_t i = 0; i < msgCount; i++) {
        // >>> Breakpoint here, there is stuff in scope to inspect
        messages.push_back(
            LogMessage{lvl, std::string("Message") + " #" + std::to_string(i)});
        lvl = next(lvl);
    }
    const auto first = messages[0];
    std::cout << "First message is " << first.text << std::endl;
    for (const auto &msg : messages) {
        std::cout << static_cast<int>(msg.level) << " " << msg.text
                  << std::endl;
    }
}