/* libhaven project
 *
 * Copyright (c) 2022 András Bodor
 * All rights reserved.
 *
 * Originally created: 2022-11-16.
 *
 * src/haven/common/check_conditions --
 *   Generic class for pre- and postcondition checking.
 *   Used by libhaven to provide debugging support; their execution can be configured by
 *   HAVEN_DBG_CHECK_* macros.
 *   See the central CMakeLists.txt for possible options.
 */
#ifndef LIBHAVEN_CHECK_CONDITIONS_HXX
#define LIBHAVEN_CHECK_CONDITIONS_HXX

#include <concepts>
#include <functional>
#include <iostream>
#include <source_location>
#include <string_view>
#include <syncstream>

namespace hvn {
    template<class P>
    concept printable = requires(const P& p, std::ostream& os) {
                            { os << p } -> std::convertible_to<std::ostream&>;
                        };

    template<bool Post>
    struct is_postcondition {
        constexpr const static auto value = Post;

        static constexpr auto
        is_post() noexcept { return value; }
        static constexpr auto
        is_pre() noexcept { return !value; }
    };

    template<class X>
    concept condition_type_marker =
           (std::same_as<is_postcondition<true>, X>
            || std::same_as<is_postcondition<false>, X>)
           && requires {
                  { X::is_post() } -> std::convertible_to<bool>;
                  { X::is_pre() } -> std::convertible_to<bool>;
              };

    struct message_type {
        std::string_view data;

    private:
        friend std::ostream&
        operator<<(std::ostream& os, const message_type& msg) {
            return os << msg.data;
        }
    };

    inline namespace literals {
        message_type operator""_msg(const char* str, std::size_t str_sz);
    }

    template<condition_type_marker Type>
    struct condition_impl {
        constexpr explicit condition_impl(std::source_location src = std::source_location::current())
             : _pos(src) { }

        template<class Fn, class... Args>
            requires(std::predicate<Fn, Args...>)
        constexpr void
        operator()(Fn&& fn, Args&&... args) const noexcept(std::is_nothrow_invocable_r_v<bool, Fn, Args...>) {
            (*this)("unnamed-condition"_msg, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }

        template<class Fn, class... Args>
            requires(std::predicate<Fn, Args...>)
        constexpr void
        operator()(message_type msg, Fn&& fn, Args&&... args) const noexcept(std::is_nothrow_invocable_r_v<bool, Fn, Args...>) {
            using namespace std::literals;
            check_condition<do_pre_checks(), HAVEN_DBG_CHECK_PRE_PARAMS_V>("precondition failed: "sv,
                                                                           msg,
                                                                           std::forward<Fn>(fn),
                                                                           std::forward<Args>(args)...);
            check_condition<do_post_checks(), HAVEN_DBG_CHECK_POST_PARAMS_V>("postcondition failed: "sv,
                                                                             msg,
                                                                             std::forward<Fn>(fn),
                                                                             std::forward<Args>(args)...);
        }

    private:
        std::source_location _pos;

        template<bool do_check, bool do_param_print, printable M, class Fn, class... Args>
            requires(std::predicate<Fn, Args...>)
        void check_condition(std::string_view cond_msg,
                             M&& msg,
                             Fn&& fn,
                             Args&&... args) const {
            if constexpr (do_check) {
                if (std::invoke(std::forward<Fn>(fn),
                                std::forward<Args>(args)...)) {
                    return;
                }
                print_failure<do_param_print>(cond_msg,
                                              std::forward<M>(msg),
                                              std::forward<Args>(args)...);
                std::abort();
            }
        }

        template<bool do_params, printable M, class... Args>
        void
        print_failure(std::string_view static_msg,
                      M&& msg,
                      Args&&... args) const {
            std::osyncstream sync_err(std::cerr);
            sync_err << "ABORT::\n"
                     << _pos.file_name() << ":" << _pos.line() << ":" << _pos.column() << " in function " << _pos.function_name() << ":\n"
                     << static_msg << std::forward<M>(msg) << "\n";
            if constexpr (do_params
                          && (printable<Args> && ...)) {
                sync_err << "parameters:\n";
                ([&sync_err]<class Arg>(Arg&& arg) {
                    sync_err << "\t- " << std::forward<Arg>(arg) << "\n";
                }(args),
                 ...);
            }
        }

        static consteval auto
        do_post_checks() noexcept(noexcept(Type::is_post() && HAVEN_DBG_CHECK_POST_V)) {
            return Type::is_post() && HAVEN_DBG_CHECK_POST_V;
        }

        static consteval auto
        do_pre_checks() noexcept(noexcept(Type::is_pre() && HAVEN_DBG_CHECK_PRE_V)) {
            return Type::is_pre() && HAVEN_DBG_CHECK_PRE_V;
        }
    };
}

using precondition = hvn::condition_impl<hvn::is_postcondition<false>>;
using postcondition = hvn::condition_impl<hvn::is_postcondition<true>>;

#endif
