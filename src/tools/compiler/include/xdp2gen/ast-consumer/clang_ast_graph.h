// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2024 SiXDP2 Inc.
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

#ifndef XDP2GEN_AST_CONSUMER_CLANG_AST_GRAPH_H
#define XDP2GEN_AST_CONSUMER_CLANG_AST_GRAPH_H

#include <boost/range.hpp>
#include <iostream>
#include <span>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>

#include "xdp2gen/util.h"

namespace xdp2gen
{
namespace ast_consumer
{
class clang_ast_graph {
public:
    using node_type = void const *;
    using attr_type =
        std::variant<::clang::Decl const *, ::clang::Stmt const *>;

private:
    static constexpr size_t npos = -1;

    size_t curr_size = 0;
    std::vector<attr_type> index_node_map;
    std::unordered_map<node_type, size_t> ptr_index_map;
    std::vector<std::tuple<std::vector<size_t>, std::vector<size_t>>> adj_list;

    template <typename N>
        requires std::is_base_of_v<::clang::Stmt, N> ||
                 std::is_base_of_v<::clang::Decl, N>
    size_t __increase_graph(node_type const &ptr, N const *n)
    {
        index_node_map.push_back(n);
        ptr_index_map.insert({ ptr, curr_size });
        adj_list.push_back({ {}, {} });

        return curr_size++;
    }

    template <typename N>
    std::pair<node_type, size_t> __search_and_insert(N const *n)
        requires std::is_base_of_v<::clang::Stmt, N> ||
                 std::is_base_of_v<::clang::Decl, N>
    {
        if (auto it = ptr_index_map.find(n); it != ptr_index_map.end()) {
            return *it;
        }
        auto idx = __increase_graph(n, n);

        return { n, idx };
    }

    bool __add_edge(size_t source, size_t target)
    {
        if (curr_size == npos) {
            return false;
        }
        if (!(source < curr_size && target < curr_size)) {
            std::cerr << "source, target, curr_size: " << source << ", "
                      << target << ", " << curr_size << std::endl;
        }
        assert(source < curr_size && target < curr_size);

        std::get<0>(adj_list[source]).push_back(target);
        std::get<1>(adj_list[target]).push_back(source);
        /// TODO: maybe insert sorted to avoid repetition?
        return true;
    }

    size_t increase_stmt(::clang::Stmt const *n, bool insert_adj = false)
    {
        if (auto const *ice = ::clang::dyn_cast<::clang::ImplicitCastExpr>(n)) {
            auto children = ice->children();
            if (std::distance(children.begin(), children.end()) == 1) {
                return increase_stmt(*children.begin(), insert_adj);
            } else {
                assert(!"ImplicitCastExpr should not have more than one child");
            }
        } else {
            auto [raw_ptr, idx] = __search_and_insert(n);
            if (insert_adj) {
                for (auto const child : n->children()) {
                    if (child != nullptr) {
                        __add_edge(idx, increase(child, insert_adj));
                    }
                }
            }

            return idx;
        }
    }

    size_t increase(::clang::Stmt const *stmt, bool insert_adj = false)
    {
        // std::cout << "Inserting Stmt of type " << stmt->getStmtClassName() << std::endl;
        auto const idx = increase_stmt(stmt, insert_adj);
        if (auto const *n = ::clang::dyn_cast<::clang::DeclStmt>(stmt)) {
            if (insert_adj) {
                for (auto const *decl : n->getDeclGroup()) {
                    __add_edge(idx, increase(decl, insert_adj));
                }
            }
        }

        return idx;
    }

    size_t increase_decl(::clang::Decl const *n, bool insert_adj = false)
    {
        auto [raw_ptr, idx] = __search_and_insert(n);

        return idx;
    }

    size_t increase(::clang::Decl const *decl, bool insert_adj = false)
    {
        // std::cout << "Inserting Decl of type " << decl->getDeclKindName() << std::endl;
        auto const idx = increase_decl(decl, insert_adj);
        if (auto const *fd = ::clang::dyn_cast<::clang::FunctionDecl>(decl)) {
            if (insert_adj) {
                auto const num_params = fd->getNumParams();
                for (unsigned i = 0; i < num_params; ++i) {
                    __add_edge(idx, increase(fd->getParamDecl(i), insert_adj));
                }
                if (fd->hasBody()) {
                    __add_edge(idx, increase(fd->getBody(), insert_adj));
                }
            }
        } else if (auto const *vd = ::clang::dyn_cast<::clang::VarDecl>(decl)) {
            // if (insert_adj && vd->hasInit()) {
            //     __add_edge(idx, increase(vd->getInit(), insert_adj));
            // }
        } else if (auto const *rd =
                       ::clang::dyn_cast<::clang::RecordDecl>(decl)) {
            if (insert_adj) {
                auto fields = rd->fields();
                for (auto const field : fields) {
                    __add_edge(idx, increase(field, insert_adj));
                }
            }
        }

        return idx;
    }

public:
    clang_ast_graph(::clang::ASTContext const &ast_context)
    {
        auto decls = ast_context.getTranslationUnitDecl()->decls();
        auto i = 0;
        for (auto const decl : decls) {
            std::cout << "decl #" << i++ << std::endl;
            std::cout << "curr_size: " << curr_size << std::endl;
            increase(decl, true);
        }
        std::cout << "Finished building clang ast\n";
    }

    clang_ast_graph(::clang::Decl const *decl)
    {
        increase(decl, true);
    }

    auto adj_nodes(size_t i) const
    {
        assert(i < curr_size);

        return boost::make_iterator_range(std::get<0>(adj_list[i]).cbegin(),
                                          std::get<0>(adj_list[i]).cend());
    }

    auto inv_adj_nodes(size_t i) const
    {
        assert(i < curr_size);

        return boost::make_iterator_range(std::get<1>(adj_list[i]).cbegin(),
                                          std::get<1>(adj_list[i]).cend());
    }

    size_t size() const
    {
        return curr_size;
    }

    attr_type const &attrs(size_t i) const
    {
        assert(i < curr_size);

        return index_node_map.at(i);
    }
};

auto get_size(xdp2gen::ast_consumer::clang_ast_graph const &g) -> size_t
{
    return g.size();
}

auto get_adj_list(xdp2gen::ast_consumer::clang_ast_graph const &g, size_t i)
{
    return g.adj_nodes(i);
}

auto get_inv_adj_list(xdp2gen::ast_consumer::clang_ast_graph const &g,
                      size_t i)
{
    return g.inv_adj_nodes(i);
}

auto get_attrs(xdp2gen::ast_consumer::clang_ast_graph const &g, size_t i)
    -> xdp2gen::ast_consumer::clang_ast_graph::attr_type const &
{
    return g.attrs(i);
}

}
}

#endif
