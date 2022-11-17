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
hvn::page_allocator::commit(page_allocator::loaned_page) {
    precondition()("this function shall not be called under the MinGW runtime"_msg,
                   [] { return false; });
    return {nullptr, 0};
}


std::variant<hvn::page_allocator::loaned_page,
             hvn::page_allocator::allocated_page>
hvn::page_allocator::loan(page_allocator::allocated_page page) {
    return page;
}

std::variant<hvn::page_allocator::loaned_page,
             hvn::page_allocator::committed_page>
hvn::page_allocator::loan(page_allocator::committed_page page) {
    return page;
}
