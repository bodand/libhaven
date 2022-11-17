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
    precondition("this function shall not be called under the MinGW runtime"_msg,
                 [] { return false; });
    return {nullptr, 0};
}


std::variant<page_allocator::loaned_page,
             page_allocator::allocated_page>
page_allocator::loan(page_allocator::allocated_page page) {
    return page;
}

std::variant<page_allocator::loaned_page,
             page_allocator::committed_page>
page_allocator::loan(page_allocator::committed_page page) {
    return page;
}
