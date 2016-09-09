// -*- mode: c++ -*-

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE unique_resource

#include <unique_resource.hpp>

#include <boost/format.hpp>
using fmt = boost::format;

#include <boost/type_index.hpp>
#include <boost/test/unit_test.hpp>
namespace utf = boost::unit_test;

#include <iostream>
#include <exception>

namespace X = std::experimental;

BOOST_AUTO_TEST_SUITE(details)

////////////////////////////////////////////////////////////////////////

namespace A {

template< int a, int b >
struct S {
    S& operator= (const S&) noexcept { return *this; }
    S& operator= (S&&) noexcept (b) { return *this; }
};

template< int b >
struct S< 0, b > {
    S& operator= (const S&) = delete;
    S& operator= (S&&) noexcept (b) { return *this; }
};

} // namespace A

BOOST_AUTO_TEST_CASE (conditional_move_assign_test) {
    using namespace A;

    BOOST_TEST (( true == X::detail::should_move_assign_v< S< 0, 0 > >));
    BOOST_TEST (( true == X::detail::should_move_assign_v< S< 0, 1 > >));
    BOOST_TEST ((false == X::detail::should_move_assign_v< S< 1, 0 > >));
    BOOST_TEST (( true == X::detail::should_move_assign_v< S< 1, 1 > >));
}

////////////////////////////////////////////////////////////////////////

namespace AA {

struct S { };

template< typename T, typename U >
inline bool is_boxable () {
    return std::is_constructible< X::detail::box< T >, U&& >::value;
}

template< typename T, typename U >
inline bool is_boxable_ref () {
    return std::is_constructible< X::detail::box< T& >, U&& >::value;
}

} // namespace AA

BOOST_AUTO_TEST_CASE (std_detail_box_test) {
    using namespace AA;

    {
#define T(t, x, y) BOOST_TEST (t == (is_boxable< x, y > ()))

        T (true,       S,          S);
        T (true, const S,          S);

        T (true,       S,          S&);
        T (true, const S,          S&);

        T (true,       S,    const S&);
        T (true, const S,    const S&);

        T (true,       S,          S&&);
        T (true, const S,          S&&);

#undef T
    }

    {
#define T(t, x, y) BOOST_TEST (t == (is_boxable_ref< x, y > ()))

        T (false,       S,          S);
        T ( true, const S,          S);

        T ( true,       S,          S&);
        T ( true, const S,          S&);

        T (false,       S,    const S&);
        T ( true, const S,    const S&);

        T (false,       S,          S&&);
        T ( true, const S,          S&&);

#undef T
    }

    {
        X::detail::box< S > x (S { });
        BOOST_TEST (true == (std::is_same< decltype (x.get ()),  S&  >::value));
        BOOST_TEST (true == (std::is_same< decltype (x.move ()), S&& >::value));
    }

    {
        const X::detail::box< S > x (S { });
        BOOST_TEST (true == (std::is_same< decltype (x.get ()),  const S& >::value));
    }

    {
        S s;
        X::detail::box< S& > x (s);
        BOOST_TEST (true == (std::is_same< decltype (x.get ()), S& >::value));
    }

    {
        S s;
        const X::detail::box< S& > x (s);
        BOOST_TEST (true == (std::is_same< decltype (x.move ()), S& >::value));
    }
}

////////////////////////////////////////////////////////////////////////

namespace B {

struct S {
    int value = 0;

    ~S () {
        assert (value == std::uncaught_exceptions ());
    }
};

template< size_t N >
struct X {
    int value = 0;

    ~X () {
        try {
            X< N - 1 > x { value + 1 };
            S s { value + 1 };
            throw 0;
        }
        catch (...) {
        }
    };
};

template<  >
struct X< 0 > {
    int value = 0;

    ~X () {
        try {
            S s { value + 1 };
            throw 0;
        }
        catch (...) {
        }
    };
};

};

BOOST_AUTO_TEST_CASE (uncaught_exceptions_test) {

    {
        using namespace B;
        X< 1 > x;
        X< 33 > y;
    }
}

////////////////////////////////////////////////////////////////////////

namespace C {

template< typename T = std::function< void() > >
struct S {
    using function_type = T;

    ~S () { f_ (); }

    function_type f_;
};

}

BOOST_AUTO_TEST_CASE (exit_policy_test) {
    using namespace C;

    {
        {
            X::detail::scope_exit_policy p;

            BOOST_TEST (p.value);

            S< > c1 { [&] { BOOST_TEST (true == p.value); } };
            S< > c2 { [&] { BOOST_TEST (true == p.should_execute ()); } };
        }

        {
            X::detail::scope_exit_policy p;

            p.release ();
            BOOST_TEST (false == p.value);

            S< > c1 { [&] { BOOST_TEST (false == p.value); } };
            S< > c2 { [&] { BOOST_TEST (false == p.should_execute ()); } };
        }
    }

    {
        try {
            X::detail::scope_exit_policy p;

            S< > c1 { [&] { BOOST_TEST (true == p.value); } };
            S< > c2 { [&] { BOOST_TEST (true == p.should_execute ()); } };

            throw 0;
        }
        catch (...) {
        }
    }
}

