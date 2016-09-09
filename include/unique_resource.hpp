// -*- mode: c++; -*-
// Original code (http://tinyurl.com/hpzgalw) is copyright Eric Niebler, (c) 2016

#ifndef STD_UNIQUE_RESOURCE_HPP
#define STD_UNIQUE_RESOURCE_HPP

#include <memory>
#include <utility>

#include <detail/box.hpp>
#include <detail/guard.hpp>
#include <detail/uncaught_exceptions.hpp>

namespace std {
namespace experimental {

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
