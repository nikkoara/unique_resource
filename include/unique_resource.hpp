// -*- mode: c++; -*-
// Original code (http://tinyurl.com/hpzgalw) is copyright Eric Niebler, (c) 2016

#ifndef STD_UNIQUE_RESOURCE_HPP
#define STD_UNIQUE_RESOURCE_HPP

#include <memory>
#include <utility>

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

////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////

template< typename ResourceT, typename DeleterT >
struct unique_resource {
private:
    using resource_type = ResourceT;
    using  deleter_type = DeleterT;

    static_assert (
        detail::is_nothrow_move_constructible_v< resource_type > ||
        detail::is_copy_constructible_v< resource_type >,
        "resource must be nothrow_move_constructible or copy_constructible");

    static_assert (
        detail::is_nothrow_move_constructible_v< deleter_type > ||
        detail::is_copy_constructible_v< deleter_type >,
        "deleter must be nothrow_move_constructible or copy_constructible");

private:
    // More helpers
    template< typename T >
    static constexpr auto is_boxable_resource_v = std::is_constructible<
        detail::box< resource_type >, T, detail::scope_ignore >::value;

    template< typename T >
    static constexpr auto is_boxable_deleter_v = std::is_constructible<
        detail::box< deleter_type >, T, detail::scope_ignore >::value;

    template< typename T, typename U >
    using enable_constructor_t = std::enable_if_t<
        is_boxable_resource_v< T > && is_boxable_deleter_v< U > >;

private:
    detail::box< resource_type > resource_;
    detail::box< deleter_type >  deleter_;

    bool execute_on_destruction_ = true;

private:
    // Non-copyable
    unique_resource (const unique_resource&) = delete;
    unique_resource& operator= (const unique_resource&) = delete;

public:
    template< typename T, typename U, typename = enable_constructor_t< T, U > >
    explicit unique_resource (T&& t, U&& u)
        noexcept (
            noexcept (detail::box< resource_type > (
                          (resource_type&&)t, detail::scope_ignore { })) &&
            noexcept (detail::box<  deleter_type > (
                          (deleter_type&&)u, detail::scope_ignore { })))
        : resource_ (std::move (t), make_scope_exit ([&] { u (t); })),
          deleter_  (std::move (u), make_scope_exit ([&, this] { u (get ()); }))
        { }

    unique_resource (unique_resource&& other)
        noexcept (
            noexcept (detail::box< resource_type > (
                          other.resource_.move (), detail::scope_ignore { })) &&
            noexcept (detail::box< deleter_type > (
                          other.deleter_.move (), detail::scope_ignore { })))
        : resource_ (other.resource_.move (), detail::scope_ignore { }),
                             deleter_  (other.deleter_.move (), make_scope_exit (
                         [&, this] () noexcept {
                             other.get_deleter ()(get ());
                             other.release ();
                         })),
          execute_on_destruction_ (
              std::exchange (other.execute_on_destruction_, false))
        { }

    unique_resource&
    operator= (unique_resource&& other)
        noexcept (std::is_nothrow_move_assignable< resource_type >::value &&
                  std::is_nothrow_move_assignable<  deleter_type >::value) {
        if (this == &other)
            return *this;

        reset ();

        if (std::is_nothrow_move_assignable< detail::box< resource_type > >::value) {
            // Failure to move the deleter leaves the cleanup to other
            deleter_  = detail::maybe_move_assign (other.deleter_);
            resource_ = detail::maybe_move_assign (other.resource_);
        }
        else if (std::is_nothrow_move_assignable< detail::box< deleter_type > >::value) {
            // Failure to move the resource leaves the cleanup to other
            resource_ = detail::maybe_move_assign (other.resource_);
            deleter_  = detail::maybe_move_assign (other.deleter_);
        }
        else {
            resource_ = detail::as_const (other.resource_);

            try {
                // Try to avoid a resource leak
                deleter_  = detail::as_const (other.deleter_);
            }
            catch (...) {
                // Release the resource with the other deleter
                other.get_deleter ()(get ());

                // Deactivate deleters
                execute_on_destruction_ = other.execute_on_destruction_ = false;

                throw;
            }
        }

        execute_on_destruction_ = std::exchange (
            other.execute_on_destruction_, false);

        return *this;
    }

    ~unique_resource () noexcept {
        reset ();
    }

    void
    reset () noexcept {
        if (execute_on_destruction_) {
            execute_on_destruction_ = false;
            get_deleter ()(get ());
        }
    }

    void
    reset (resource_type&& r)
        noexcept (noexcept (detail::box< resource_type > (std::move (r)))) {
        reset ();
        resource_ = std::move (r);
        execute_on_destruction_ = true;
    }

    const resource_type&
    release () noexcept {
        execute_on_destruction_ = false;
        return get ();
    }

    resource_type&
    get () noexcept {
        return resource_.get ();
    }

    const resource_type&
    get () const noexcept {
        return resource_.get ();
    }

    operator const resource_type& () const noexcept {
        return get ();
    }

    resource_type operator-> () const noexcept {
        return get ();
    }

    typename std::add_lvalue_reference <
        typename std::remove_pointer< resource_type >::type >::type
    operator* () const {
        return *resource_;
    }

    const deleter_type&
    get_deleter () const noexcept {
        return deleter_.get ();
    }
};

template< typename T, typename U >
inline auto
make_unique_resource (T&& t, U&& u)
    noexcept (noexcept ( unique_resource< T, U > (
                             std::forward< T > (t), std::forward< U > (u)))) {
    return unique_resource< T, U > (
        std::forward< T > (t), std::forward< U > (u));
}

template< typename T, typename U >
inline auto
make_unique_resource (T& t, U&& u)
    noexcept (noexcept ( unique_resource< T, U > (t, std::forward< U > (u)))) {
    return unique_resource< T, U > (t, std::forward< U > (u));
}

}}

#endif // STD_UNIQUE_RESOURCE_HPP
