#ifndef LIB_HEADER
#define LIB
#include <string>
namespace nyub {

enum class LogLevel {
    DEBUG,
    INFO,
    ERROR,
};

struct LogMessage {
    LogLevel level;
    std::string text;

    static LogMessage debug(std::string const &msg);
    static LogMessage info(std::string const &msg);
    static LogMessage error(std::string const &msg);
    bool operator==(LogMessage const &other) const;
    bool operator!=(LogMessage const &other) const;
};

std::string newString(const char *str);
} // namespace nyub

#endif // LIB_HEADER