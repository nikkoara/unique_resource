// -*- mode: c++; -*-

#include <functional>
#include <iostream>

#include <unique_resource.hh>
namespace X = std::experimental;

void f(int) { }

int main () {
    {
        using T = X::unique_resource< int, std::function< void(int) > >;
        T t = X::make_unique_resource (0, f);
    }

    {
        using T = X::unique_resource< int, std::function< void(int) > >;
        T t = X::make_unique_resource_checked (0, 0, f);
    }

    return 0;
}
