#include "lib.hpp"
namespace nyub {
LogMessage LogMessage::debug(std::string const &msg) {
    return LogMessage{.level = LogLevel::DEBUG, .text = msg};
}

LogMessage LogMessage::info(std::string const &msg) {
    return LogMessage{.level = LogLevel::INFO, .text = msg};
}

LogMessage LogMessage::error(std::string const &msg) {
    return LogMessage{.level = LogLevel::ERROR, .text = msg};
}

bool LogMessage::operator==(LogMessage const &other) const {
    return level == other.level && text == other.text;
}
/**
 * \note This needs to be overloaded for ASSERT_NE even if operator== is already
 * defined
 */
bool LogMessage::operator!=(LogMessage const &other) const {
    return !(*this == other);
}

std::string newString(const char *str) { return std::string(str); }
} // namespace nyub
