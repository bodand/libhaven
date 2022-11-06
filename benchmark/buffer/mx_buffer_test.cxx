/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-02.
 *
 * benchmark/ffs vs ringbuffer/ffs_buffer_test --
 *   Tests for the ffs based buffer implementation.
 */

#include <algorithm>
#include <iostream>
#include <latch>
#include <mutex>
#include <numeric>
#include <ranges>
#include <set>
#include <thread>
#include <vector>

#include <boost/ut.hpp>

#include "mx_buffer.hxx"
#include "test_type.hxx"
using namespace boost::ut;

[[maybe_unused]] const suite mx_buffer_tests = [] {
    "capacity is reported to maximum PageSize/type size"_test = [] {
        constexpr const auto exp_size = 4096 / sizeof(test_type);
        mx_buffer<test_type> buf;
        expect(that % buf.capacity() <= exp_size);
    };

    "capacity is reported to be PageSize/(alignof + sizeof)"_test = [] {
        constexpr const auto exp_size = 4096 / (alignof(test_type) + sizeof(test_type));
        mx_buffer<test_type> buf;
        expect(that % buf.capacity() == exp_size);
    };

    "empty buffer behaves"_test = [] {
        mx_buffer<test_type> buf;

        "empty buffer was not full"_test = [&buf] {
            expect(not buf.was_full());
        };

        "empty buffer can be inserted into"_test = [&buf] {
            auto ptr = buf.insert(test_type(1, 2));
            expect(that % ptr->a == 1);
            expect(that % ptr->b == 2);
        };
    };

    "full buffer behaves"_test = [] {
        mx_buffer<test_type> buf;
        std::vector<decltype(buf)::ref_t> ref_vec;
        ref_vec.reserve(buf.capacity());
        std::generate_n(std::back_inserter(ref_vec),
                        buf.capacity(),
                        [&buf]() {
                            return buf.insert(test_type(1, 1));
                        });

        "full buffer was full"_test = [&buf] {
            expect(buf.was_full());
        };

        "full buffer returns null reference when trying to add more"_test = [&buf] {
            auto null = std::remove_cvref_t<decltype(buf)>::ref_t();
            auto got = buf.insert(test_type(2, 2));

            expect(null == got);
        };
    };

    "many threads can insert into empty buffer at the same time"_test = [] {
        const auto thr_cnt = std::jthread::hardware_concurrency();

        mx_buffer<test_type> buf;
        std::set<decltype(buf)::ref_t> tests;
        std::mutex tests_mx;
        {
            std::latch starter(1);
            std::vector<std::jthread> workers;
            workers.reserve(thr_cnt);
            std::generate_n(std::back_inserter(workers),
                            thr_cnt,
                            [&, i = 1]() mutable {
                                return std::jthread([&starter, &tests, &tests_mx, &buf, i = i++]() {
                                    auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
                                    starter.wait();
                                    auto ptr = buf.insert(test_type(i, static_cast<std::uint32_t>(id)));

                                    std::scoped_lock lck(tests_mx);
                                    tests.insert(std::move(ptr));
                                });
                            });
            starter.count_down();
        }

        "all threads got different pointers"_test = [&tests, thr_cnt] {
            expect(that % tests.size() == thr_cnt);
        };

        "each thread only got once"_test = [&tests] {
            std::vector<int> found(tests.size());
            std::ranges::transform(tests, found.begin(), [](const auto& tt) {
                return tt->a;
            });
            std::ranges::sort(found);

            std::vector<int> ids(tests.size());
            std::iota(ids.begin(), ids.end(), 1);

            expect((that % ids.size() == tests.size()) >> fatal);

            auto id_it = ids.cbegin();
            for (auto a : found) {
                expect(that % a == *(id_it++));
            }
        };
    };
};

int
main() {
    std::cout << "hw concurrency used: " << std::thread::hardware_concurrency() << "\n";

    /* automagically runs suites */
}
