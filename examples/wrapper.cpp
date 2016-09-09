// -*- mode: c++; -*-

//
// A detailed look at a wrapper class, by value and by reference:
//

#include <iostream>
#include <utility>

#include <boost/type_index.hpp>
namespace ti = boost::typeindex;

#define TI(x) ti::type_id_with_cvr< x > ().pretty_name ()
#define TV(x) ti::type_id_with_cvr< decltype (x) > ().pretty_name ()

static int a, b, c, d;

namespace X {

//
// Primary template wraps lvalues/rvalues by copying them:
//
template< typename T >
struct wrapper {
    using value_type = T;

    explicit wrapper (const T& t) noexcept (noexcept (T (t)))
        : value (t) {
        std::cout << " --> wrapper< " << TI (T)
                  << " >::wrapper (" << TI (decltype (t))
                  << ")\n";
        ++a;
    }

    explicit wrapper (T&& t) noexcept (noexcept (T (t)))
        : value (std::move_if_noexcept (t)) {
        std::cout << " --> wrapper< " << TI (T)
                  << " >::wrapper (" << TI (decltype (t))
                  << ")\n";
        ++b;
    }

private:
    template< typename U >
    using enable_construction_from = std::enable_if_t<
        std::is_constructible< T, U >::value >;

public:
    //
    // Conditionally, construction from other types, if constructible from them:
    //
    template< typename U, typename = enable_construction_from< U > >
    explicit wrapper (U&& u) noexcept (noexcept (wrapper ((T&&)u)))
        : value (std::forward< U > (u)) {
        std::cout << " --> wrapper< " << TI (T)
                  << " >::wrapper (" << TI (decltype (u))
                  << ")\n";
        ++c;
    }

    value_type value;
};

//
// The specialization wraps lvalues by reference:
//
template< typename T >
struct wrapper< T& > {
    using value_type = std::reference_wrapper< T >;

private:
    template< typename U >
    using enable_construction_from = std::enable_if_t<
        std::is_convertible< U, T& >::value >;

public:
    //
    // Conditionally, wraps universal references from other types, if
    // convertible to T&:
    //
    template< typename U, typename = enable_construction_from< U > >
    wrapper (U&& u) noexcept (noexcept (static_cast< T& > (u)))
        : value (static_cast< T& > (u)) {
        std::cout << " --> wrapper< " << TI (T&)
                  << " >::wrapper (" << TI (decltype (u))
                  << ")\n";
        ++d;
    }

    value_type value;
};

template< typename T >
struct gift {
    using value_type = wrapper< T >;

private:
    template< typename U >
    static constexpr auto is_wrappable_resource_v =
        std::is_constructible< wrapper< T >, U >::value;

    template< typename U >
    using enable_construction_from = std::enable_if_t<
        is_wrappable_resource_v< U > >;

public:
    template< typename U, typename = enable_construction_from< U > >
    explicit gift (U&& u)
        : value (std::forward< U > (u))
        { }

    value_type value;
};

template< typename T >
inline auto
wrap (T&& t) {
    std::cout << "wrap(1)<" << TI (T) << "> (" << TI (T&&) << ")\n";
    return X::gift< T > (std::forward< T > (t));
}

template< typename T >
inline auto
wrap (T& t) {
    std::cout << "wrap(2)<" << TI (T) << "> (" << TI (T&) << ")\n";
    return X::gift< T& > (t);
}

} // namespace X

int f () { return 0; }

struct V { };
struct W : V { };

static void
direct_construction () {
    using namespace X;

    {
        auto x = wrapper< int > (0);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        int i = 0;
        auto x = wrapper< int > (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const int i = 0;
        auto x = wrapper< int > (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        int i;
        auto x = wrapper< int& > (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const int i = 0;
        auto x = wrapper< const int& > (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }


    {
        auto x = wrapper< unsigned > (int (0));
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        W w;
        auto x = wrapper< V > (w);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const W w { };
        auto x = wrapper< V > (w);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        W w;
        auto x = wrapper< V& > (w);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const W w { };
        auto x = wrapper< const V& > (w);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto x = wrapper< int(&)() > (f);
        std::cout << "   : " << TV (x) << "\n\n";

        auto y = wrapper< int(*)() > (&f);
        std::cout << "   : " << TV (y) << "\n\n";
    }

    {
        auto lambda = [&]() { return 0; };
        auto x = wrapper< decltype (lambda) > (std::move (lambda));
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto lambda = [&]() { return 0; };
        auto x = wrapper< decltype (lambda)& > (lambda);
        std::cout << "   : " << TV (x) << "\n\n";
    }
}

static void
indirect_construction () {
    using namespace X;

    {
        auto x = wrap (0);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        int i = 0;
        auto x = wrap (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const int i = 0;
        auto x = wrap (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto x = wrap< unsigned > (int (0));
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        W w;
        auto x = wrap< V > (w);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const W w { };
        auto x = wrap< const V > (w);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto x = wrap (f);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto x = wrap< int(*)() > (&f);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto x = wrap ([&]() { return 0; });
        std::cout << "   : " << TV (x) << "\n\n";
    }
}

int main () {
    a = b = c = d = 0;
    direct_construction ();

    std::cout << a << ", " << b << ", " << c << ", " << d << "\n\n";

    a = b = c = d = 0;
    indirect_construction ();

    std::cout << a << ", " << b << ", " << c << ", " << d << "\n";

    return 0;
}
