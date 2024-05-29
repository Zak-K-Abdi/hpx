//  Copyright (c) 2024 Zakaria Abdi
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

//////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(2, 101);

template <typename IteratorTag>
void test_contains(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    const std::size_t n = c.size();
    c.at(n / 2) = 1;

    bool result1 =
        hpx::contains(iterator(std::begin(c)), iterator(std::end(c)), int(1));
    HPX_TEST_EQ(result1, true);

    bool result2 =
        hpx::contains(iterator(std::begin(c)), iterator(std::end(c)), int(110));
    HPX_TEST_EQ(result2, false);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    const std::size_t n = c.size();
    c.at(n / 2) = 1;

    bool result1 = hpx::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(1));
    HPX_TEST_EQ(result1, true);

    bool result2 = hpx::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(110));
    HPX_TEST_EQ(result2, false);
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_contains_explicit_sender_direct(
    Policy l, ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    const std::size_t n = c.size();
    c.at(n / 2) = 1;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    bool result1 = hpx::contains(policy.on(exec), iterator(std::begin(c)),
        iterator(std::end(c)), int(1));
    HPX_TEST_EQ(result1, true);

    bool result2 = hpx::contains(policy.on(exec), iterator(std::begin(c)),
        iterator(std::end(c)), int(110));
    HPX_TEST_EQ(result2, false);
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_contains_explicit_sender(Policy l, ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    std::vector<int> c(10007);
    const std::size_t n = c.size();
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(n / 2) = 1;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));
    auto result1 = ex::just(iterator(std::begin(c)), iterator(std::end(c)),
        int(1) | hpx::contains(policy.on(exec)) | tt::sync_wait());

    HPX_TEST_EQ(hpx::get<0>(*result1), true);

    auto result2 = ex::just(iterator(std::begin(c)), iterator(std::end(c)),
        int(110) | hpx::contains(policy.on(exec)) | tt::sync_wait());

    HPX_TEST_EQ(hpx::get<0>(*result2), false);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    const std::size_t n = c.size();
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(n / 2) = 1;

    hpx::future<bool> result1 = hpx::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(1));
    result1.wait();
    HPX_TEST_EQ(result1.get(), true);

    hpx::future<bool> result2 = hpx::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(110));
    result2.wait();
    HPX_TEST_EQ(result2.get(), false);
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_contains_explicit_sender_direct_async(
    Policy l, ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    std::vector<int> c(10007);
    const std::size_t n = c.size();
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(n / 2) = 1;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    auto result1 = hpx::contains(p.on(exec), iterator(std::begin(c)),
                       iterator(std::end(c)), int(1)) |
        tt::sync_wait();
    HPX_TEST_EQ(hpx::get<0>(*result1), true);

    auto result2 = hpx::contains(p.on(exec), iterator(std::begin(c)),
                       iterator(std::end(c)), int(110)) |
        tt::sync_wait();
    HPX_TEST_EQ(hpx::get<0>(*result2), false);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_contains_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    const std::size_t n = c.size();
    c.at(n / 2) = 0;
    bool caught_exception = false;
    try
    {
        hpx::contains(decorated_iterator(std::begin(c),
                          []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));

        HPX_TEST(false);
    }

    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<hpx::execution::sequenced_policy,
            IteratorTag>::call(hpx::execution::seq, e);
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    const std::size_t n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;

    bool caught_exception = false;
    try
    {
        hpx::contains(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));

        HPX_TEST(false);
    }

    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_exception_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    const std::size_t n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> result = hpx::contains(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));
        returned_from_algorithm = true;
        result.get();
        HPX_TEST(false);
    }

    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

//////////////////////////////////////////////////////////////////////////////

template <typename ExPolicy, typename IteratorTag>
void test_contains_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_b<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    const std::size_t n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;
    bool caught_bad_alloc = false;
    try
    {
        hpx::contains(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), int(0));
        HPX_TEST(false);
    }

    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_bad_alloc_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    const std::size_t n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;
    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> result = hpx::contains(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), int(0));
        returned_from_algorithm = true;
        result.get();
        HPX_TEST(false);
    }

    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST(returned_from_algorithm);
}