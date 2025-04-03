// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2022 SiXDP2 Inc.
 *
 * Authors: Felipe Magno de Almeida <felipe@expertise.dev>
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

#ifndef XDP2GEN_JSON_METADATA_H
#define XDP2GEN_JSON_METADATA_H

#include <nlohmann/json.hpp>
#include <xdp2gen/clang-ast/metadata_spec.h>

namespace xdp2gen
{
namespace json
{

nlohmann::ordered_json
to_json(std::variant<clang_ast::metadata_integer, clang_ast::metadata_record,
                     clang_ast::metadata_array>
            v);
nlohmann::ordered_json to_json(
    std::variant<clang_ast::metadata_integer, clang_ast::metadata_record> v);
nlohmann::ordered_json to_json(clang_ast::metadata_record r);
nlohmann::ordered_json to_json(clang_ast::metadata_record r);
nlohmann::ordered_json to_json(clang_ast::metadata_integer r)
{
    nlohmann::ordered_json json{ { "size", r.bit_size } };
    return json;
}
nlohmann::ordered_json to_json(clang_ast::metadata_array r)
{
    nlohmann::ordered_json json{ { "array_size", r.size },
                                 { "array_type", json::to_json(r.type) } };
    return json;
}
nlohmann::ordered_json to_json(clang_ast::metadata_record r)
{
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    if (r.is_union)
        json["is_union"] = r.is_union;
    json.push_back({ "fields", nlohmann::ordered_json::array() });

    for (auto &&field : r.fields) {
        auto f =
            std::visit([](auto &&v) { return json::to_json(v); }, field.type);
        f["name"] = field.name;
        json["fields"].push_back(f);
    }

    return json;
}

nlohmann::ordered_json
to_json(std::variant<clang_ast::metadata_integer, clang_ast::metadata_record,
                     clang_ast::metadata_array>
            v)
{
    return std::visit([](auto &&t) { return json::to_json(t); }, v);
}
nlohmann::ordered_json
to_json(std::variant<clang_ast::metadata_integer, clang_ast::metadata_record> v)
{
    return std::visit([](auto &&t) { return json::to_json(t); }, v);
}

}
}

#endif
