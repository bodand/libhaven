/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-12.
 *
 * benchmark/pool/mx_pool/mx_pool --
 */
#ifndef LIBHAVEN_MX_POOL_HXX
#define LIBHAVEN_MX_POOL_HXX

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include <haven/mem/page-allocator.hxx>
#include <haven/mem/puddle.hxx>

namespace hvn {
    template<class T,
             allocator Allocator = page_allocator>
    struct pool {
        template<class... Args>
        [[nodiscard]] T*
        allocate(Args&&... args) {
            T* ret = nullptr;
            std::size_t idx;
            while (ret == nullptr) {
                {
                    std::scoped_lock lck(_ctrl_mx);
                    auto empty = std::ranges::find(_ctrl, slot_empty);
                    idx = std::distance(_ctrl.begin(), empty);
                    if (empty == _ctrl.end()) {
                        _puddles.emplace_back();
                        _ctrl.push_back(slot_used);
                        idx = _puddles.size() - 1;
                        empty = _ctrl.begin() + (_ctrl.size() - 1);
                    }
                    *empty = slot_used;
                }
                ret = _puddles[idx]->try_allocate(std::forward<Args>(args)...);
            }
            for (std::size_t i = 0; i < _puddles.size(); ++i) {
                if (i != idx) _puddles[i]->unused_in_allocation();
            }
            return ret;
        }

        void
        deallocate(T* mem) {
            for (std::size_t i = 0; i < _puddles.size(); ++i) {
                if (_puddles[i]->deallocate(mem)) {
                    std::scoped_lock lck(_ctrl_mx);
                    _ctrl[i] = slot_empty;
                    return;
                }
            }
        }

    private:
        constexpr const static auto slot_used = std::uint_fast8_t{0};
        constexpr const static auto slot_empty = std::uint_fast8_t{0xFF};
        using ctrl_type = std::vector<std::uint_fast8_t>; // maybe simd-ify, prolly not though

        ctrl_type _ctrl = ctrl_type(1, slot_empty);
        std::mutex _ctrl_mx;

        std::vector<std::unique_ptr<puddle<T, Allocator>>> _puddles{std::make_unique<puddle<T, Allocator>>()};
    };
}

#endif
