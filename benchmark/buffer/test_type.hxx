/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-06.
 *
 * benchmark/buffer/test_type --
 */
#ifndef LIBHAVEN_TEST_TYPE_HXX
#define LIBHAVEN_TEST_TYPE_HXX

#include <cstdint>
#include <random>

struct test_type {
    template<class E>
    static test_type
    mk_random(E& rng) {
        std::uniform_int_distribution<std::uint32_t> dist(0, 65000);
        return test_type(dist(rng), dist(rng));
    }

    test_type(std::uint32_t a,
              std::uint32_t b) : a(a),
                                 b(b) { }

    std::uint32_t a;
    std::uint32_t b;
};
static_assert(sizeof(test_type) == 8);

#endif
