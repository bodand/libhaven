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

#include <memory>
#include <mutex>
#include <variant>

#include <haven/common/check_conditions.hxx>
#include <haven/mem/page-allocator.hxx>
#include <xsimd/xsimd.hpp>

namespace hvn {
    template<class T,
             allocator Allocator = page_allocator>
    struct puddle {
        static_assert(sizeof(T) >= alignof(T),
                      "Trivial implementation reasons");
        using value_type = T;
        using allocator_type = Allocator;

        puddle(allocator_type* allocator)
             : _allocator(allocator),
               _ctrl(_allocator->page_size() / sizeof(T), slot_empty),
               _state(_allocator->reserve(_allocator->page_size())) {
            postcondition()([](auto size) { return size > 0; }, _allocator->page_size() / sizeof(T));
            postcondition()([this](auto) { return !valid_memory(); }, _state.index());
        }

        [[nodiscard]] std::size_t
        capacity() const noexcept {
            return _ctrl.size();
        }

        void
        unused_in_allocation() {
            std::scoped_lock lck(_puddle_mx);
            dec_use();
        }

        template<class... Args>
        [[nodiscard]] T*
        try_allocate(Args&&... args) {
            T* ret;
            std::size_t idx;
            {
                std::scoped_lock lck(_puddle_mx);

                inc_use();
                idx = find_empty();

                postcondition()([this](auto) { return valid_memory(); }, _state.index());
                postcondition()([](auto use) { return use > 0; }, _use);

                if (idx == std::size_t(-1)) return nullptr;
                ++_allocated_count;
            }
            auto& page = std::get<typename allocator_type::committed_page>(_state);
            ret = std::construct_at(reinterpret_cast<T*>(page.base_addr()) + idx, std::forward<Args>(args)...);

            // clang-format off
            postcondition()("ret is either null or in the page"_msg,
                      [](auto page_, auto ret_) {
                          return ret_ == nullptr
                                 || (page_.base_addr() <= ret_ && ret_ < page_.base_addr() + page_.size());
                      }, page, reinterpret_cast<std::byte*>(ret)); // clang-format on
            return ret;
        }

        [[nodiscard]] bool
        deallocate(T* ptr) {
            if (ptr == nullptr) return true;
            precondition()([this] { return valid_memory(); });

            auto& page = std::get<typename allocator_type::committed_page>(_state);
            if (!(page.base_addr() <= reinterpret_cast<std::byte*>(ptr)
                  && reinterpret_cast<std::byte*>(ptr) < page.base_addr() + page.size())) return false;

            std::destroy_at(ptr);
            auto idx = std::distance(reinterpret_cast<T*>(page.base_addr()), ptr);
            {
                std::scoped_lock lck(_puddle_mx);
                _ctrl[idx] = slot_empty;
                ++_deallocated_count;

                postcondition()([](auto slot) { return slot == slot_empty; }, _ctrl[idx]);
            }

            postcondition()([this] { return valid_memory(); });
            return true;
        }

        ~puddle() noexcept {
#ifdef HAVEN_DBG_PUDDLE_TRACE
            try {
                std::osyncstream ostr(std::cerr);
                ostr << "puddle@" << static_cast<void*>(this) << "\n";
                ostr << "\tlifetime allocated: " << _allocated_count << "\n";
                ostr << "\tlifetime deallocated: " << _deallocated_count << "\n";
                bool good = true;
                auto base_addr = std::visit(
                       [&_allocator = *_allocator](const auto& page) {
                           return static_cast<std::byte*>(page.base_addr());
                       },
                       _state);
                for (std::size_t i = 0; i < _ctrl.size(); ++i) {
                    if (_ctrl[i] == slot_used) {
                        if (good) {
                            ostr << "\t!! the elements at the following memory addresses have not been deallocated !!\n";
                            good = false;
                        }
                        ostr << "\t\t- " << base_addr + sizeof(T) * i << "\n";
                    }
                }
                if (good) {
                    ostr << "\t.. puddle has deallocated all contained items ..\n";
                }
            } catch (...) {
                // debug related logging failed, oh well
            }
#endif
            std::visit(
                   [&_allocator = *_allocator](const auto& page) {
                       _allocator.deallocate(page);
                   },
                   _state);
        }

    private:
        constexpr const static auto slot_used = std::uint_fast8_t{0};
        constexpr const static auto slot_empty = std::uint_fast8_t{0xFF};
        struct is_used_slot {
            constexpr auto
            operator()(auto elem) const noexcept {
                return elem == slot_used;
            }
        };

        using ctrl_type = std::vector<std::uint_fast8_t, xsimd::default_allocator<std::uint_fast8_t>>;

        void
        inc_use() {
            if (_use < 0b111u) {
                ++_use;
                if (_use == 0b001u) retake_buffer();
            }

            postcondition()([](auto use) { return use > 0u; }, _use);
            postcondition()([](auto use) { return use <= 0b111u; }, _use);
        }

        void
        dec_use() {
            if (_use > 0b000u) {
                --_use;
                if (_use == 0b000u) give_up_buffer();
            }

            postcondition()([](auto use) { return use < 0b111u; }, _use);
            postcondition()([](auto use) { return use >= 0u; }, _use);
        }

        void
        retake_buffer() {
            _state = std::visit(
                   [&_allocator = *_allocator](const auto& page) {
                       return _allocator.commit(page);
                   },
                   _state);

            postcondition()([this](auto) { return valid_memory(); }, _state.index());
        }

        void
        give_up_buffer() {
            precondition()([this](auto) { return valid_memory(); }, _state.index());

            if (std::ranges::any_of(_ctrl, is_used_slot{})) return;
            _state = std::visit(
                   [&_allocator = *_allocator](const auto& page) {
                       return _allocator.loan(page);
                   },
                   _state);

            postcondition()([this](auto) { return !valid_memory(); }, _state.index());
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
                    auto found = std::ranges::find(&_ctrl[i], &_ctrl[i] + simd_size, slot_empty);
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

        std::uint8_t _use = 0b000u;
        std::mutex _puddle_mx{};
        allocator_type* _allocator;
        ctrl_type _ctrl;
        std::variant<typename allocator_type::allocated_page,
                     typename allocator_type::committed_page,
                     typename allocator_type::loaned_page>
               _state;
#ifdef HAVEN_DBG_PUDDLE_TRACE
        std::size_t _allocated_count{};
        std::size_t _deallocated_count{};
#endif
    };
}

#endif
