/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-12.
 *
 * benchmark/pool/common/page --
 */

#include "page-allocator.hxx"

#include <algorithm>
#include <cassert>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

#include "../common/check_conditions.hxx"

hvn::page_allocator::committed_page
hvn::page_allocator::commit(page_allocator::loaned_page page) {
    precondition()([](auto addr) { return addr != nullptr; }, page.base_addr());
    precondition()([](auto size) { return size != 0; }, page.size());

    auto status = ReclaimVirtualMemory(page.base_addr(),
                                       page.size());
    switch (status) {
    case ERROR_SUCCESS:
    case ERROR_BUSY:
        // memory reclaimed :)
        // we don't, in any case, try to reuse this memory anyway
        break;
    default:
        throw std::bad_alloc{};
    }

    return {static_cast<std::byte*>(page.base_addr()), page.size()};
}


std::variant<hvn::page_allocator::loaned_page,
             hvn::page_allocator::allocated_page>
hvn::page_allocator::loan(page_allocator::allocated_page page) {
    auto succ = OfferVirtualMemory(page.base_addr(),
                                   page.size(),
                                   VmOfferPriorityLow);
    if (succ != ERROR_SUCCESS)
        return page;

    return loaned_page{page.base_addr(), page.size()};
}

std::variant<hvn::page_allocator::loaned_page,
             hvn::page_allocator::committed_page>
hvn::page_allocator::loan(page_allocator::committed_page page) {
    auto succ = OfferVirtualMemory(page.base_addr(),
                                   page.size(),
                                   VmOfferPriorityLow);
    if (succ != ERROR_SUCCESS)
        return page;

    return loaned_page{page.base_addr(), page.size()};
}
