#pragma once

#include <array>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>

namespace output_streambuf {

struct nullbuf : std::streambuf {
    using base_type = std::streambuf;
    using char_type = typename base_type::char_type;
    using int_type = typename base_type::int_type;

  protected:
    auto overflow(int_type ch) -> int_type override { return ch; }
};

template <std::size_t N>
struct arraybuf : std::streambuf {
    using base_type = std::streambuf;
    using char_type = typename base_type::char_type;
    using int_type = typename base_type::int_type;
    static constexpr auto capacity = N;

    arraybuf(std::ostream& os = std::cout) : sink_{os} { setp(buffer_.begin(), buffer_.end()); }

    ~arraybuf() { sync(); }

    auto sink() -> std::ostream& { return sink_; }

  protected:
    auto overflow(int_type ch) -> int_type override {
        commit();
        sputc(std::char_traits<char_type>::to_char_type(ch));
        return ch;
    }

    auto sync() -> int override {
        commit();
        sink_.flush();
        return 0;
    }

  private:
    auto commit() -> void {
        const auto n = pptr() - pbase();
        sink_.write(pbase(), n);
        pbump(-n);
    }

    std::array<base_type::char_type, N> buffer_ = {};
    std::ostream& sink_;
};

struct stringbuf : std::stringbuf {
    using base_type = std::stringbuf;
    using char_type = typename base_type::char_type;
    using int_type = typename base_type::int_type;

    stringbuf(std::ostream& os = std::cout) : sink_{os} {}

    ~stringbuf() { sync(); }

    auto sink() -> std::ostream& { return sink_; }

  protected:
    auto sync() -> int override {
        const auto n = pptr() - pbase();
        sink_.write(pbase(), n);
        sink_.flush();

        str("");
        return 0;
    }

  private:
    std::ostream& sink_;
};

template <class Prefix, class OutputStreambuf>
struct prefixed : OutputStreambuf {
    using base_type = OutputStreambuf;
    using char_type = typename base_type::char_type;
    using int_type = typename base_type::int_type;
    static constexpr auto prefix_value = Prefix::value;

    prefixed(std::ostream& os = std::cout) : base_type{os} {}

    ~prefixed() {
        if (this->pptr() > this->pbase()) {
            sync();
        }
    }

  protected:
    auto overflow(int_type ch) -> int_type override {
        if (send_prefix_) {
            this->sink().write(prefix_value, prefix_length);
            send_prefix_ = false;
        }

        return base_type::overflow(ch);
    }

    auto sync() -> int override {
        if (send_prefix_) {
            this->sink().write(prefix_value, prefix_length);
        }
        send_prefix_ = true;

        return base_type::sync();
    }

  private:
    static constexpr auto length(const char* s) -> std::size_t {
        return (*s == 0) ? 0 : length(s + 1) + 1;
    }

    static constexpr auto prefix_length = length(prefix_value);

    bool send_prefix_ = true;
};

} // namespace output_streambuf

namespace log {
namespace osbuf = output_streambuf;

class logger {
    struct debug_prefix {
        static constexpr auto value = "[debug]: ";
    };
    using debug_buf_type = osbuf::prefixed<debug_prefix, osbuf::arraybuf<42>>;

    struct info_prefix {
        static constexpr auto value = "[info]: ";
    };
    using info_buf_type = osbuf::prefixed<info_prefix, osbuf::arraybuf<42>>;

  public:
    logger() = delete;

    static auto debug() -> std::ostream& {
        static auto buf = debug_buf_type{std::cerr};
        static auto os = std::ostream{&buf};
        return os;
    }
    static auto info() -> std::ostream& {
        static auto buf = info_buf_type{std::cerr};
        static auto os = std::ostream{&buf};
        return os;
    }
};

// Tag types providing a simpler logging interface
namespace detail {

struct debug_t {
    static auto get_impl() noexcept -> std::ostream& { return logger::debug(); }
};

struct info_t {
    static auto get_impl() noexcept -> std::ostream& { return logger::info(); }
};

template <class LogTag, class T>
auto operator<<(LogTag, T&& t) -> std::ostream& {
#if !defined(DISABLE_LOGGING)
    return std::decay_t<LogTag>::get_impl() << std::forward<T>(t);
#else
    (void)t;
    static auto buf = osbuf::nullbuf{};
    static auto os = std::ostream{&buf};
    return os;
#endif
}

} // namespace detail

constexpr detail::debug_t debug{};
constexpr detail::info_t info{};

} // namespace log
