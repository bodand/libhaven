/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-06.
 *
 * benchmark/buffer/mx_buffer --
 */
#ifndef LIBHAVEN_MX_BUFFER_HXX
#define LIBHAVEN_MX_BUFFER_HXX

#include <algorithm>
#include <bit>
#include <cstddef>
#include <mutex>

template<class T, std::size_t PageSize = 4096>
class mx_buffer {
    constexpr static const auto debug_byte = std::byte(0xAF);

    template<class S>
    struct store {
        enum class status_type : std::uint8_t {
            Empty,
            Deleted,
            InUse
        };

        store() {
#ifndef NDEBUG
            std::ranges::fill(_data, debug_byte);
#endif
        }

        [[nodiscard]] S*
        get() noexcept {
            return std::bit_cast<S*>(&_data);
        }

        [[nodiscard]] const S*
        get() const noexcept {
            return std::bit_cast<const S*>(&_data);
        }

        template<class... Args>
        void
        construct(Args&&... args) {
            try {
                std::construct_at(std::bit_cast<S*>(&_data), std::forward<Args>(args)...);
                _status = status_type::InUse;
            } catch (...) {
#ifndef NDEBUG
                std::ranges::fill(_data, debug_byte);
#endif
                throw;
            }
        }

        void
        destruct() {
            try {
                std::destroy_at(std::bit_cast<S*>(&_data));
                _status = status_type::Deleted;
            } catch (...) {
                // why the fuck does a destructor throw in the first place
                // be a good citizen and don't throw in the destructor
#ifndef NDEBUG
                std::ranges::fill(_data, debug_byte);
                _status = status_type::Empty;
#endif
                throw;
            }
        }

        [[nodiscard]] bool
        valid() const noexcept {
            return _status == status_type::InUse;
        }

    private:
        status_type _status = status_type::Empty;
        alignas(S) std::byte _data[sizeof(S)]{};
    };

public:
    using size_type = decltype(PageSize);
    struct ref_t {
        using buffer_type = mx_buffer<T, PageSize>;
        using size_type = typename mx_buffer<T, PageSize>::size_type;

        [[nodiscard]] size_type
        get_idx() const noexcept {
            return _idx;
        }

        [[nodiscard("Memory management passed down to caller--use the index")]] size_type
        keep_memory() {
            _keep = true;
            return get_idx();
        }

        ref_t()
             : _buf(nullptr),
               _store_ptr(nullptr),
               _idx(-1),
               _keep(true) { }

        ref_t(const ref_t& cp)
             : _buf(cp._buf),
               _store_ptr(cp._store_ptr),
               _idx(cp._idx),
               _keep(true) { }
        ref_t&
        operator=(const ref_t& cp) {
            assert(this != &cp);

            if (!_keep) _buf->checked_remove(_idx);
            _buf = cp._buf;
            _store_ptr = cp._store_ptr;
            _idx = cp._idx;
            _keep = true;
        }
        ref_t(ref_t&& mv) noexcept
             : _buf(std::move(mv._buf)),
               _store_ptr(std::move(mv._store_ptr)),
               _idx(std::move(mv._idx)),
               _keep(mv._keep) {
            mv._keep = true;
        }
        ref_t&
        operator=(ref_t&& mv) noexcept {
            assert(this != &mv);

            if (!_keep) _buf->checked_remove(_idx);
            _buf = std::move(mv._buf);
            _store_ptr = std::move(mv._store_ptr);
            _idx = std::move(mv._idx);
            _keep = mv._keep;
            mv._keep = true;
        }

        T*
        operator->() noexcept { return _store_ptr->get(); }
        const T*
        operator->() const noexcept { return _store_ptr->get(); }

        T&
        operator*() noexcept { return *(_store_ptr->get()); }
        const T&
        operator*() const noexcept { return *(_store_ptr->get()); }

        ~ref_t() noexcept {
            if (!_keep) _buf->checked_remove(_idx);
        }

    private:
        friend mx_buffer;

        friend bool
        operator==(const ref_t& a, const ref_t& b) {
            return a._store_ptr == b._store_ptr;
        }
        friend constexpr auto
        operator<=>(const ref_t& a, const ref_t& b) {
            return a._store_ptr <=> b._store_ptr;
        }

        explicit ref_t(buffer_type* buf,
                       store<T>* ptr,
                       size_type idx)
             : _buf(buf),
               _store_ptr(ptr),
               _idx(idx) { }

        buffer_type* _buf;
        store<T>* _store_ptr;
        size_type _idx;
        bool _keep = false;
    };

    constexpr const static auto page_size = PageSize;
    constexpr const static auto elem_size = sizeof(store<T>);
    constexpr const static auto max_count = page_size / elem_size;

    constexpr auto
    capacity() const noexcept {
        return max_count;
    }

    [[nodiscard]] bool
    was_full() {
        std::scoped_lock lck(_read_mx);
        return std::ranges::all_of(_data, &store<T>::valid);
    }

    template<class U>
    [[nodiscard]] ref_t
    insert(U&& arg) {
        using std::begin, std::end;
        std::scoped_lock lck(_read_mx);
        auto free_store = std::ranges::find_if(_data, [](const auto& data_store) {
            return !data_store.valid();
        });
        if (free_store == end(_data)) return ref_t{};

        {
            std::scoped_lock wr_lck(_write_mx);
            free_store->construct(std::forward<U>(arg));
        }

        return ref_t(this, &*free_store, std::distance(begin(_data), free_store));
    }

    void
    remove(size_type idx) {
        std::scoped_lock lck(_read_mx, _write_mx);
        _data[idx].destruct();
    }

    void
    checked_remove(size_type idx) {
        std::scoped_lock lck(_read_mx, _write_mx);
        if (_data[idx].valid()) _data[idx].destruct();
    }

private:
    std::mutex _read_mx;
    std::mutex _write_mx;

    using store_type = store<T>;
    store_type _data[elem_size]{};
};


#endif