BOOST_AUTO_TEST_CASE (fail_policy_test) {
    using namespace C;

    static constexpr auto maxint = (std::numeric_limits< int >::max) ();

    {
        {
            X::detail::scope_fail_policy p;

            BOOST_TEST (0 == p.value);

            S< > c1 { [&] { BOOST_TEST (0 == p.value); } };
            S< > c2 { [&] { BOOST_TEST (false == p.should_execute ()); } };
        }

        {
            X::detail::scope_fail_policy p;

            p.release ();
            BOOST_TEST (maxint == p.value);

            S< > c1 { [&] { BOOST_TEST (maxint == p.value); } };
            S< > c2 { [&] { BOOST_TEST (false == p.should_execute ()); } };
        }
    }

    {
        try {
            X::detail::scope_fail_policy p;

            S< > c1 { [&] { BOOST_TEST (0 == p.value); } };
            S< > c2 { [&] { BOOST_TEST (true == p.should_execute ()); } };

            throw 0;
        }
        catch (...) {
        }

        try {
            X::detail::scope_fail_policy p;

            p.release ();

            S< > c1 { [&] { BOOST_TEST (maxint == p.value); } };
            S< > c2 { [&] { BOOST_TEST (false == p.should_execute ()); } };

            throw 0;
        }
        catch (...) {
        }
    }
}

BOOST_AUTO_TEST_CASE (success_policy_test) {
    using namespace C;

    {
        {
            X::detail::scope_success_policy p;

            BOOST_TEST (0 == p.value);

            S< > c1 { [&] { BOOST_TEST (0 == p.value); } };
            S< > c2 { [&] { BOOST_TEST (true == p.should_execute ()); } };
        }

        {
            X::detail::scope_success_policy p;

            p.release ();
            BOOST_TEST (-1 == p.value);

            S< > c1 { [&] { BOOST_TEST (-1 == p.value); } };
            S< > c2 { [&] { BOOST_TEST (false == p.should_execute ()); } };
        }
    }

    {
        try {
            X::detail::scope_success_policy p;

            S< > c1 { [&] { BOOST_TEST (0 == p.value); } };
            S< > c2 { [&] { BOOST_TEST (false == p.should_execute ()); } };

            throw 0;
        }
        catch (...) {
        }
    }
}

namespace D {

static int counter /* = 0 */;

static void f () {
    ++counter;
}

} // namespace D

BOOST_AUTO_TEST_CASE (basic_scope_guard_test) {
    using namespace D;

    auto g = [&] { ++counter; };

#define T(type, func, value)                          \
    { counter = 0; X::make_scope_ ## type (func); }   \
    BOOST_TEST (value == counter)

    T (   exit, f, 1);
    T (   exit, g, 1);
    T (success, f, 1);
    T (success, g, 1);
    T (   fail, f, 0);
    T (   fail, g, 0);

#undef T
}

////////////////////////////////////////////////////////////////////////

namespace E {

int global_resource = 0;

struct S {
    S () { maybe_throw (); }

    S (int value) { S::static_value = value; }

    S (S const&) { maybe_throw (); }
    S (S&&)      { maybe_throw (); }

    S& operator= (const S&) { return maybe_throw (), *this; }
    S& operator= (S&&)      { return maybe_throw (), *this; }

    operator int () const {
        return S::static_value;
    }

private:
    void maybe_throw () {
        if (0 == S::static_value)
            throw 0;

        if (0 < S::static_value)
            --S::static_value;
    }

private:
    static int static_value;
};

int S::static_value /* = 0 */;

void acquire_resource () {
    global_resource = 1;
}

void release_resource (S&) {
    BOOST_TEST (1 == global_resource);
    --global_resource;
}

} // namespace E

BOOST_AUTO_TEST_CASE (unique_resource_exception_safety_test) {
    using namespace E;

    for (int i = 0; i < 100; ++i) {
        acquire_resource ();

        try {
            auto t = X::make_unique_resource (S (i), &release_resource);

            for (;;) {
                auto tmp (std::move (t));
                t = std::move (tmp);
            }
        }
        catch (int) {
        }
        catch (...) {
            BOOST_TEST (false, "unexpected exception");
        }

        BOOST_TEST (0 == global_resource);
    }
}

BOOST_AUTO_TEST_CASE (unique_resource_test) {
    using namespace E;

    {
        acquire_resource ();
        auto t = X::make_unique_resource (S (-1), release_resource);
        BOOST_TEST (t.get () == -1);
    }

    BOOST_TEST (0 == global_resource);

    {
        acquire_resource ();
        auto t = X::make_unique_resource (S (-1), &release_resource);
        BOOST_TEST (t.get () == -1);
    }

    BOOST_TEST (0 == global_resource);

    {
        acquire_resource ();

        S s (-1);
        auto t = X::make_unique_resource (s, &release_resource);

        BOOST_TEST (t.get () == -1);
    }

    BOOST_TEST (0 == global_resource);

    {
        acquire_resource ();

        auto t = X::make_unique_resource (S (-1), [&](S&) { --global_resource; });

        BOOST_TEST (t.get () == -1);
    }

    BOOST_TEST (0 == global_resource);

    {
        acquire_resource ();

        S s (-1);
        auto t = X::make_unique_resource (s, [&](S&) { --global_resource; });

        BOOST_TEST (t.get () == -1);
    }

    BOOST_TEST (0 == global_resource);
}

BOOST_AUTO_TEST_SUITE_END()
