/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-17.
 *
 * test/common/preconditions --
 */

#include <boost/ut.hpp>
#include <haven/common/check_conditions.hxx>

using namespace boost::ut;

[[maybe_unused]] const suite preconditions_suite = [] {
    "precondition can be called nullary function"_test = [] {
        precondition()([] { return true; });
        expect(true) << "precondition aborted";
    };

    "precondition can be passed arguments, if passed function has non-zero arity"_test = [] {
        precondition()(
               [](auto x) {
                   expect(that % x == 42);
                   return true;
               },
               42);
        expect(true) << "precondition aborted";
    };

    "precondition can be passed a message_type buffer with nullary function"_test = [] {
        using namespace hvn::literals;
        precondition()("custom abort message"_msg, [] { return true; });
        expect(true) << "precondition aborted";
    };

    "precondition can be passed arguments, if passed function has non-zero arity"_test = [] {
        using namespace hvn::literals;
        precondition()(
               "custom abort message"_msg,
               [](auto x) {
                   expect(that % x == 42);
                   return true;
               },
               42);
        expect(true) << "precondition aborted";
    };

#if !defined(_WIN32) && !defined(_WIN64)
    // abortion checks only available on fork-compatible platforms, ie. non-Windows
    "precondition aborts on false check"_test = [] {
        expect(aborts([] {
            precondition([] { return false; });
        }));
    };
#else
    skip / "precondition aborts on false check"_test = [] {
        /*nop*/
    };
#endif
};
