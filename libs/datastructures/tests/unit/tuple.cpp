// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

//  tuple_test_bench.cpp  --------------------------------

// clang-format off
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdouble-promotion"
#elif defined (__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif
// clang-format on

#include <hpx/config.hpp>
#include <hpx/datastructures/tuple.hpp>
#include <hpx/testing.hpp>

// clang-format off
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined (__GNUC__)
#  pragma GCC diagnostic pop
#endif
// clang-format on

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

class A
{
};
class B
{
};
class C
{
};

// classes with different kinds of conversions
class AA
{
};
class BB : public AA
{
};
struct CC
{
    CC() {}
    CC(const BB&) {}
};
struct DD
{
    operator CC() const
    {
        return CC();
    };
};

// something to prevent warnings for unused variables
template <class T>
void dummy(const T&)
{
}

// no public default constructor
class foo
{
public:
    explicit foo(int v)
      : val(v)
    {
    }

    bool operator==(const foo& other) const
    {
        return val == other.val;
    }

private:
    foo() {}
    int val;
};

// another class without a public default constructor
class no_def_constructor
{
    no_def_constructor() {}

public:
    no_def_constructor(std::string) {}
};

// A non-copyable class
class no_copy
{
    no_copy(const no_copy&) {}

public:
    no_copy(){};
};

// ----------------------------------------------------------------------------
// Testing different element types --------------------------------------------
// ----------------------------------------------------------------------------

typedef hpx::util::tuple<int> t1;
typedef hpx::util::tuple<double&, const double&, const double, double*,
    const double*>
    t2;
typedef hpx::util::tuple<A, int (*)(char, int), C> t3;
typedef hpx::util::tuple<std::string, std::pair<A, B>> t4;
typedef hpx::util::tuple<A*, hpx::util::tuple<const A*, const B&, C>, bool,
    void*>
    t5;
typedef hpx::util::tuple<volatile int, const volatile char&, int (&)(float)> t6;
typedef hpx::util::tuple<B (A::*)(C&), A&> t7;

// -----------------------------------------------------------------------
// -tuple construction tests ---------------------------------------------
// -----------------------------------------------------------------------

no_copy y;
hpx::util::tuple<no_copy&> x = hpx::util::tuple<no_copy&>(y);    // ok

char cs[10];
hpx::util::tuple<char (&)[10]> v2(cs);    // ok

void construction_test()
{
    hpx::util::tuple<int> t1;
    HPX_TEST_EQ(hpx::util::get<0>(t1), int());

    hpx::util::tuple<float> t2(5.5f);
    HPX_TEST_RANGE(hpx::util::get<0>(t2), 5.4f, 5.6f);

    hpx::util::tuple<foo> t3(foo(12));
    HPX_TEST(hpx::util::get<0>(t3) == foo(12));

    hpx::util::tuple<double> t4(t2);
    HPX_TEST_RANGE(hpx::util::get<0>(t4), 5.4f, 5.6f);

    hpx::util::tuple<int, float> t5;
    HPX_TEST_EQ(hpx::util::get<0>(t5), int());
    HPX_TEST_EQ(hpx::util::get<1>(t5), float());

    hpx::util::tuple<int, float> t6(12, 5.5f);
    HPX_TEST_EQ(hpx::util::get<0>(t6), 12);
    HPX_TEST_RANGE(hpx::util::get<1>(t6), 5.4f, 5.6f);

    hpx::util::tuple<int, float> t7(t6);
    HPX_TEST_EQ(hpx::util::get<0>(t7), 12);
    HPX_TEST_RANGE(hpx::util::get<1>(t7), 5.4f, 5.6f);

    hpx::util::tuple<long, double> t8(t6);
    HPX_TEST_EQ(hpx::util::get<0>(t8), 12);
    HPX_TEST_RANGE(hpx::util::get<1>(t8), 5.4f, 5.6f);

    dummy(hpx::util::tuple<no_def_constructor, no_def_constructor,
        no_def_constructor>(std::string("Jaba"),    // ok, since the default
        std::string("Daba"),                        // constructor is not used
        std::string("Doo")));

    // testing default values
    dummy(hpx::util::tuple<int, double>());
    dummy(hpx::util::tuple<int, double>(1, 3.14));

    //dummy(hpx::util::tuple<double&>()); // should fail, not defaults for references
    //dummy(hpx::util::tuple<const double&>()); // likewise

    double dd = 5;
    dummy(hpx::util::tuple<double&>(dd));    // ok

    dummy(hpx::util::tuple<const double&>(dd + 3.14));    // ok, but dangerous

    //dummy(hpx::util::tuple<double&>(dd+3.14)); // should fail,
    // temporary to non-const reference
}

