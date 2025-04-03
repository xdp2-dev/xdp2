/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 SiXDP2 Inc.
 *
 * Authors: Otávio Lucas Alves da Silva <otavio.silva@expertisesolutions.com.br>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef XDP2GEN_LLVM_UTIL_H
#define XDP2GEN_LLVM_UTIL_H

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace xdp2gen
{

template <typename... Ts>
struct args_size {
    static constexpr size_t value = sizeof...(Ts);
};

template <typename... Ts>
constexpr auto args_size_v = args_size<Ts...>::value;

template <size_t I, typename... Ts>
struct select_type {
    using type = typename std::tuple_element<I, std::tuple<Ts...>>::type;
};

template <size_t I, typename... Ts>
using select_type_t = typename select_type<I, Ts...>::type;

template <typename T, typename... U>
struct index_of;

template <typename T, typename... Ts>
struct index_of<T, T, Ts...> {
    static constexpr size_t value = 0;
};

template <typename T, typename U, typename... Ts>
struct index_of<T, U, Ts...> {
    static constexpr size_t value = 1 + index_of<T, Ts...>::value;
};

template <typename T, typename... Ts>
constexpr auto index_of_v = index_of<T, Ts...>::value;

template <typename Test, typename... Args>
struct one_of : std::disjunction<std::is_same<Test, Args>...> {};

template <typename Test, typename... Args>
constexpr auto one_of_v = one_of<Test, Args...>::value;

}

#endif
