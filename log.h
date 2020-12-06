#pragma once

#include <array>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>

namespace array_streambuf {

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

    explicit arraybuf(std::ostream& os = std::cout) : sink_{os} {
        setp(buffer_.begin(), buffer_.end());
    }

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

namespace detail {
namespace log {
namespace osbuf = output_streambuf;

template <class Void>
class logger {
    static_assert(std::is_void_v<Void>);

#if !defined(DISABLE_LOGGING)
    struct debug_prefix {
        static constexpr auto value = "[debug]: ";
    };
    using debug_buf_type = osbuf::prefixed<debug_prefix, osbuf::arraybuf<42>>;

    struct info_prefix {
        static constexpr auto value = "[info]: ";
    };
    using info_buf_type = osbuf::prefixed<info_prefix, osbuf::arraybuf<42>>;

    static debug_buf_type debug_buf;
    static info_buf_type info_buf;
#else
    static osbuf::nullbuf null_buf;
#endif

  public:
    logger() = delete;

    static std::ostream debug;
    static std::ostream info;
};

#if !defined(DISABLE_LOGGING)
template <class Void>
typename logger<Void>::debug_buf_type logger<Void>::debug_buf =
    logger<Void>::debug_buf_type{std::cerr};
template <class Void>
typename logger<Void>::info_buf_type logger<Void>::info_buf =
    logger<Void>::info_buf_type{std::cerr};

template <class Void>
auto logger<Void>::debug = std::ostream{&logger<Void>::debug_buf};
template <class Void>
auto logger<Void>::info = std::ostream{&logger<Void>::info_buf};
#else
template <class Void>
typename osbuf::nullbuf logger<Void>::null_buf = osbuf::nullbuf{};

template <class Void>
auto logger<Void>::debug = std::ostream{&logger<Void>::null_buf};
template <class Void>
auto logger<Void>::info = std::ostream{&logger<Void>::null_buf};
#endif

} // namespace log
} // namespace detail

using log = detail::log::logger<void>;

} // namespace array_streambuf