// ----------------------------------------------------------------------------
// - testing element access ---------------------------------------------------
// ----------------------------------------------------------------------------

void element_access_test()
{
    double d = 2.7;
    A a;
    hpx::util::tuple<int, double&, const A&, int> t(1, d, a, 2);
    const hpx::util::tuple<int, double&, const A, int> ct = t;

    int i = hpx::util::get<0>(t);
    int i2 = hpx::util::get<3>(t);

    HPX_TEST(i == 1 && i2 == 2);

    int j = hpx::util::get<0>(ct);
    HPX_TEST_EQ(j, 1);

    HPX_TEST(hpx::util::get<0>(t) = 5);

    //hpx::util::get<0>(ct) = 5; // can't assign to const

    double e = hpx::util::get<1>(t);
    HPX_TEST_RANGE(e, 2.69, 2.71);

    hpx::util::get<1>(t) = 3.14 + i;
    HPX_TEST_RANGE(hpx::util::get<1>(t), 4.13, 4.15);

    //hpx::util::get<2>(t) = A(); // can't assign to const
    //dummy(hpx::util::get<4>(ct)); // illegal index

    ++hpx::util::get<0>(t);
    HPX_TEST_EQ(hpx::util::get<0>(t), 6);

    HPX_TEST((std::is_const<hpx::util::tuple_element<0,
                  hpx::util::tuple<int, float>>::type>::value != true));
    HPX_TEST((std::is_const<hpx::util::tuple_element<0,
            const hpx::util::tuple<int, float>>::type>::value));

    HPX_TEST((std::is_const<hpx::util::tuple_element<1,
                  hpx::util::tuple<int, float>>::type>::value != true));
    HPX_TEST((std::is_const<hpx::util::tuple_element<1,
            const hpx::util::tuple<int, float>>::type>::value));

    HPX_TEST(
        (std::is_same<hpx::util::tuple_element<1, std::array<float, 4>>::type,
            float>::value));

    dummy(i);
    dummy(i2);
    dummy(j);
    dummy(e);    // avoid warns for unused variables
}

// ----------------------------------------------------------------------------
// - copying tuples -----------------------------------------------------------
// ----------------------------------------------------------------------------

void copy_test()
{
    hpx::util::tuple<int, char> t1(4, 'a');
    hpx::util::tuple<int, char> t2(5, 'b');
    t2 = t1;
    HPX_TEST_EQ(hpx::util::get<0>(t1), hpx::util::get<0>(t2));
    HPX_TEST_EQ(hpx::util::get<1>(t1), hpx::util::get<1>(t2));

    hpx::util::tuple<long, std::string> t3(2, "a");
    t3 = t1;
    HPX_TEST_EQ((double) hpx::util::get<0>(t1), hpx::util::get<0>(t3));
    HPX_TEST_EQ(hpx::util::get<1>(t1), hpx::util::get<1>(t3)[0]);

    // testing copy and assignment with implicit conversions between elements
    // testing tie

    hpx::util::tuple<char, BB*, BB, DD> t;
    hpx::util::tuple<int, AA*, CC, CC> a(t);
    a = t;

    int i;
    char c;
    double d;
    hpx::util::tie(i, c, d) = hpx::util::make_tuple(1, 'a', 5.5);

    HPX_TEST_EQ(i, 1);
    HPX_TEST_EQ(c, 'a');
    HPX_TEST_RANGE(d, 5.4, 5.6);
}

