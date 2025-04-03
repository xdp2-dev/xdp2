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

#ifndef XDP2GEN_METADATA_SPEC_H
#define XDP2GEN_METADATA_SPEC_H

#include <vector>
#include <variant>

#include <clang/Tooling/Tooling.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/RecordLayout.h>
#include <iosfwd>
#include <xdp2gen/program-options/log_handler.h>

namespace xdp2gen
{
namespace clang_ast
{

struct metadata_integer {
    std::size_t bit_size;

    friend inline std::ostream &operator<<(std::ostream &os, metadata_integer i)
    {
        return os << "metadata_integer [ " << i.bit_size << "]";
    }
};
struct metadata_field;
inline std::ostream &operator<<(std::ostream &os, metadata_field f);

struct metadata_record {
    bool is_union;
    std::vector<metadata_field> fields;
};
struct metadata_array {
    std::size_t size;
    std::variant<metadata_integer, metadata_record> type;
};
struct metadata_field {
    std::string name;
    std::variant<metadata_integer, metadata_record, metadata_array> type;
    std::size_t offset_in_record;
};
inline std::ostream &operator<<(std::ostream &os, metadata_record fs)
{
    os << "metadata_record [ is_union: " << fs.is_union << ", fields: [";
    int i = 0;
    for (auto &&f : fs.fields) {
        os << "\n[" << i++ << ": " << f << "],";
    }
    return os << "]\n]";
}
inline std::ostream &operator<<(std::ostream &os, metadata_array a)
{
    os << "metadata_array [ size: " << a.size;
    std::visit([&os](auto &&t) { os << ", type: " << t; }, a.type);
    return os << "]";
}
inline std::ostream &operator<<(std::ostream &os, metadata_field f)
{
    os << "metadata_field [ name: " << f.name;
    std::visit([&os](auto &&t) { os << ", type: " << t; }, f.type);
    os << " offset_in_record: " << f.offset_in_record;
    return os << "]";
}

inline metadata_record metadata_record_fields(clang::RecordDecl *record)
{
    metadata_record r{ record->isUnion(), {} };
    auto fields = record->fields();

    auto &loc_ast_context = record->getASTContext();
    const clang::ASTRecordLayout &ast_rec_layout =
        loc_ast_context.getASTRecordLayout(record);

    for (auto &&f : fields) {
        auto type = f->getType();
        auto name = f->getNameAsString();

        auto field_index = f->getFieldIndex();
        plog::log(std::cout) << "field index: " << field_index << std::endl;

        std::size_t offset_in_record =
            ast_rec_layout.getFieldOffset(field_index);
        plog::log(std::cout)
            << "offset_in_record: " << offset_in_record << std::endl;

        if (type->isIntegerType()) {
            plog::log(std::cout) << "is integer" << std::endl;
            auto type_info = record->getASTContext().getTypeInfo(type);
            r.fields.push_back({ name, metadata_integer{ type_info.Width },
                                 offset_in_record });
        } else if (type->isRecordType()) {
            auto inner = metadata_record_fields(type->getAsRecordDecl());
            r.fields.push_back({ name, inner, offset_in_record });
        } else if (type->isConstantArrayType()) {
            auto array_type = clang::dyn_cast<clang::ConstantArrayType>(type);
            if (array_type) {
                plog::log(std::cout)
                    << "is a constant array type " << name << std::endl;
                std::size_t size = array_type->getSize().getZExtValue();
                auto base_type = array_type->getElementType();

                plog::log(std::cout) << "base_type name is \n";
                if (plog::is_display_log())
                    base_type->dump();
                plog::log(std::cout) << std::endl;

                if (base_type->isIntegerType()) {
                    auto type_info =
                        record->getASTContext().getTypeInfo(base_type);
                    r.fields.push_back(
                        { name,
                          metadata_array{
                              size, { metadata_integer{ type_info.Width } } },
                          offset_in_record });
                } else if (base_type->isRecordType() &&
                           base_type->getAsRecordDecl()->isCompleteDefinition()) {
                    auto inner =
                        metadata_record_fields(base_type->getAsRecordDecl());
                    r.fields.push_back({ name,
                                         metadata_array{ size, { inner } },
                                         offset_in_record });

                } else {
                    plog::log(std::cout) << "a non-supported type" << std::endl;
                    type->dump();
                    std::abort();
                }
            }
        } else {
            plog::log(std::cout) << "a non-supported type" << std::endl;
            type->dump();
            std::abort();
        }
    }
    plog::log(std::cout) << "returning" << std::endl;
    return r;
}

std::size_t get_size(std::variant<metadata_integer, metadata_record> v);
std::size_t
get_size(std::variant<metadata_integer, metadata_record, metadata_array> v);

std::size_t get_size(metadata_array v)
{
    return v.size * get_size(v.type);
}
std::size_t get_size(metadata_integer v)
{
    return v.bit_size;
}
std::size_t get_size(metadata_record v)
{
    std::size_t size = 0;
    for (auto &&f : v.fields) {
        if (v.is_union)
            size = std::max(get_size(f.type), size);
        else
            size += get_size(f.type);
    }
    return size;
}

std::size_t get_size(std::variant<metadata_integer, metadata_record> v)
{
    return std::visit([](auto t) { return get_size(t); }, v);
}
std::size_t
get_size(std::variant<metadata_integer, metadata_record, metadata_array> v)
{
    return std::visit([](auto t) { return get_size(t); }, v);
}

std::optional<std::size_t>
get_const_array_index_from_name(std::string const &name)
{
    std::size_t first_idx = name.find("[", 0);
    std::size_t second_idx = name.find("]", first_idx);

    bool is_array_indicator_present = first_idx != std::string::npos &&
                                      second_idx != std::string::npos;

    bool does_index_is_present = is_array_indicator_present &&
                                 (second_idx - first_idx) > 1;

    if (does_index_is_present) {
        auto index_str_len = second_idx - first_idx - 1;

        std::string index = name.substr(first_idx + 1, index_str_len);

        auto it =
            std::find_if_not(index.begin(), index.end(), [](char const &c) {
                return c >= '0' && c <= '9';
            });

        bool is_index_integer_literal = (it == index.end());

        if (is_index_integer_literal)
            return { std::stoi(index) };
    }

    return {};
}

std::optional<std::pair<std::size_t, std::size_t>>
get_offset_size_from_field(metadata_record r,
                           std::vector<std::string> field_names);
std::optional<std::pair<std::size_t, std::size_t>>
get_offset_size_from_field(std::variant<metadata_integer, metadata_record> v,
                           std::vector<std::string> field_names);

std::optional<std::pair<std::size_t, std::size_t>>
get_offset_size_from_field(metadata_integer i,
                           std::vector<std::string> field_names)
{
    return std::make_pair(0ul, i.bit_size);
}

std::optional<std::pair<std::size_t, std::size_t>>
get_offset_size_from_field(metadata_array a,
                           std::vector<std::string> field_names)
{
    return get_offset_size_from_field(a.type, field_names);
}

std::optional<std::pair<std::size_t, std::size_t>>
get_offset_size_from_field(std::variant<metadata_integer, metadata_record> v,
                           std::vector<std::string> field_names)
{
    return std::visit(
        [field_names](auto &&p) {
            return get_offset_size_from_field(p, field_names);
        },
        v);
}

std::optional<std::pair<std::size_t, std::size_t>> get_offset_size_from_field(
    std::variant<metadata_integer, metadata_record, metadata_array> v,
    std::vector<std::string> field_names)
{
    return std::visit(
        [field_names](auto &&p) {
            return get_offset_size_from_field(p, field_names);
        },
        v);
}

std::string trim_record_name(std::string const &name)
{
    return { name.substr(0, name.find("[", 0)) };
}

std::optional<std::pair<std::size_t, std::size_t>>
get_offset_size_from_field(metadata_record r,
                           std::vector<std::string> field_names)
{
    if (field_names.empty())
        return {};
    assert(!field_names.empty());
    std::optional<std::pair<std::size_t, std::size_t>> result;
    std::size_t current_offset = 0;
    auto name = trim_record_name(field_names[0]);
    metadata_field found_f;
    for (auto &&f : r.fields) {
        current_offset = f.offset_in_record;

        plog::log(std::cout) << "comparing " << f.name << " with "
                             << field_names[0] << std::endl;
        if (!f.name.empty() && f.name == name) {
            plog::log(std::cout) << "found field" << std::endl;
            std::vector<std::string> new_names(std::next(field_names.begin()),
                                               field_names.end());

            if (new_names.empty()) {
                found_f = f;
                result = std::make_pair(current_offset, get_size(f.type));
                break;
            } else {
                result = get_offset_size_from_field(f.type, new_names);
                if (result) {
                    result->first += current_offset;
                }
                found_f = f;
                break;
            }
        } else if (f.name.empty()) {
            // try searching with anonymous records

            plog::log(std::cout) << "found empty field" << std::endl;
            result = get_offset_size_from_field(f.type, field_names);
            if (result) {
                result->first += current_offset;
                found_f = f;
                break;
            }
        }

        if (!r.is_union)
            current_offset += get_size(f.type);
    }

    if (result.has_value()) {
        if (std::holds_alternative<metadata_array>(found_f.type)) {
            std::optional<std::size_t> idx =
                get_const_array_index_from_name(field_names[0]);

            if (idx.has_value()) {
                auto type_size =
                    get_size(std::get<metadata_array>(found_f.type).type);

                result->first += idx.value() * type_size;
            }
        }

        return result;
    }

    return {};
}

std::optional<std::pair<std::size_t, std::size_t>>
get_offset_size_from_field(metadata_record r, std::string field_names)
{
    std::vector<std::string> names;
    std::size_t i = 0, j = field_names.find(".", i);
    while (i != std::string::npos) {
        plog::log(std::cout) << "substr [" << i << "][" << j << "]"
                             << field_names.substr(i, j) << std::endl;
        names.push_back(
            field_names.substr(i, j == std::string::npos ? j : j - i));
        i = (j == std::string::npos ? j : j + 1);
        j = field_names.find(".", i);
    }
    return get_offset_size_from_field(r, names);
}

}
}

#endif
