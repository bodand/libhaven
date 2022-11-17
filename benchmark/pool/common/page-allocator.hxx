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
#include <variant>

#ifdef HAVEN_DBG_PAGE_TRACE
#  include <iostream>
#  include <syncstream>
#  include <vector>
#endif

template<class P>
concept haven_notational_memory_page =
       requires(P p) {
           P::name;
           { p.base_addr } -> std::same_as<void* const&>;
           p.size;
           requires std::unsigned_integral<decltype(p.size)>;
           requires std::is_const_v<decltype(p.size)>;
       };

template<class P>
concept haven_committed_memory_page =
       requires(P p) {
           P::name;
           { p.base_addr } -> std::same_as<std::byte* const&>;
           p.size;
           requires std::unsigned_integral<decltype(p.size)>;
           requires std::is_const_v<decltype(p.size)>;
       };

template<class P>
concept haven_memory_page = haven_notational_memory_page<P>
                            || haven_committed_memory_page<P>;

template<class A>
concept haven_allocator =
       requires(A alloc) {
           requires haven_memory_page<typename A::allocated_page>;
           requires haven_memory_page<typename A::committed_page>;
           requires haven_memory_page<typename A::loaned_page>;

           // allocate memory 2 in 1 ready to use
           { alloc.allocate(std::declval<std::size_t>()) } -> haven_committed_memory_page;
           // reserve memory for future use
           { alloc.reserve(std::declval<std::size_t>()) } -> haven_notational_memory_page;
           // commit memory for use
           { alloc.commit(std::declval<typename A::allocated_page>()) } -> haven_committed_memory_page;
           { alloc.commit(std::declval<typename A::committed_page>()) } -> haven_committed_memory_page;
           { alloc.commit(std::declval<typename A::loaned_page>()) } -> haven_committed_memory_page;
           // decommit memory, but keep for future use
           { alloc.decommit(std::declval<typename A::committed_page>()) } -> haven_notational_memory_page;
           // deallocate memory
           { alloc.deallocate(std::declval<typename A::committed_page>()) } -> std::same_as<void>;
           { alloc.deallocate(std::declval<typename A::allocated_page>()) } -> std::same_as<void>;
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
        void* const base_addr;
        const std::size_t size;

    private:
        allocated_page(void* base, std::size_t size)
             : base_addr(base),
               size(size) { }
        friend page_allocator;
    };

    struct committed_page {
        constexpr const static auto name = std::string_view("committed");
        std::byte* const base_addr;
        const std::size_t size;

    private:
        committed_page(std::byte* base, std::size_t size)
             : base_addr(base),
               size(size) { }
        friend page_allocator;
    };

    struct loaned_page {
        constexpr const static auto name = std::string_view("loaned");
        void* const base_addr;
        const std::size_t size;

    private:
        loaned_page(void* base, std::size_t size)
             : base_addr(base),
               size(size) { }
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

    [[nodiscard]] std::variant<loaned_page, allocated_page>
    loan(allocated_page page);

    [[nodiscard]] std::variant<loaned_page, committed_page>
    loan(committed_page page);

#ifdef HAVEN_DBG_PAGE_TRACE
    ~page_allocator() noexcept {
        try {
            std::osyncstream ostr(std::cerr);
            ostr << static_cast<void*>(this) << "allocator::\n";
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
    template<haven_memory_page P>
    friend std::ostream&
    operator<<(std::ostream& os, const P& page) {
        return os << P::name << " page @ " << page.base_addr << " width: " << page.size;
    }
    static std::size_t
    figure_out_page_size() noexcept;

    std::size_t _page_size = page_size();
#ifdef HAVEN_DBG_PAGE_TRACE
    std::size_t _count{};
    std::vector<const void*> _allocated{};
#endif
};
static_assert(haven_allocator<page_allocator>);

#endif
