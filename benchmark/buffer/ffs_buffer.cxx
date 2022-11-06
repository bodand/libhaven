/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-01.
 *
 * benchmark/lzcnt vs ringbuffer/lzcnt_buffer --
 */

#include "ffs_buffer.hxx"

#ifdef _MSC_VER
#  include <intrin.h>
#  pragma intrinsic(_BitScanForward64)
#  pragma intrinsic(__popcnt64)
#endif

#ifdef __GNUG__

std::size_t
bit::ffs(std::uint64_t data) noexcept {
    return __builtin_ffsll(static_cast<long long>(data)) - 1;
}

std::size_t
bit::popcount(std::uint64_t data) noexcept {
    return __builtin_popcountll(static_cast<long long>(data));
}

#elif defined(_MSC_VER)

std::size_t
bit::ffs(std::uint64_t data) noexcept {
    unsigned long idx = 0;
    auto found = _BitScanForward64(&idx, data);
    return found * std::size_t(idx) - static_cast<int>(!static_cast<bool>(found));
}

std::size_t
bit::popcount(std::uint64_t data) noexcept {
    return __popcnt64(data);
}

#endif
