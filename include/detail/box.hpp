// -*- mode: c++; -*-
// Original code (http://tinyurl.com/hpzgalw) is copyright Eric Niebler, (c) 2016

#ifndef STD_DETAIL_BOX_HPP
#define STD_DETAIL_BOX_HPP

#include <utility>

namespace std {
namespace experimental {
namespace detail {

template< typename T >
constexpr auto should_move_assign_v =
    std::is_nothrow_move_assignable< T >::value || !std::is_copy_assignable< T >::value;

template< typename T >
using maybe_move_assign_result = std::conditional<
    should_move_assign_v< T >, T&&, const T& >;

template< typename T >
constexpr typename maybe_move_assign_result< T >::type
maybe_move_assign (T& x) noexcept {
    return std::move (x);
}

template< typename T >
const T&
as_const (T& x) noexcept {
    return x;
}

////////////////////////////////////////////////////////////////////////

struct scope_ignore;

template< typename T >
struct box {
    using value_type = T;

public:
    box (T const& t) noexcept (noexcept (T (t)))
        : value (t)
        { }

    box (T&& t) noexcept (noexcept (T (std::move_if_noexcept (t))))
        : value (std::move_if_noexcept (t))
        { }

private:
    template< typename U >
    using enable_constructor = std::enable_if_t<
        std::is_constructible< T, U >::value >;

public:
    template< typename U, typename G = scope_ignore,
              typename = enable_constructor< U > >
    explicit box (U&& u, G&& guard = G ())
        noexcept (noexcept (value_type (std::forward< U > (u))))
        : value (std::forward< U > (u)) {
        guard.release ();
    }

    T& get () noexcept {
        return value;
    }

    const T& get () const noexcept {
        return value;
    }

    T&& move () noexcept {
        return std::move (value);
    }

    void reset (const T& t) noexcept (noexcept (value = t)) {
        value = t;
    }

    void reset (T&& t)
        noexcept (noexcept (value = maybe_move_assign (t))) {
        value = maybe_move_assign (t);
    }

private:
    value_type value;
};

template< typename T >
struct box< T& > {
    using value_type = std::reference_wrapper< T >;

private:
    template< typename U >
    using enable_constructor = std::enable_if_t<
        std::is_convertible< U, T& >::value >;

public:
    template< typename U, typename G = scope_ignore,
              typename = enable_constructor< U > >
    box (U&& u, G&& guard = G ())
        noexcept (noexcept (static_cast< T& > (u)))
        : value (static_cast< T& > (u)) {
        guard.release ();
    }

    T& get () const noexcept {
        return value.get ();
    }

    T& move () const noexcept {
        return get ();
    }

    void reset (T& t) noexcept {
        value = std::ref (t);
    }

private:
    value_type value;
};

}}}

#endif // STD_DETAIL_BOX_HPP
