//! @file Illustrates Googletest way to customize representation of data under
//! test

#include "lib.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace nyub { // operator overload must live in the same namespace as the
                 // printed value
std::ostream &operator<<(std::ostream &os, LogLevel const &logLevel) {
    std::string repr("<INVALID LOG LEVEL>");
    switch (logLevel) {
    case LogLevel::DEBUG:
        repr = "DEBUG";
        break;
    case LogLevel::INFO:
        repr = "INFO";
        break;
    case LogLevel::ERROR:
        repr = "ERROR";
        break;
    }
    return os << repr;
}

std::ostream &operator<<(std::ostream &os, nyub::LogMessage const &msg) {
    return os << "{ .level = " << msg.level << ", .text = " << msg.text << " }";
}
} // namespace nyub

TEST(Test, FailEquality) {
    nyub::LogMessage a{.level = nyub::LogLevel::ERROR, .text = "AaA"};
    nyub::LogMessage b{.level = nyub::LogLevel::DEBUG, .text = "BbB"};
    ASSERT_EQ(a, b);
}