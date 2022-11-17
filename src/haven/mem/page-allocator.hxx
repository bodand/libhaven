/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-12.
 *
 * benchmark/pool/common/page --
 */
#ifndef LIBHAVEN_PAGE_ALLOCATOR_HXX
#define LIBHAVEN_PAGE_ALLOCATOR_HXX

#include <concepts>
#include <cstddef>
#include <string_view>
#include <variant>

#ifdef HAVEN_DBG_PAGE_TRACE
#  include <iostream>
#  include <syncstream>
#  include <vector>
#endif

namespace hvn {
    template<class P>
    concept notational_memory_page =
           requires(P p) {
               P::name;
               { p.base_addr() } -> std::same_as<void*>;
               { p.size() } -> std::unsigned_integral;
           };

    template<class P>
    concept committed_memory_page =
           requires(P p) {
               P::name;
               { p.base_addr() } -> std::same_as<std::byte*>;
               { p.size() } -> std::unsigned_integral;
           };

    template<class P>
    concept memory_page = notational_memory_page<P>
                          || committed_memory_page<P>;

    template<class A>
    concept allocator =
           requires(A alloc) {
               requires memory_page<typename A::allocated_page>;
               requires memory_page<typename A::committed_page>;
               requires memory_page<typename A::loaned_page>;

               // allocate memory 2 in 1 ready to use
               { alloc.allocate(std::declval<std::size_t>()) } -> committed_memory_page;
               // reserve memory for future use
               { alloc.reserve(std::declval<std::size_t>()) } -> notational_memory_page;
               // commit memory for use
               { alloc.commit(std::declval<typename A::allocated_page>()) } -> committed_memory_page;
               { alloc.commit(std::declval<typename A::committed_page>()) } -> committed_memory_page;
               { alloc.commit(std::declval<typename A::loaned_page>()) } -> committed_memory_page;
               // decommit memory, but keep for future use
               { alloc.decommit(std::declval<typename A::committed_page>()) } -> notational_memory_page;
               // deallocate memory
               { alloc.deallocate(std::declval<typename A::committed_page>()) } -> std::same_as<void>;
               { alloc.deallocate(std::declval<typename A::allocated_page>()) } -> std::same_as<void>;
               { alloc.deallocate(std::declval<typename A::loaned_page>()) } -> std::same_as<void>;
               // maybe loan memory back to the os
               { alloc.loan(std::declval<typename A::committed_page>()) };
               { alloc.loan(std::declval<typename A::allocated_page>()) };

               // at least approximate L1 cache line size
               { alloc.approx_cache_line1() } -> std::unsigned_integral;
               // provide page size
               { alloc.page_size() } -> std::unsigned_integral;
           };

    struct page_allocator {
        struct allocated_page {
            constexpr const static auto name = std::string_view("allocated");

            [[nodiscard]] auto
            base_addr() const noexcept { return _base_addr; }

            [[nodiscard]] auto
            size() const noexcept { return _size; }

        private:
            void* _base_addr;
            std::size_t _size;

            allocated_page(void* base, std::size_t size)
                 : _base_addr(base),
                   _size(size) { }

            friend page_allocator;
        };

        struct committed_page {
            constexpr const static auto name = std::string_view("committed");

            [[nodiscard]] auto
            base_addr() const noexcept { return _base_addr; }

            [[nodiscard]] auto
            size() const noexcept { return _size; }

        private:
            std::byte* _base_addr;
            std::size_t _size;

            committed_page(std::byte* base, std::size_t size)
                 : _base_addr(base),
                   _size(size) { }
            friend page_allocator;
        };

        struct loaned_page {
            constexpr const static auto name = std::string_view("loaned");

            [[nodiscard]] auto
            base_addr() const noexcept { return _base_addr; }

            [[nodiscard]] auto
            size() const noexcept { return _size; }

        private:
            void* _base_addr;
            std::size_t _size;

            loaned_page(void* base, std::size_t size)
                 : _base_addr(base),
                   _size(size) { }
            friend page_allocator;
        };

        [[nodiscard]] static std::size_t
        page_size() { return figure_out_page_size(); }

        [[nodiscard]] static std::size_t
        approx_cache_line1();

        [[nodiscard]] allocated_page
        reserve(std::size_t wanted_size);

        [[nodiscard]] committed_page
        commit(allocated_page page);

        [[nodiscard]] committed_page
        commit(loaned_page page);

        [[nodiscard]] committed_page
        commit(committed_page page) const { return page; }

        [[nodiscard]] allocated_page
        decommit(committed_page page);

        [[nodiscard]] committed_page
        allocate(std::size_t size);

        void
        deallocate(committed_page page);
        void
        deallocate(allocated_page page);
        void
        deallocate(loaned_page page);

        [[nodiscard]] std::variant<loaned_page, allocated_page>
        loan(allocated_page page);

        [[nodiscard]] std::variant<loaned_page, committed_page>
        loan(committed_page page);

#ifdef HAVEN_DBG_PAGE_TRACE
        ~page_allocator() noexcept {
            try {
                std::osyncstream ostr(std::cerr);
                ostr << "page_allocator@" << static_cast<void*>(this) << "\n";
                ostr << "\tallocated pages: " << _count << "\n";
                if (_allocated.empty()) {
                    ostr << "\t.. all have been properly cleaned up ..\n";
                }
                else {
                    ostr << "\t!! the pages at the following starting address(es) have not been cleaned up !!\n";
                    for (const auto page : _allocated) {
                        ostr << "\t\t - " << page << "\n";
                    }
                }
            } catch (...) {
                // debug related logging failed, oh well
            }
        }
#endif

    private:
        template<memory_page P>
        friend std::ostream&
        operator<<(std::ostream& os, const P& page) {
            return os << P::name << " page@" << page.base_addr() << ":: width: " << page.size();
        }
        static std::size_t
        figure_out_page_size() noexcept;

        std::size_t _page_size = page_size();
#ifdef HAVEN_DBG_PAGE_TRACE
        std::size_t _count{};
        std::vector<const void*> _allocated{};
#endif
    };
    static_assert(allocator<page_allocator>, "hvn::page_allocator needs to be a hvn::allocator");
}

#endif
