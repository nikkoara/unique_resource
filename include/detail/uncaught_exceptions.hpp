// -*- mode: c++; -*-

#ifndef STD_UNCAUGHT_EXCEPTIONS_HPP
#define STD_UNCAUGHT_EXCEPTIONS_HPP

namespace std {
namespace detail {

#if defined (__APPLE__) && defined (__clang__)

extern "C" int __cxa_uncaught_exceptions ();

inline unsigned
uncaught_exception_count () {
    return __cxa_uncaught_exceptions ();
}

#else
#  error unsupported platform
#endif // __GNUG__ || __CLANG__

} // namespace detail

inline int
uncaught_exceptions () noexcept {
    return detail::uncaught_exception_count ();
}

} // namespace std

#endif // STD_UNCAUGHT_EXCEPTIONS_HPP
