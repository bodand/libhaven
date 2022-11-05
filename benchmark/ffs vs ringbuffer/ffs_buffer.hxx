/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-01.
 *
 * benchmark/lzcnt vs ringbuffer/lzcnt_buffer --
 */

#ifndef LIBHAVEN_FFS_BUFFER_HXX
#define LIBHAVEN_FFS_BUFFER_HXX

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ranges>
#include <type_traits>

namespace bit {
    std::size_t
    ffs(std::uint64_t data) noexcept;

    std::size_t
    popcount(std::uint64_t data) noexcept;
}

template<class T, std::size_t PageSize = 4096>
struct ffs_buffer {
    struct ref_t {
        using buffer_type = ffs_buffer<T, PageSize>;
        using size_type = std::size_t;

        [[nodiscard]] size_type
        get_idx() const noexcept {
            return _idx;
        }

        [[nodiscard("Memory management passed down to caller--use the index")]] size_type
        keep_memory() {
            _keep = true;
            return get_idx();
        }

        ref_t() : _buf(nullptr),
                  _ptr(nullptr),
                  _idx(-1),
                  _keep(true) { }

        ref_t(const ref_t& cp)
             : _buf(cp._buf),
               _ptr(cp._ptr),
               _idx(cp._idx),
               _keep(true) { }
        ref_t&
        operator=(const ref_t& cp) {
            assert(this != &cp);

            if (!_keep) _buf->checked_remove(_idx);
            _buf = cp._buf;
            _ptr = cp._ptr;
            _idx = cp._idx;
            _keep = true;
        }
        ref_t(ref_t&& mv) noexcept
             : _buf(std::move(mv._buf)),
               _ptr(std::move(mv._ptr)),
               _idx(std::move(mv._idx)),
               _keep(mv._keep) {
            mv._keep = true;
        }
        ref_t&
        operator=(ref_t&& mv) noexcept {
            assert(this != &mv);

            if (!_keep) _buf->checked_remove(_idx);
            _buf = std::move(mv._buf);
            _ptr = std::move(mv._ptr);
            _idx = std::move(mv._idx);
            _keep = mv._keep;
            mv._keep = true;
        }

        T*
        operator->() noexcept { return _ptr; }
        const T*
        operator->() const noexcept { return _ptr; }

        T&
        operator*() noexcept { return *_ptr; }
        const T&
        operator*() const noexcept { return *_ptr; }

        ~ref_t() noexcept {
            if (!_keep) _buf->checked_remove(_idx);
        }

    private:
        friend ffs_buffer;

        friend bool
        operator==(const ref_t& a, const ref_t& b) {
            return a._ptr == b._ptr;
        }
        friend constexpr auto
        operator<=>(const ref_t& a, const ref_t& b) {
            return a._ptr <=> b._ptr;
        }

        explicit ref_t(buffer_type* buf, T* ptr, size_type idx)
             : _buf(buf),
               _ptr(ptr),
               _idx(idx) { }

        buffer_type* _buf;
        T* _ptr;
        size_type _idx;
        bool _keep = false;
    };

    constexpr const static auto page_size = PageSize;
    constexpr const static auto elem_size = sizeof(T);
    constexpr const static auto max_count = page_size / elem_size;

    using size_type = std::remove_cv_t<decltype(max_count)>;

    static_assert(page_size > 0);
    static_assert(elem_size >= alignof(T));
    static_assert(max_count > 0);

    ffs_buffer() {
#ifndef NDEBUG
        std::ranges::fill(_data, debug_byte);
#endif
    }

    // TODO
    ffs_buffer(const ffs_buffer&) = delete;
    ffs_buffer&
    operator=(const ffs_buffer&) = delete;
    ffs_buffer(ffs_buffer&&) noexcept = delete;
    ffs_buffer&
    operator=(ffs_buffer&&) noexcept = delete;

    [[nodiscard]] constexpr auto
    capacity() const noexcept {
        return max_count;
    }

    [[nodiscard]] bool
    was_full() const noexcept {
        return std::ranges::all_of(_ctrl,
                                   [](const auto& x) { return bit::popcount(x.load(std::memory_order::acquire)) == 0; });
    }

    template<class Tp>
    ref_t
    insert(Tp&& val) {
        auto typed_idx = alloc_next_free_slot();
        if (typed_idx == -1ULL) return {};

        auto raw_idx = typed_idx2buffer_idx(typed_idx);
        try {
            std::construct_at(std::bit_cast<T*>(&_data[raw_idx]), std::forward<Tp>(val));
            return ref_t(this, std::bit_cast<T*>(&_data[raw_idx]), typed_idx);
        } catch (...) {
            dealloc_slot(typed_idx);
            throw;
        }
#ifdef __GNUG__
        __builtin_unreachable();
#elif _MSC_VER
        __assume(false);
#endif
    }

