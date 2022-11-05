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
#include <latch>
#include <mutex>
#include <numeric>
#include <ranges>
#include <set>
#include <thread>
#include <vector>

#include <boost/ut.hpp>

#include "ffs_buffer.hxx"
using namespace boost::ut;

[[maybe_unused]] const suite ffs_tests = [] {
    "ffs returns -1 for zero"_test = [] {
        expect(that % bit::ffs(0) == -1ULL);
    };

    "ffs returns 0 for full-1 value"_test = [] {
        expect(that % bit::ffs(0xFFFF'FFFF'FFFF'FFFFULL) == 0ULL);
    };

    constexpr const auto size_type_bits = sizeof(std::size_t) * CHAR_BIT;
    std::vector<int> shifts(size_type_bits);
    std::iota(shifts.begin(), shifts.end(), 0);
    "ffs returns 32 - returned bit"_test = [](int shift) {
        const std::size_t val = 0b1ULL << shift;
        expect(that % bit::ffs(val) == shift);
    } | shifts;
};

[[maybe_unused]] const suite popcount_tests = [] {
    "popcount returns 0 for zero"_test = [] {
        expect(that % bit::popcount(0ULL) == 0ULL);
    };

    "popcount returns non-zero for non-zero values"_test = [] {
        expect(that % bit::popcount(11ULL) == 3ULL);
    };
};

struct test_type {
    test_type(std::uint32_t a,
              std::uint32_t b) : a(a),
                                 b(b) { }

    std::uint32_t a;
    std::uint32_t b;
};
static_assert(sizeof(test_type) == 8);

[[maybe_unused]] const suite ffs_buffer_test = [] {
    "capacity is reported to be PageSize/type size"_test = [] {
        constexpr const auto exp_size = 4096 / sizeof(test_type);
        ffs_buffer<test_type> buf;
        expect(that % buf.capacity() == exp_size);
    };

    "empty buffer behaves"_test = [] {
        ffs_buffer<test_type> buf;

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
        ffs_buffer<test_type> buf;
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

        ffs_buffer<test_type> buf;
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
