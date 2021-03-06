// -*- mode: c++ -*-

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE unique_resource

#include <unique_resource.hh>

#include <boost/hana/cartesian_product.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/replicate.hpp>
#include <boost/hana/tuple.hpp>
namespace hana = boost::hana;

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

namespace _01 {

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

} // namespace _01

BOOST_AUTO_TEST_CASE (conditional_move_assign_test) {
    using namespace _01;

    BOOST_TEST (( true == X::detail::should_move_assign_v< S< 0, 0 > >));
    BOOST_TEST (( true == X::detail::should_move_assign_v< S< 0, 1 > >));
    BOOST_TEST ((false == X::detail::should_move_assign_v< S< 1, 0 > >));
    BOOST_TEST (( true == X::detail::should_move_assign_v< S< 1, 1 > >));
}

////////////////////////////////////////////////////////////////////////

namespace _02 {

struct S { };

template< typename T, typename U >
inline bool is_boxable () {
    return std::is_constructible< X::detail::box< T >, U&& >::value;
}

template< typename T, typename U >
inline bool is_boxable_ref () {
    return std::is_constructible< X::detail::box< T& >, U&& >::value;
}

} // namespace _02

BOOST_AUTO_TEST_CASE (std_detail_box_test) {
    using namespace _02;

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

namespace _03 {

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
    using namespace _03;

    {
        X< 1 > x;
        X< 33 > y;
    }
}

////////////////////////////////////////////////////////////////////////

namespace _04 {

template< typename T = std::function< void() > >
struct S {
    using function_type = T;

    ~S () { f_ (); }

    function_type f_;
};

} // _04

BOOST_AUTO_TEST_CASE (exit_policy_test) {
    using namespace _04;

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
    using namespace _04;

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
    using namespace _04;

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

////////////////////////////////////////////////////////////////////////

namespace _05 {

static int counter /* = 0 */;

static void f () {
    ++counter;
}

} // namespace D

BOOST_AUTO_TEST_CASE (basic_scope_guard_test) {
    using namespace _05;

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

namespace _06 {
void f (int) { }
}

BOOST_AUTO_TEST_CASE (conversion_test) {
    using namespace _06;

    {
        using T = X::unique_resource< int, std::function< void(int) > >;
        T t = X::make_unique_resource (0, f);
    }

    {
        using T = X::unique_resource< int, std::function< void(int) > >;
        T t = X::make_unique_resource_checked (0, 0, f);
    }
}

////////////////////////////////////////////////////////////////////////

namespace _07 {

template< bool B > void throw_helper () { throw 0; }
template< > void throw_helper< true > () { }

int global_resource = 0;

void acquire_resource () {
    global_resource = 1;
}

void release_resource () {
    BOOST_TEST (1 == global_resource);
    --global_resource;
}

template< typename A, typename B >
struct S {
    S () noexcept { }

    S (int value) { S::static_value = value; }

    S (S const&) noexcept (A::value) { maybe_throw< A::value, 1 > (); }
    S (S&&)      noexcept (B::value) { maybe_throw< B::value, 2 > (); }

    S& operator= (const S&) noexcept (A::value) { return maybe_throw< A::value, 3 > (), *this; }
    S& operator= (S&&)      noexcept (B::value) { return maybe_throw< B::value, 4 > (), *this; }

    operator int () const {
        return S::static_value;
    }

    template< typename C, typename D >
    void operator() (const S< C, D >&) const {
        release_resource ();
    }

    static int static_value;

private:
    template< bool Y, int >
    void maybe_throw () noexcept (Y) {
        if (0 == S::static_value) {
            throw_helper< Y > ();
        }

        if (0 < S::static_value)
            --S::static_value;
    }
};

template< typename A, typename B >
int S< A, B >::static_value /* = 0 */;

template< typename ...Ts >
S< Ts... > make_S (int x, const Ts&...) {
    return { x };
}

template< typename A, typename B >
std::ostream&
operator<< (std::ostream& ss, const S< A, B >& s) {
    ss << "< " << std::boolalpha << A::value << ", " << B::value
       << " >";
    return ss;
}

} // namespace _07

template< typename T, typename U >
void do_test (T&& t, U&& u) {
    using namespace _07;

    std::cout
        << " --> types: R" << t << "(" << T::static_value << "), D"
        << u << "(" << U::static_value << ")" << std::endl;

    acquire_resource ();

    try {
        auto x = X::make_unique_resource (
            std::forward< T > (t), std::forward< U > (u));

	for (int i = 0; i < 64; ++i) {
	    auto y (std::move (x));
	    x = std::move (y);
	}
    }
    catch (int) {
    }
    catch (...) {
        BOOST_TEST (false, "unexpected exception");
    }

    BOOST_TEST (0 == global_resource);
}

BOOST_AUTO_TEST_CASE (unique_resource_big_exception_safety_test) {
    using namespace _07;

    constexpr auto xs = hana::cartesian_product (
        hana::replicate< hana::tuple_tag > (
            hana::tuple_c< bool, true, false >, hana::size_c< 2 >));

#if 1

    for (int i = 0; i < 32; ++i) {
	for (int j = 0; j < 32; ++j) {
            hana::for_each (xs, [&](auto x) {
                hana::unpack (x, [&](auto ...args1) {
                    hana::for_each (xs, [&](auto y) {
                        hana::unpack (y, [&](auto ...args2) {
                            do_test (make_S (i, args1...), make_S (j, args2...));
                        });
                    });
                });
            });
        }
    }

#else

#define T(i, a, b, j, c, d)                                 \
    do_test (                                               \
        make_S (i, hana::bool_c< a >, hana::bool_c< b >),   \
        make_S (j, hana::bool_c< c >, hana::bool_c< d >)    \
        )

    T (4, true, false, 2, false, false);

#endif // 0
}

BOOST_AUTO_TEST_SUITE_END()
