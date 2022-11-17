/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-17.
 *
 * test/common/postconditions --
 */

#include <boost/ut.hpp>
#include <haven/common/check_conditions.hxx>

using namespace boost::ut;

[[maybe_unused]] const suite postconditions_suite = [] {
    "postcondition can be called nullary function"_test = [] {
        postcondition()([] { return true; });
        expect(true) << "postcondition aborted";
    };

    "postcondition can be passed arguments, if passed function has non-zero arity"_test = [] {
        postcondition()(
               [](auto x) {
                   expect(that % x == 42);
                   return true;
               },
               42);
        expect(true) << "postcondition aborted";
    };

    "postcondition can be passed a message_type buffer with nullary function"_test = [] {
        using namespace hvn::literals;
        postcondition()("custom abort message"_msg, [] { return true; });
        expect(true) << "postcondition aborted";
    };

    "postcondition can be passed arguments, if passed function has non-zero arity"_test = [] {
        using namespace hvn::literals;
        postcondition()(
               "custom abort message"_msg,
               [](auto x) {
                   expect(that % x == 42);
                   return true;
               },
               42);
        expect(true) << "postcondition aborted";
    };

#if !defined(_WIN32) && !defined(_WIN64)
    // abortion checks only available on fork-compatible platforms, ie. non-Windows
    "postcondition aborts on false check"_test = [] {
        expect(aborts([] {
            postcondition([] { return false; });
        }));
    };
#else
    skip / "postcondition aborts on false check"_test = [] {
        /*nop*/
    };
#endif
};
