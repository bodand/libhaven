/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-02.
 *
 * benchmark/lzcnt vs ringbuffer/main --
 */

#define NONIUS_RUNNER
#include <random>
#include <vector>

#include "ffs_buffer.hxx"
#include "mx_buffer.hxx"
#include "test_type.hxx"
#include <nonius/main.h++>
#include <nonius/nonius.h++>

NONIUS_BENCHMARK("mutex buffer insert", [](nonius::chronometer meter) {
    std::mt19937 mt(std::random_device{}());
    mx_buffer<test_type> mx_bfr;
    auto insert_fn = [&mx_bfr, &mt] {
        return mx_bfr.insert(test_type::mk_random(mt));
    };

    std::vector<decltype(mx_bfr)::ref_t> fills;
    std::generate_n(std::back_inserter(fills), mx_bfr.capacity(), insert_fn);

    meter.measure(insert_fn);
})

NONIUS_BENCHMARK("ffs buffer insert", [](nonius::chronometer meter) {
    std::mt19937 mt(std::random_device{}());
    ffs_buffer<test_type> ffs_bfr;
    auto insert_fn = [&ffs_bfr, &mt] {
        return ffs_bfr.insert(test_type::mk_random(mt));
    };

    std::vector<decltype(ffs_bfr)::ref_t> fills;
    std::generate_n(std::back_inserter(fills), ffs_bfr.capacity(), insert_fn);

    meter.measure(insert_fn);
})
