/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-12.
 *
 * benchmark/pool/common/page --
 */

#include <algorithm>
#include <cassert>

#include <page-allocator.hxx>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include "check_conditions.hxx"

std::size_t
page_allocator::figure_out_page_size() noexcept {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    auto ret = sys_info.dwPageSize;

    postcondition("page size is zero"_msg, [ret] { return ret > 0; });
    return ret;
}

page_allocator::allocated_page
page_allocator::reserve(std::size_t size) {
    precondition([](auto wanted_size,
                    auto page_size) { return wanted_size % page_size == 0; },
                 size,
                 _page_size);

    LPVOID memory = VirtualAlloc(nullptr,
                                 size,
                                 MEM_RESERVE,
                                 PAGE_READWRITE);
    if (!memory) throw std::bad_alloc{};

#ifdef HAVEN_DBG_PAGE_TRACE
    _allocated.push_back(memory);
    _count++;
#endif

    postcondition([](auto mem) { return mem != nullptr; }, memory);
    return {memory, size};
}

page_allocator::committed_page
page_allocator::commit(page_allocator::allocated_page page) {
    precondition([](auto page) { return page.base_addr != nullptr; },
                 page);
    precondition([](auto wanted_size,
                    auto page_size) { return wanted_size % page_size == 0; },
                 page.size,
                 _page_size);

    LPVOID memory = VirtualAlloc(page.base_addr,
                                 page.size,
                                 MEM_COMMIT,
                                 PAGE_READWRITE);
    if (!memory) throw std::bad_alloc{};

    postcondition([](auto mem) { return mem != nullptr; }, memory);
    postcondition([&page](auto mem) { return mem == page.base_addr; }, memory);
    return {static_cast<std::byte*>(memory), page.size};
}

page_allocator::allocated_page
page_allocator::decommit(page_allocator::committed_page page) {
    precondition([](auto addr) { return addr != nullptr; }, page.base_addr);
    precondition([](auto size) { return size > 0; }, page.size);

    VirtualFree(page.base_addr,
                page.size,
                MEM_DECOMMIT);

    return {page.base_addr, page.size};
}

namespace {
    void
    decommit_release(void* addr,
                     std::size_t size) {
        precondition([](auto addr) { return addr != nullptr; }, addr);
        precondition([](auto size) { return size > 0; }, size);

        auto succ = VirtualFree(addr,
                                size,
                                MEM_DECOMMIT | MEM_RELEASE);

        postcondition([](bool succ, auto) { return succ; }, succ, GetLastError());
    }

    void
    decommit_release(void* addr,
                     std::size_t size,
                     std::vector<const void*>& alloc) {
        decommit_release(addr, size);
        std::erase(alloc, addr);

        postcondition([&alloc, &addr]() {
            return std::ranges::none_of(alloc, [&addr](auto x) { return x == addr; });
        });
    }
}

#ifdef HAVEN_DBG_PAGE_TRACE
#  define ALLOCATED_PARAM , _allocated
#else
#  define ALLOCATED_PARAM
#endif

void
page_allocator::deallocate(page_allocator::committed_page page) {
    decommit_release(page.base_addr,
                     page.size
                            ALLOCATED_PARAM);
}

void
page_allocator::deallocate(page_allocator::allocated_page page) {
    decommit_release(page.base_addr,
                     page.size
                            ALLOCATED_PARAM);
}

page_allocator::committed_page
page_allocator::allocate(std::size_t size) {
    assert(size % _page_size == 0);

    LPVOID memory = VirtualAlloc(nullptr,
                                 size,
                                 MEM_RESERVE | MEM_COMMIT,
                                 PAGE_READWRITE);
    if (!memory) throw std::bad_alloc{};

#ifdef HAVEN_DBG_PAGE_TRACE
    _allocated.push_back(memory);
    _count++;
#endif

    return {static_cast<std::byte*>(memory), size};
}

std::size_t
page_allocator::approx_cache_line1() {
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buf[256];
    DWORD buf_sz = std::size(buf);
    GetLogicalProcessorInformationEx(RelationCache,
                                     buf,
                                     &buf_sz);

    for (int i = 0; i < buf_sz; ++i) {
        if (buf[i].Relationship == RelationCache) {
            [[likely]] return buf[i].Cache.LineSize;
        }
    }
    return std::size_t{64}; // approx.
}
