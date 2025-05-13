#include "lib.hpp"
#include <algorithm>
#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
namespace nyub { // operator overload must live in the same namespace as the
                 // printed value
std::ostream &operator<<(std::ostream &os, const LogLevel &logLevel) {
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

std::ostream &operator<<(std::ostream &os, const nyub::LogMessage &msg) {
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
    explicit failed(const Err &error) : m_error(error) {}
    [[nodiscard]] Err error() const { return m_error; }

  private:
    Err m_error;
};

template <typename Value, typename Err>
class Result : std::variant<Value, failed<Err>> {
  public:
    Result(const Value &a) : std::variant<Value, failed<Err>>(a) {}
    Result(const failed<Err> &ko) : std::variant<Value, failed<Err>>(ko) {}
    [[nodiscard]] bool has_value() const {
        return std::holds_alternative<Value>(*this);
    }
    [[nodiscard]] bool is_failure() const {
        return std::holds_alternative<failed<Err>>(*this);
    }
    explicit operator bool() const { return has_value(); }

    [[nodiscard]] Value value() const { return std::get<Value>(*this); }
    [[nodiscard]] Err error() const {
        return std::get<failed<Err>>(*this).error();
    }

    template <typename Method,
              std::enable_if_t<std::is_invocable_v<Method, const Value &>,
                               bool> = true>
    [[nodiscard]] auto andThen(const Method &f) const
        -> Result<decltype(f(value())), Err> {
        using NewValue = decltype(f(value()));
        if (is_failure()) {
            return Result<NewValue, Err>(failed(error()));
        }
        return Result<NewValue, Err>(f(value()));
    }

    template <
        class Method,
        // Only if Value is a class
        typename Value_ = Value, // To trigger parameter substitution and SFINAE
        std::enable_if_t<std::is_class_v<Value_>, bool> = true,

        std::enable_if_t<std::is_member_function_pointer_v<Method>, bool> =
            true>
    [[nodiscard]] Result<std::invoke_result_t<Method, const Value &>, Err>
    andThen(const Method &f) // Not using -> decltype(...) more readable syntax
                             // because Intellisense does not handle SFINAE for
                             // this construct and emits warnings when
                             // instantiating with non-class Value types
        const {
        using NewValue = decltype((value().*f)());
        if (is_failure()) {
            return Result<NewValue, Err>(failed(error()));
        }
        return Result<NewValue, Err>((value().*f)());
    }
};

MATCHER(Succeeded, "to be successfull") {
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
testing::Matcher<Result<Value, Err>> SucceededWith(const Value &v) {
    return testing::AllOf(
        Succeeded(),
        testing::Property("value", &Result<Value, Err>::value, testing::Eq(v)));
}

template <class Value, class Err = std::string>
testing::Matcher<Result<Value, Err>> FailedWith(const Err &err) {
    return testing::AllOf(Failed(),
                          testing::Property("error", &Result<Value, Err>::error,
                                            testing::Eq(err)));
}

template <class Value>
testing::Matcher<Result<Value, std::string>>
FailedWithMessageContaining(const std::string &err) {
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
                                    const customWithToString &_this) {
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
    static_assert(std::is_invocable_v<decltype(twice), int>);

    Result<int, std::string> okTwice = ok.andThen(twice);
    Result<std::string, std::string> okTwiceStr =
        okTwice.andThen([](const int &i) {
            return std::to_string(i); // since std::to_string is overloaded we
                                      // cannot pass it directly :(
        });
    Result<size_t, std::string> mapByMethod =
        okTwiceStr.andThen(&std::string::size);
    Result<int, std::string> koTwice = ko.andThen(twice);

    ASSERT_THAT(okTwice, SucceededWith(6));
    ASSERT_THAT(okTwiceStr, SucceededWith<std::string>("6"));
    ASSERT_THAT(mapByMethod, SucceededWith<size_t>(1));
    ASSERT_THAT(koTwice, (FailedWith<int, std::string>("Oops")));
}

/**
 * A CRTP to define strongly typed flags
 * @tparam F the resulting flag type
 */
template <class F> class Flag {
  public:
    static F TRUE() {
        return F(true);
    }
    static F FALSE() {
        return F(true);
    }
    explicit Flag(const bool b) : m_flag(b) {}
    explicit operator bool() const { return m_flag; }
    bool operator!() const { return !m_flag; }
    [[nodiscard]] bool isSet() const { return m_flag; }

  private:
    bool m_flag;
};

class OneFlag : public Flag<OneFlag> {
  public:
    explicit OneFlag(const bool b) : Flag(b) {}
};
class AnotherFlag : public Flag<AnotherFlag> {
  public:
    explicit AnotherFlag(const bool b) : Flag(b) {}
};

template <class A, class B> constexpr void assert_incompatible() {
    static_assert(!std::is_assignable_v<A, B>);
    static_assert(!std::is_assignable_v<B, A>);
    static_assert(!std::is_convertible_v<A, B>);
    static_assert(!std::is_convertible_v<B, A>);
}

TEST(Flag, UseAsBoolean) {
    ASSERT_TRUE(OneFlag::TRUE());
    ASSERT_FALSE(OneFlag::FALSE());

    auto a = OneFlag::TRUE();
    if (!a) {
        FAIL();
    }

    a = OneFlag::FALSE();
    if (a) {
        FAIL();
    }
}

TEST(Flag, NoMixinPossible) { assert_incompatible<OneFlag, AnotherFlag>(); }