//! @file Illustrates Googletest way to customize representation of data under
//! test

#include "lib.hpp"
#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
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

    EXPECT_FATAL_FAILURE(
        ({
            nyub::LogMessage a{.level = nyub::LogLevel::ERROR, .text = "AaA"};
            nyub::LogMessage b{.level = nyub::LogLevel::DEBUG, .text = "BbB"};
            ASSERT_EQ(a, b);
        }),
        "{ .level = ERROR, .text = AaA }");
}

template <class Err> class failed {
  public:
    failed(Err const &error) : m_error(error) {}
    Err error() const { return m_error; }

  private:
    Err m_error;
};

template <typename Value, typename Err>
class Result : private std::variant<Value, failed<Err>> {
  public:
    Result(const Value &a) : std::variant<Value, failed<Err>>(a) {}
    Result(const failed<Err> &ko) : std::variant<Value, failed<Err>>(ko) {}
    bool has_value() const { return std::holds_alternative<Value>(*this); }
    bool is_failure() const {
        return std::holds_alternative<failed<Err>>(*this);
    }
    operator bool() const { return has_value(); }

    Value value() const { return std::get<Value>(*this); }
    Err error() const { return std::get<failed<Err>>(*this).error(); }

    template <typename A, class Function>
    Result<A, Err> andThen(Function const &f) const {
        if (is_failure()) {
            return Result<A, Err>(failed(error()));
        }
        return Result<A, Err>(f(value()));
    }
};

MATCHER(Succeeded, "to be sucessfull") {
    if (arg.is_failure()) {
        *result_listener << "failed with <"
                         << testing::PrintToString(arg.error()) << ">";
        return false;
    }
    return true;
}

MATCHER(Failed, "to be a failure") {
    if (arg.has_value()) {
        *result_listener << "succeeded with <"
                         << testing::PrintToString(arg.value()) << ">";
        return false;
    }
    return true;
}

template <class Value, class Err = std::string>
testing::Matcher<Result<Value, Err>> SucceededWith(Value const &v) {
    return testing::AllOf(
        Succeeded(),
        testing::Property("value", &Result<Value, Err>::value, testing::Eq(v)));
}

template <class Value, class Err = std::string>
testing::Matcher<Result<Value, Err>> FailedWith(Err const &err) {
    return testing::AllOf(Failed(),
                          testing::Property("error", &Result<Value, Err>::error,
                                            testing::Eq(err)));
}

template <class Value>
testing::Matcher<Result<Value, std::string>>
FailedWithMessageContaining(std::string const &err) {
    return testing::AllOf(
        Failed(), testing::Property("error", &Result<Value, std::string>::error,
                                    testing::HasSubstr(err)));
}

TEST(Result, Demo) {
    Result<int, std::string> ok = 0;
    Result<int, std::string> ko = failed<std::string>("Oops");

    ASSERT_TRUE(ok.has_value());
    ASSERT_TRUE(ok);
    ASSERT_FALSE(ok.is_failure());
    ASSERT_EQ(ok.value(), 0);

    ASSERT_FALSE(ko.has_value());
    ASSERT_FALSE(ko);
    ASSERT_TRUE(ko.is_failure());
    ASSERT_EQ(ko.error(), "Oops");
}

struct customWithToString {
    int i;
    long l;
    friend std::ostream &operator<<(std::ostream &os,
                                    customWithToString const &_this) {
        return os << "{ i = " << _this.i << ", l = " << _this.l << " }";
    }
};

struct customWithoutToString {
    int i;
    long l;
};

TEST(Result, Matchers) {
    Result<int, std::string> ok = 0;
    Result<int, std::string> ko = failed<std::string>("Oops");

    EXPECT_THAT(ok, Succeeded());
    EXPECT_THAT(ok, SucceededWith(0));
    EXPECT_THAT(ko, Failed());
    EXPECT_THAT(ko, (FailedWith<int, std::string>("Oops")));
    EXPECT_THAT(ko, (FailedWithMessageContaining<int>("Oo")));

    EXPECT_FATAL_FAILURE(({
                             Result<int, std::string> ko =
                                 failed<std::string>("Oops");
                             ASSERT_THAT(ko, Succeeded());
                         }),
                         "failed with <\"Oops\">");
    EXPECT_FATAL_FAILURE(({
                             Result<int, std::string> ok = 1;
                             ASSERT_THAT(ok, Failed());
                         }),
                         "succeeded with <1>");

    EXPECT_FATAL_FAILURE(
        ({
            Result<int, customWithoutToString> noToStringRequired =
                failed<customWithoutToString>({1, 2});
            ASSERT_THAT(noToStringRequired, Succeeded());
        }),
        "failed with <8-byte object");

    EXPECT_FATAL_FAILURE(
        ({
            Result<int, customWithToString> toStringOverloaded =
                failed<customWithToString>({1, 2});
            ASSERT_THAT(toStringOverloaded, Succeeded());
        }),
        "failed with <{ i = 1, l = 2 }>");

    EXPECT_FATAL_FAILURE(({
                             Result<int, std::string> ko =
                                 failed<std::string>("Oops");
                             ASSERT_THAT(ko, SucceededWith(0));
                         }),
                         "failed with <\"Oops\">");
    EXPECT_FATAL_FAILURE(({
                             Result<int, std::string> ok = 1;
                             ASSERT_THAT(ok, SucceededWith(0));
                         }),
                         "property `value` is 1");

    EXPECT_FATAL_FAILURE(
        ({
            Result<int, std::string> ko = failed<std::string>("Wrong message");
            ASSERT_THAT(ko, (FailedWith<int, std::string>("Oops")));
        }),
        "property `error` is \"Wrong message\"");
}

TEST(Result, Chaining) {
    Result<int, std::string> ok = 3;
    Result<int, std::string> ko = failed<std::string>("Oops");
    auto twice = [](const int &i) { return 2 * i; };
    auto intToString = [](const int &i) { return std::to_string(i); };

    Result<int, std::string> okTwice = ok.andThen<int>(twice);
    auto okTwiceStr = okTwice.andThen<std::string>(intToString);
    auto mapByMethod = okTwiceStr.andThen<size_t>(&std::size<std::string>);
    Result<int, std::string> koTwice = ko.andThen<int>(twice);

    ASSERT_THAT(okTwice, SucceededWith(6));
    ASSERT_THAT(okTwiceStr, SucceededWith<std::string>("6"));
    ASSERT_THAT(mapByMethod, SucceededWith<size_t>(1));
    ASSERT_THAT(koTwice, (FailedWith<int, std::string>("Oops")));
}