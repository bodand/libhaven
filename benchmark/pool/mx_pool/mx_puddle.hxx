/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-12.
 *
 * benchmark/pool/mx_pool/mx_puddle --
 */
#ifndef LIBHAVEN_PUDDLE_HXX
#define LIBHAVEN_PUDDLE_HXX

#include <bit>
#include <memory>
#include <mutex>
#include <variant>

#include <check_conditions.hxx>
#include <page-allocator.hxx>
#include <xsimd/xsimd.hpp>

template<class T,
         haven_allocator Allocator = page_allocator>
struct puddle {
    static_assert(sizeof(T) >= alignof(T),
                  "Trivial implementation reasons");
    using value_type = T;
    using allocator_type = Allocator;

    puddle()
         : _state(_allocator.reserve(_allocator.page_size())),
           _ctrl(_allocator.page_size() / sizeof(T), slot_empty) {
        postcondition([](auto size) { return size > 0; }, _allocator.page_size() / sizeof(T));
        postcondition([this](auto) { return valid_memory(); }, _state.index());
    }

    void
    unused_in_allocation() {
        std::scoped_lock lck(_puddle_mx);
        dec_use();
    }

    template<class... Args>
    T*
    try_allocate(Args&&... args) {
        T* ret = nullptr;
        std::size_t idx = -1;
        {
            std::scoped_lock lck(_puddle_mx);

            inc_use();
            idx = find_empty();

            postcondition([this](auto) { return valid_memory(); }, _state.index());
            postcondition([](auto use) { return use > 0; }, _use);
        }
        auto& page = std::get<allocator_type::committed_page>(_state);
        ret = std::construct_at(std::bit_cast<T*>(page.base_addr) + idx, std::forward<Args>(args)...);

        // clang-format off
        postcondition("ret is either null or in the page",
                      [](auto page, auto ret) {
                          return ret == nullptr
                                 || (page.base_addr <= ret && ret < page.base_addr + page.size);
                      }, page, ret); // clang-format on
        return ret;
    }

    bool
    deallocate(T* ptr) {
        if (ptr == nullptr) return true;
        precondition([this] { valid_memory(); });
        auto& page = std::get<allocator_type::committed_page>(_state);
        if (!(page.base_addr <= ptr && ptr < page.base_addr + page.size)) return false;

        std::destroy_at(ptr);
        auto idx = std::distance(std::bit_cast<T*>(page.base_addr), ptr);
        {
            std::scoped_lock lck(_puddle_mx);
            _ctrl[idx] = slot_empty;

            postcondition([](auto slot) { return slot == slot_empty; }, _ctrl[idx]);
        }
        postcondition([this] { valid_memory(); });
        return true;
    }

private:
    constexpr const static auto slot_used = std::uint_fast8_t{0};
    constexpr const static auto slot_empty = std::uint_fast8_t{0xFF};
    using ctrl_type = std::vector<std::uint_fast8_t, xsimd::default_allocator<std::uint_fast8_t>>;

    void
    inc_use() {
        if (_use < 0b111u) {
            ++_use;
            if (_use == 0b001u) retake_buffer();
        }

        postcondition([](auto use) { return use > 0u; }, _use);
        postcondition([](auto use) { return use <= 0b111u; }, _use);
    }

    void
    dec_use() {
        if (_use > 0b000u) {
            --_use;
            if (_use == 0b000u) give_up_buffer();
        }

        postcondition([](auto use) { return use < 0b111u; }, _use);
        postcondition([](auto use) { return use >= 0u; }, _use);
    }

    void
    retake_buffer() {
        _state = std::visit(
               [&_allocator = _allocator](const auto& page) {
                   return _allocator.commit(page);
               },
               _state);

        postcondition([this](auto) { return valid_memory(); }, _state.index());
    }

    void
    give_up_buffer() {
        precondition([this](auto) { return valid_memory(); }, _state.index());

        _state = std::visit(
               [&_allocator = _allocator](const auto& page) {
                   return _allocator.loan(page);
               },
               _state);

        postcondition([this](auto) { return !valid_memory(); }, _state.index());
    }

    bool
    valid_memory() {
        return std::holds_alternative<typename Allocator::committed_page>(_state);
    }

    std::size_t
    find_empty() {
        namespace xs = xsimd;
        using batch_type = xs::batch<std::uint_fast8_t>;
        auto empty = batch_type ::broadcast(slot_empty);

        auto simd_size = batch_type ::size;
        auto ctrl_size = _ctrl.size();
        auto vectorized_size = ctrl_size - ctrl_size % simd_size;

        for (std::size_t i = 0; i < vectorized_size; i += simd_size) {
            auto batch = batch_type ::load_aligned(&_ctrl[i]);
            auto bools = batch == empty;
            auto has_empty = xs::any(bools);
            if (has_empty) {
                auto found = std::ranges::find(&_ctrl[i], &_ctrl[i] + simd_size, std::uint_fast8_t{0});
                *found = slot_used;
                return std::distance(&_ctrl[0], found);
            }
        }

        if (vectorized_size != ctrl_size) {
            auto found = std::ranges::find(&_ctrl[vectorized_size],
                                           _ctrl.data() + ctrl_size,
                                           std::uint_fast8_t{0});
            if (found != _ctrl.data() + ctrl_size) {
                *found = slot_used;
                return std::distance(&_ctrl[0], found);
            }
        }
        return std::size_t(-1);
    }

    std::uint8_t _use = 0b100u;
    std::mutex _puddle_mx{};
    allocator_type _allocator{};
    ctrl_type _ctrl;
    std::variant<typename allocator_type::reserved_page,
                 typename allocator_type::comitted_page,
                 typename allocator_type::loaned_page>
           _state;
};

#endif