    void
    remove(size_type typed_idx) noexcept(std::is_nothrow_destructible_v<T>) {
        const auto idx = typed_idx2buffer_idx(typed_idx);
        const auto buf_ptr = &_data[idx];
        std::destroy_at(std::bit_cast<T*>(buf_ptr));
        dealloc_slot(typed_idx);
    }

    void
    checked_remove(size_type typed_idx) noexcept(std::is_nothrow_destructible_v<T>) {
        const auto [ctrl_idx, bit_idx] = destructure_idx(typed_idx);

        const auto on_mask = 1ULL << bit_idx;
        const auto off_mask = ~(1ULL << bit_idx);
        const auto val = _ctrl[ctrl_idx].load(std::memory_order::acquire);
        auto with_value = val & off_mask;
        const auto without_value = val | on_mask;
        if (_ctrl[ctrl_idx].compare_exchange_strong(with_value,
                                                    without_value,
                                                    std::memory_order::acq_rel)) {
            remove(typed_idx);
        }
    }

    ~ffs_buffer() noexcept {
#ifndef NDEBUG
        assert(true);
        size_type idx = 0;
        auto err = std::ranges::find_if_not(_data,
                                            [&idx](auto x) { ++idx; return x == debug_byte; });
        assert(err == std::end(_data));
#endif
    }

private:
    constexpr static const auto debug_byte = std::byte(0xAF);

    constexpr auto
    destructure_idx(size_type idx) const noexcept {
        struct destructured_idx {
            size_type ctrl_idx;
            size_type bit_idx;
        };

        return destructured_idx(idx / ctrl_size, idx % ctrl_size);
    }

    void
    dealloc_slot(size_type idx) {
        auto [ctrl_idx, bit_idx] = destructure_idx(idx);
        assert(ctrl_idx < _ctrl.size());
        assert(bit_idx < sizeof(_ctrl[ctrl_idx]));

        auto mask = 1ULL << bit_idx; // 0 everywhere except for bit_idx
        auto val = _ctrl[ctrl_idx].load(std::memory_order::acquire);
        val |= mask;
        _ctrl[ctrl_idx].store(val, std::memory_order::release);
#ifndef NDEBUG
        auto raw_idx = typed_idx2buffer_idx(idx);
        std::fill_n(_data + raw_idx, elem_size, debug_byte);
#endif
    }

    /**
     * Translate the outside visible type based index, to the internal byte
     * based buffer.
     * This can be used to transform index data from outside API calls into
     * indices that can be used to index the data block.
     */
    size_type
    typed_idx2buffer_idx(size_type i) {
        return i * elem_size;
    }

    size_type
    alloc_next_free_slot() noexcept {
        for (std::size_t i = 0; i < _ctrl.size(); ++i) {
            bool cont = false;
            std::uint64_t set_val; // value with bit swapped
            std::uint64_t idx;     // index of swapped bit
            std::uint64_t val = _ctrl[i].load(std::memory_order::acquire);
            do {
                idx = bit::ffs(val);
                if (idx == size_type(-1)) {
                    // if we encounter a full ctrl block, go to the next
                    cont = true;
                    break;
                }
                const auto mask = ~(1ULL << idx); // 1 everywhere except at idx
                set_val = val & mask;
            } while (!_ctrl[i].compare_exchange_strong(val,
                                                       set_val,
                                                       std::memory_order::release,
                                                       std::memory_order::acquire));
            if (cont) continue;
            return i * sizeof(_ctrl[i]) * CHAR_BIT + idx;
        }
        return size_type(-1);
    }

    template<std::size_t I>
    struct size_constant : std::integral_constant<std::size_t, I> {
    };

    using ctrl_impl_type =
           std::conditional_t<max_count / 64 == 0, // if max_cnt < 64, use 1
                              size_constant<1>,
                              std::conditional_t<max_count % 64 == 0,                 // if max_cnt divisible by 64...
                                                 size_constant<max_count / 64>,       // use exact amount needed
                                                 size_constant<max_count / 64 + 1>>>; // else take ceil
    constexpr const static auto ctrl_size = ctrl_impl_type::value;
    static_assert(ctrl_size > 0, "cannot have 0 size control block");

    std::array<std::atomic<std::uint64_t>, ctrl_size> _ctrl{{0xFFFF'FFFF'FFFF'FFFFULL}};
    alignas(T) std::byte _data[page_size]{};
};

#endif