void mutate_test()
{
    hpx::util::tuple<int, float, bool, foo> t1(5, 12.2f, true, foo(4));
    hpx::util::get<0>(t1) = 6;
    hpx::util::get<1>(t1) = 2.2f;
    hpx::util::get<2>(t1) = false;
    hpx::util::get<3>(t1) = foo(5);

    HPX_TEST_EQ(hpx::util::get<0>(t1), 6);
    HPX_TEST_RANGE(hpx::util::get<1>(t1), 2.1f, 2.3f);
    HPX_TEST_EQ(hpx::util::get<2>(t1), false);
    HPX_TEST(hpx::util::get<3>(t1) == foo(5));
}

// ----------------------------------------------------------------------------
// make_tuple tests -----------------------------------------------------------
// ----------------------------------------------------------------------------

void make_tuple_test()
{
    hpx::util::tuple<int, char> t1 = hpx::util::make_tuple(5, 'a');
    HPX_TEST_EQ(hpx::util::get<0>(t1), 5);
    HPX_TEST_EQ(hpx::util::get<1>(t1), 'a');

    hpx::util::tuple<int, std::string> t2;
    t2 = hpx::util::make_tuple((short int) 2, std::string("Hi"));
    HPX_TEST_EQ(hpx::util::get<0>(t2), 2);
    HPX_TEST_EQ(hpx::util::get<1>(t2), "Hi");

    A a = A();
    B b;
    const A ca = a;
    hpx::util::make_tuple(std::cref(a), b);
    hpx::util::make_tuple(std::ref(a), b);
    hpx::util::make_tuple(std::ref(a), std::cref(b));

    hpx::util::make_tuple(std::ref(ca));

    // the result of make_tuple is assignable:
    HPX_TEST(hpx::util::make_tuple(2, 4, 6) ==
        (hpx::util::make_tuple(1, 2, 3) = hpx::util::make_tuple(2, 4, 6)));

    hpx::util::make_tuple("Donald", "Daisy");    // should work;

    // You can store a reference to a function in a tuple
    hpx::util::tuple<void (&)()> adf(make_tuple_test);

    dummy(adf);    // avoid warning for unused variable

    // But make_tuple doesn't work (in C++03)
    // with function references, since it creates a const qualified function type

    hpx::util::make_tuple(make_tuple_test);

    // With function pointers, make_tuple works just fine

    hpx::util::make_tuple(&make_tuple_test);

    // wrapping it the function reference with ref

    // hpx::util::make_tuple(ref(foo3));
}

void tie_test()
{
    int a;
    char b;
    foo c(5);

    hpx::util::tie(a, b, c) = hpx::util::make_tuple(2, 'a', foo(3));
    HPX_TEST_EQ(a, 2);
    HPX_TEST_EQ(b, 'a');
    HPX_TEST(c == foo(3));

    hpx::util::tie(a, hpx::util::ignore, c) =
        hpx::util::make_tuple((short int) 5, false, foo(5));
    HPX_TEST_EQ(a, 5);
    HPX_TEST_EQ(b, 'a');
    HPX_TEST(c == foo(5));

    // testing assignment from std::pair
    int i, j;
    hpx::util::tie(i, j) = std::make_pair(1, 2);
    HPX_TEST(i == 1 && j == 2);

    hpx::util::tuple<int, int, float> ta;
    //ta = std::make_pair(1, 2); // should fail, tuple is of length 3, not 2

    dummy(ta);
}

