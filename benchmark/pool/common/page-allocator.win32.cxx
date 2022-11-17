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

page_allocator::committed_page
page_allocator::commit(page_allocator::loaned_page page) {

    auto status = ReclaimVirtualMemory(page.base_addr,
                                       page.size);
    switch (status) {
    case ERROR_SUCCESS:
    case ERROR_BUSY:
        // memory reclaimed :)
        break;
    default:
        throw std::bad_alloc{};
    }

    return {static_cast<std::byte*>(page.base_addr), page.size};
}


std::variant<page_allocator::loaned_page,
             page_allocator::allocated_page>
page_allocator::loan(page_allocator::allocated_page page) {
    auto succ = OfferVirtualMemory(page.base_addr,
                                   page.size,
                                   VmOfferPriorityVeryLow);
    if (succ != ERROR_SUCCESS)
        return page;

    return loaned_page{page.base_addr, page.size};
}

std::variant<page_allocator::loaned_page,
             page_allocator::committed_page>
page_allocator::loan(page_allocator::committed_page page) {
    auto succ = OfferVirtualMemory(page.base_addr,
                                   page.size,
                                   VmOfferPriorityVeryLow);
    if (succ != ERROR_SUCCESS)
        return page;

    return loaned_page{page.base_addr, page.size};
}
