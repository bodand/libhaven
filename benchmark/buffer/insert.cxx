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

#include "ffs_buffer.hxx"
#include "mx_buffer.hxx"
#include "test_type.hxx"
#include <nonius/main.h++>
#include <nonius/nonius.h++>

NONIUS_BENCHMARK("mutex buffer insert", [](nonius::chronometer meter) {
    std::mt19937 mt(std::random_device{}());
    mx_buffer<test_type> mx_bfr;
    meter.measure([&mx_bfr, &mt] {
        return mx_bfr.insert(test_type::mk_random(mt));
    });
})

NONIUS_BENCHMARK("ffs buffer insert", [](nonius::chronometer meter) {
    std::mt19937 mt(std::random_device{}());
    ffs_buffer<test_type> ffs_bfr;
    meter.measure([&ffs_bfr, &mt] {
        return ffs_bfr.insert(test_type::mk_random(mt));
    });
})