// ----------------------------------------------------------------------------
// - testing cat -----------------------------------------------------------
// ----------------------------------------------------------------------------
void tuple_cat_test()
{
    hpx::util::tuple<int, float> two = hpx::util::make_tuple(1, 2.f);

    // Cat two tuples
    {
        hpx::util::tuple<int, float, int, float> res =
            hpx::util::tuple_cat(two, two);

        auto expected = hpx::util::make_tuple(1, 2.f, 1, 2.f);

        HPX_TEST(res == expected);
    }

    // Cat multiple tuples
    {
        hpx::util::tuple<int, float, int, float, int, float> res =
            hpx::util::tuple_cat(two, two, two);

        auto expected = hpx::util::make_tuple(1, 2.f, 1, 2.f, 1, 2.f);

        HPX_TEST(res == expected);
    }

    // Cat move only types
    {
        auto t0 = hpx::util::make_tuple(std::unique_ptr<int>(new int(0)));
        auto t1 = hpx::util::make_tuple(std::unique_ptr<int>(new int(1)));
        auto t2 = hpx::util::make_tuple(std::unique_ptr<int>(new int(2)));

        hpx::util::tuple<std::unique_ptr<int>, std::unique_ptr<int>,
            std::unique_ptr<int>>
            result = hpx::util::tuple_cat(
                std::move(t0), std::move(t1), std::move(t2));

        HPX_TEST_EQ((*hpx::util::get<0>(result)), 0);
        HPX_TEST_EQ((*hpx::util::get<1>(result)), 1);
        HPX_TEST_EQ((*hpx::util::get<2>(result)), 2);
    }

    // Don't move references unconditionally (copyable types)
    {
        int i1 = 11;
        int i2 = 22;

        hpx::util::tuple<int&> f1 = hpx::util::forward_as_tuple(i1);
        hpx::util::tuple<int&&> f2 = hpx::util::forward_as_tuple(std::move(i2));

        hpx::util::tuple<int&, int&&> result =
            hpx::util::tuple_cat(std::move(f1), std::move(f2));

        HPX_TEST_EQ((hpx::util::get<0>(result)), 11);
        HPX_TEST_EQ((hpx::util::get<1>(result)), 22);
    }

    // Don't move references unconditionally (move only types)
    {
        std::unique_ptr<int> i1(new int(11));
        std::unique_ptr<int> i2(new int(22));

        hpx::util::tuple<std::unique_ptr<int>&> f1 =
            hpx::util::forward_as_tuple(i1);
        hpx::util::tuple<std::unique_ptr<int>&&> f2 =
            hpx::util::forward_as_tuple(std::move(i2));

        hpx::util::tuple<std::unique_ptr<int>&, std::unique_ptr<int>&&> result =
            hpx::util::tuple_cat(std::move(f1), std::move(f2));

        HPX_TEST_EQ((*hpx::util::get<0>(result)), 11);
        HPX_TEST_EQ((*hpx::util::get<1>(result)), 22);
    }
}

// ----------------------------------------------------------------------------
// - testing tuple equality   -------------------------------------------------
// ----------------------------------------------------------------------------

void equality_test()
{
    hpx::util::tuple<int, char> t1(5, 'a');
    hpx::util::tuple<int, char> t2(5, 'a');
    HPX_TEST(t1 == t2);

    hpx::util::tuple<int, char> t3(5, 'b');
    hpx::util::tuple<int, char> t4(2, 'a');
    HPX_TEST(t1 != t3);
    HPX_TEST(t1 != t4);
    HPX_TEST(!(t1 != t2));
}

// ----------------------------------------------------------------------------
// - testing tuple comparisons  -----------------------------------------------
// ----------------------------------------------------------------------------

void ordering_test()
{
    hpx::util::tuple<int, float> t1(4, 3.3f);
    hpx::util::tuple<short, float> t2(5, 3.3f);
    hpx::util::tuple<long, double> t3(5, 4.4);
    HPX_TEST(t1 < t2);
    HPX_TEST(t1 <= t2);
    HPX_TEST(t2 > t1);
    HPX_TEST(t2 >= t1);
    HPX_TEST(t2 < t3);
    HPX_TEST(t2 <= t3);
    HPX_TEST(t3 > t2);
    HPX_TEST(t3 >= t2);
}

// ----------------------------------------------------------------------------
// - testing const tuples -----------------------------------------------------
// ----------------------------------------------------------------------------
void const_tuple_test()
{
    const hpx::util::tuple<int, float> t1(5, 3.3f);
    HPX_TEST_EQ(hpx::util::get<0>(t1), 5);
    HPX_TEST_EQ(hpx::util::get<1>(t1), 3.3f);
}

