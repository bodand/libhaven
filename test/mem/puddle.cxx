/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-17.
 *
 * test/mem/puddle --
 *   Test suite for the puddle object.
 */

#include <random>
#include <thread>
#include <tuple>

#include <boost/ut.hpp>
#include <haven/mem/puddle.hxx>

using namespace boost::ut;

namespace {
    struct bad_uint128 {
        std::uint64_t upper;
        std::uint64_t lower;
    };

    struct bad_uint256 {
        bad_uint128 upper;
        bad_uint128 lower;
    };
    static_assert(sizeof(bad_uint256) == sizeof(bad_uint128) * 2);
}

[[maybe_unused]] const suite puddle_suite = [] {
    "puddle is constructible with allocator pointer"_test = [] {
        expect(constant<std::is_constructible_v<hvn::puddle<bad_uint128>, hvn::page_allocator*>>);
    };

    hvn::page_allocator alloc;
    "puddle can allocate a positive maximum amount of objects"_test = [&alloc] {
        hvn::puddle<bad_uint128> puddle(&alloc);
        expect(that % puddle.capacity() > 0u);
    };

    "puddle can store half amount of objects with twice the size"_test = [&alloc] {
        hvn::puddle<bad_uint128> smaller(&alloc);
        hvn::puddle<bad_uint256> bigger(&alloc);
        expect(that % (bigger.capacity() * 2) == smaller.capacity());
    };

    "empty puddle"_test = [&alloc] {
        hvn::puddle<bad_uint128> puddle(&alloc);

        decltype(puddle)::value_type* memory;
        "allocation does not return nullptr"_test = [&puddle, &memory] {
            memory = puddle.try_allocate();
            expect(that % memory != nullptr);
        };

        "deallocation returns true for the same puddle"_test = [&puddle, memory] {
            expect(puddle.deallocate(memory));
        };

        "allocation can take parameters passed to the constructor"_test = [&puddle, &memory] {
            memory = puddle.try_allocate(std::uint64_t{42}, std::uint64_t{69});
            expect(that % memory != nullptr);
            expect(that % memory->upper == 42ULL);
            expect(that % memory->lower == 69ULL);
            expect(puddle.deallocate(memory));
        };
    };

    "full puddle"_test = [&alloc] {
        hvn::puddle<bad_uint128> puddle(&alloc);
        std::vector<bad_uint128*> buf;

        "puddle can allocate capacity amount of items"_test = [&puddle, &buf] {
            std::ranges::generate_n(std::back_inserter(buf), puddle.capacity(), [&puddle] {
                return puddle.try_allocate();
            });
            expect(std::ranges::all_of(buf, [](auto ptr) { return ptr != nullptr; }));
        };

        "full puddle returns null on allocation"_test = [&puddle] {
            auto failed_alloc = puddle.try_allocate();
            expect(that % failed_alloc == nullptr);
        };

        for (auto ptr : buf) {
            std::ignore = puddle.deallocate(ptr);
        }
    };

    "multithreaded functionality"_test = [&alloc] {
        hvn::puddle<bad_uint128> puddle(&alloc);
        auto thr_function = [](auto puddle_ptr) {
            auto& puddle = *puddle_ptr;
            std::vector<bad_uint128*> buf;
            std::mt19937_64 rng(std::random_device{}());
            std::uniform_int_distribution op(0, 1);
            std::uniform_int_distribution<std::uint64_t> value(1ULL, 8192ULL);

            for (int i = 0; i < 2048; ++i) {
                if (op(rng) == 0) {
                    auto nonnull = std::ranges::find_if_not(buf, [](auto ptr) { return ptr != nullptr; });
                    if (nonnull != buf.end()) {
                        auto ptr = *nonnull;
                        *nonnull = nullptr;

                        expect(that % ptr->upper != 0);
                        expect(that % ptr->lower != 0);
                        expect(puddle.deallocate(ptr));
                    }
                }
                else {
                    auto upper = value(rng);
                    auto lower = value(rng);
                    auto ptr = puddle.try_allocate(upper, lower);
                    if (ptr != nullptr) { // puddle full
                        expect(that % ptr->upper == upper);
                        expect(that % ptr->lower == lower);
                        buf.push_back(ptr);
                    }
                }
            }

            for (auto ptr : buf) {
                std::ignore = puddle.deallocate(ptr);
            }
        };

        std::vector<std::jthread> workers;
        for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
            workers.emplace_back(thr_function, &puddle);
        }
    };
};
