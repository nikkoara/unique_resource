// -*- mode: c++; -*-

#include <functional>
#include <iostream>
#include <utility>

#include <boost/type_index.hpp>
namespace ti = boost::typeindex;

#define TI(x) ti::type_id_with_cvr< x > ().pretty_name ()
#define TV(x) ti::type_id_with_cvr< decltype (x) > ().pretty_name ()

static int a, b, c, d;

namespace X {

template< typename T >
struct box {
    explicit box (const T& t) noexcept (noexcept (T (t)))
        : value (t) {
        std::cout << " --> box< " << TI (T)
                  << " >::box (" << TI (decltype (t))
                  << ")\n";
        ++a;
    }

    explicit box (T&& t) noexcept (noexcept (T (std::move_if_noexcept (t))))
        : value (std::move_if_noexcept (t)) {
        std::cout << " --> box< " << TI (T) << " >::box ("
                  << TI (decltype (t)) << ")\n";
        ++b;
    }

private:
    template< typename U >
    using enable_construction_from = std::enable_if_t<
        std::is_constructible< T, U >::value >;

public:
    template< typename U, typename = enable_construction_from< U > >
    explicit box (U&& u) noexcept (noexcept (T (std::forward< U > (u))))
        : value (std::forward< U > (u)) {
        std::cout << " --> box< " << TI (T) << " >::box (" << TI (decltype (u))
                  << ")\n";
        ++c;
    }

    T value;
};

template< typename T >
struct box< T& > {
private:
    template< typename U >
    using enable_construction_from = std::enable_if_t<
        std::is_convertible< U, T& >::value >;

public:
    template< typename U, typename = enable_construction_from< U > >
    box (U&& u) noexcept (noexcept (static_cast< T& > (u)))
        : value (static_cast< T& > (u)) {
        std::cout << " --> box< " << TI (T&) << " >::box ("
                  << TI (decltype (u)) << ")\n";
        ++d;
    }

    std::reference_wrapper< T > value;
};

template< typename T >
struct gift {
private:
    template< typename U >
    static constexpr auto is_boxable_resource_v =
        std::is_constructible< box< T >, U >::value;

    template< typename U >
    using enable_construction_from = std::enable_if_t<
        is_boxable_resource_v< U > >;

public:
    template< typename U, typename = enable_construction_from< U > >
    explicit gift (U&& u) noexcept (
        noexcept (box< T > (std::forward< U > (u))))
        : value (std::forward< U > (u))
        { }

    box< T > value;
};

template< typename T >
inline auto
wrap (T&& t) {
    std::cout << "wrap(1)<" << TI (T) << "> (" << TI (T&&) << ")\n";
    return gift< T > (std::forward< T > (t));
}

template< typename T >
inline auto
wrap (T& t) {
    std::cout << "wrap(2)<" << TI (T) << "> (" << TI (T&) << ")\n";
    return gift< T& > (t);
}

} // namespace X

////////////////////////////////////////////////////////////////////////

template< typename T, typename U >
inline void do_box (U&& u) {
    auto x = X::box< T > (std::forward< U > (u));
    std::cout << "   : " << TV (x) << "\n\n";
}

template< typename T, typename U >
using enable_if_convertible_t = std::enable_if_t<
    std::is_convertible< U&, T& >::value >;

template< typename T, typename U >
using enable_if_not_convertible_t = std::enable_if_t<
    !std::is_convertible< U&, T& >::value >;

template< typename T, typename U = T >
inline enable_if_convertible_t< T, U >
box () {
    do_box< T, U > (U{ });

    {
        U u;
        do_box< T > (u);
        do_box< T& > (u);
    }

    {
        const U u{ };
        do_box< T > (u);
        do_box< const T& > (u);
    }
}

template< typename T, typename U = T >
inline enable_if_not_convertible_t< T, U >
box () {
    do_box< T, U > (U{ });

    {
        U u;
        do_box< T > (u);
    }

    {
        const U u{ };
        do_box< T > (u);
        do_box< const T& > (u);
    }
}

////////////////////////////////////////////////////////////////////////

int f () { return 0; }

struct V { };
struct W : V { };

static void
direct_construction () {
    box< int > ();
    box< int, unsigned > ();
    box< V, W > ();

    {
        auto x = X::box< int(&)() > (f);
        std::cout << "   : " << TV (x) << "\n\n";

        auto y = X::box< int(*)() > (&f);
        std::cout << "   : " << TV (y) << "\n\n";
    }

    {
        auto lambda = [&]{ return 0; };
        auto x = X::box< decltype (lambda) > (std::move (lambda));
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto lambda = [&]{ return 0; };
        auto x = X::box< decltype (lambda)& > (lambda);
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
        int i{ };
        auto x = wrap (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const int i{ };
        auto x = wrap (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        auto x = wrap< unsigned > (int{ });
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        int i{ };
        auto x = wrap< unsigned > (i);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        W w;
        auto x = wrap< V > (w);
        std::cout << "   : " << TV (x) << "\n\n";
    }

    {
        const W w{ };
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
        auto x = wrap ([&]{ return 0; });
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