// ----------------------------------------------------------------------------
// - testing length -----------------------------------------------------------
// ----------------------------------------------------------------------------
void tuple_length_test()
{
    typedef hpx::util::tuple<int, float, double> t1;
    typedef hpx::util::tuple<> t2;

    HPX_TEST_EQ(hpx::util::tuple_size<t1>::value, std::size_t(3));
    HPX_TEST_EQ(hpx::util::tuple_size<t2>::value, std::size_t(0));

    {
        using t3 = std::array<int, 4>;
        HPX_TEST_EQ(hpx::util::tuple_size<t3>::value, std::size_t(4));
    }
}

// ----------------------------------------------------------------------------
// - testing swap -----------------------------------------------------------
// ----------------------------------------------------------------------------
void tuple_swap_test()
{
    using std::swap;

    hpx::util::tuple<int, float, double> t1(1, 2.0f, 3.0), t2(4, 5.0f, 6.0);
    swap(t1, t2);
    HPX_TEST_EQ(hpx::util::get<0>(t1), 4);
    HPX_TEST_EQ(hpx::util::get<1>(t1), 5.0f);
    HPX_TEST_EQ(hpx::util::get<2>(t1), 6.0);
    HPX_TEST_EQ(hpx::util::get<0>(t2), 1);
    HPX_TEST_EQ(hpx::util::get<1>(t2), 2.0f);
    HPX_TEST_EQ(hpx::util::get<2>(t2), 3.0);

    int i = 1, j = 2;

    hpx::util::tuple<int&> t3(i), t4(j);
    swap(t3, t4);
    HPX_TEST_EQ(hpx::util::get<0>(t3), 2);
    HPX_TEST_EQ(hpx::util::get<0>(t4), 1);
    HPX_TEST_EQ(i, 2);
    HPX_TEST_EQ(j, 1);
}

void tuple_std_test()
{
#if defined(HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE)
    hpx::util::tuple<int, float, double> t1(1, 2.0f, 3.0);
    std::tuple<int, float, double> t2 = t1;
    hpx::util::tuple<int, float, double> t3 = t2;
    HPX_TEST_EQ(std::get<0>(t1), 1);
    HPX_TEST_EQ(std::get<0>(t2), 1);
    HPX_TEST_EQ(std::get<0>(t3), 1);

    HPX_TEST_EQ(hpx::util::get<0>(t1), 1);
    HPX_TEST_EQ(hpx::util::get<0>(t2), 1);
    HPX_TEST_EQ(hpx::util::get<0>(t3), 1);

    HPX_TEST_EQ(std::get<1>(t1), 2.0f);
    HPX_TEST_EQ(std::get<1>(t2), 2.0f);
    HPX_TEST_EQ(std::get<1>(t3), 2.0f);

    HPX_TEST_EQ(hpx::util::get<1>(t1), 2.0f);
    HPX_TEST_EQ(hpx::util::get<1>(t2), 2.0f);
    HPX_TEST_EQ(hpx::util::get<1>(t3), 2.0f);

    HPX_TEST_EQ(std::get<2>(t1), 3.0);
    HPX_TEST_EQ(std::get<2>(t2), 3.0);
    HPX_TEST_EQ(std::get<2>(t3), 3.0);

    HPX_TEST_EQ(hpx::util::get<2>(t1), 3.0);
    HPX_TEST_EQ(hpx::util::get<2>(t2), 3.0);
    HPX_TEST_EQ(hpx::util::get<2>(t3), 3.0);
#endif
}

void tuple_structured_binding_test()
{
#if defined(HPX_HAVE_CXX17_STRUCTURED_BINDINGS)
    auto [a1, a2] = hpx::util::make_tuple(1, '2');

    HPX_TEST_EQ(a1, 1);
    HPX_TEST_EQ(a2, '2');
#endif
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    {
        construction_test();
        element_access_test();
        copy_test();
        mutate_test();
        make_tuple_test();
        tie_test();
        tuple_cat_test();
        equality_test();
        ordering_test();
        const_tuple_test();
        tuple_length_test();
        tuple_swap_test();
        tuple_structured_binding_test();
    }

    return hpx::util::report_errors();
}
