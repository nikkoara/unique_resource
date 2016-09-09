// -*- mode: c++; -*-
// Original code (http://tinyurl.com/hpzgalw) is copyright Eric Niebler, (c) 2016

#ifndef STD_DETAIL_SCOPE_HPP
#define STD_DETAIL_SCOPE_HPP

#include <utility>
#include <detail/uncaught_exceptions.hpp>

namespace std {
namespace experimental {
namespace detail {

struct scope_ignore {
    void release () noexcept { }
};

struct scope_exit_policy {
    bool value = true;

    void release () noexcept {
        value = false;
    }

    bool should_execute () noexcept {
        return value;
    }
};

struct scope_fail_policy {
    int value = std::uncaught_exceptions ();

    void release () noexcept {
        value = std::numeric_limits< int >::max ();
    }

    bool should_execute () noexcept {
        return value < std::uncaught_exceptions ();
    }
};

struct scope_success_policy {
    int value = std::uncaught_exceptions ();

    void release () noexcept {
        value = -1;
    }

    bool should_execute () noexcept {
        return value >= std::uncaught_exceptions ();
    }
};

////////////////////////////////////////////////////////////////////////

template< typename, typename = scope_exit_policy >
struct scope_guard;

template< typename T >
using scope_exit = scope_guard< T, scope_exit_policy >;

template< typename T >
using scope_fail = scope_guard< T, scope_fail_policy >;

template< typename T >
using scope_success = scope_guard< T, scope_success_policy >;

template< bool value >
using boolean_constant = std::integral_constant< bool, value >;

template< typename FunctionT, typename PolicyT >
struct scope_guard : PolicyT {
    using function_type = FunctionT;
    using policy_type = PolicyT;

private:
    // Helpers
    template< typename T >
    static auto make_guard (std::true_type, T) {
        return scope_ignore { };
    }

    template< typename F >
    static auto make_guard (std::false_type, F* pf) {
        return scope_guard< F&, policy_type > (*pf);
    }

private:
    // Compaction
    template< typename FunctionPtr >
    using is_constructible_from = std::is_constructible<
        box< function_type >, FunctionPtr, scope_ignore >;

    template< typename FunctionPtr >
    static constexpr auto is_constructible_from_v =
        is_constructible_from< FunctionPtr >::value;

    template< typename FunctionPtr >
    using is_nothrow_constructible_from = boolean_constant<
        noexcept (box< function_type > (
                      std::declval< FunctionPtr >(), policy_type { })) >;

    template< typename FunctionPtr >
    static constexpr auto is_nothrow_constructible_from_v =
        is_nothrow_constructible_from< FunctionPtr >::value;

public:
    template< typename FunctionPtr,
              typename = std::enable_if_t< is_constructible_from_v< FunctionPtr > > >
    explicit scope_guard (FunctionPtr&& p)
        noexcept (is_nothrow_constructible_from_v< FunctionPtr >)
        : function_ (
            (FunctionPtr&&)p,
            scope_guard::make_guard (
                is_nothrow_constructible_from< FunctionPtr > { }, &p))
        { }

    scope_guard (scope_guard&& other)
        noexcept(noexcept (box< function_type > (other.function_.move (), other)))
        : policy_type (other), function_ (other.function_.move (), other)
        { }

    scope_guard& operator= (scope_guard &&) = delete;

    ~scope_guard () noexcept (noexcept (function_.get ()())) {
        if (policy_type::should_execute ())
            function_.get ()();
    }

    scope_guard (const scope_guard&) = delete;
    scope_guard& operator=(const scope_guard&) = delete;

private:
    box< function_type > function_;
};

template<typename FunctionT, typename PolicyT>
void swap (scope_guard< FunctionT, PolicyT >&,
           scope_guard< FunctionT, PolicyT >&) = delete;

////////////////////////////////////////////////////////////////////////

// More helpers
template< typename T >
constexpr auto is_reference_v = std::is_reference< T >::value;

template< typename T >
constexpr auto is_nothrow_move_constructible_v =
    std::is_nothrow_move_constructible< T >::value;

template< typename T >
constexpr auto is_copy_constructible_v = std::is_copy_constructible< T >::value;

template< typename T, typename U >
constexpr auto is_nothrow_constructible_v =
    std::is_nothrow_constructible< T, U >::value;

template< typename F, typename P >
inline auto make_scope_guard (F&& f) {
    return scope_guard< std::decay_t< F >, P > (std::forward< F > (f));
}

} // namespace detail

////////////////////////////////////////////////////////////////////////

template< typename FunctionT >
auto make_scope_exit (FunctionT&& f)
    noexcept (detail::is_nothrow_constructible_v< std::decay_t< FunctionT >, FunctionT >) {
    return detail::make_scope_guard<
        FunctionT, detail::scope_exit_policy > (std::forward< FunctionT > (f));
}

template< typename FunctionT >
auto make_scope_fail (FunctionT&& f)
    noexcept (detail::is_nothrow_constructible_v< std::decay_t< FunctionT >, FunctionT >) {
    return detail::make_scope_guard<
        FunctionT, detail::scope_fail_policy > (std::forward< FunctionT > (f));
}

template< typename FunctionT >
auto make_scope_success (FunctionT&& f)
    noexcept (detail::is_nothrow_constructible_v< std::decay_t< FunctionT >, FunctionT >) {
    return detail::make_scope_guard<
        FunctionT, detail::scope_success_policy > (std::forward< FunctionT > (f));
}

}}

#endif // STD_DETAIL_SCOPE_HPP
