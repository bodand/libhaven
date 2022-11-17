/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-16.
 *
 * src/haven/common/check_conditions --
 *   Source file for the condition file sources.
 *   Mostly empty for templates are used.
 */

#include "check_conditions.hxx"

hvn::message_type hvn::literals::operator""_msg(const char* str, std::size_t str_sz) {
    return {std::string_view{str, str_sz}};
}
