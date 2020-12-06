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
    static constexpr auto prefix_length = std::char_traits<char_type>::length(prefix_value);

    bool send_prefix_ = true;
};

} // namespace output_streambuf

namespace log {
namespace osbuf = output_streambuf;

#if !defined(DISABLE_LOGGING)

namespace detail {
namespace prefix {
struct debug {
    static constexpr auto value = "[debug]: ";
};
struct info {
    static constexpr auto value = "[info]: ";
};
} // namespace prefix

template <class Prefix>
using buf_type = osbuf::prefixed<Prefix, osbuf::arraybuf<42>>;

inline auto info_buf = buf_type<prefix::info>{std::cerr};
inline auto debug_buf = buf_type<prefix::debug>{std::cerr};

} // namespace detail

inline auto info = std::ostream{&detail::info_buf};
inline auto debug = std::ostream{&detail::debug_buf};

#else

namespace detail {
inline auto buf = osbuf::nullbuf{};
} // namespace detail

inline auto info = std::ostream{&detail::buf};
inline auto debug = std::ostream{&detail::buf};

#endif

} // namespace log

} // namespace array_streambuf
