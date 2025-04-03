/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 SiXDP2 Inc.
 *
 * Authors: Felipe Magno de Almeida <felipe@expertise.dev>
 *          João Paulo Taylor Ienczak Zanette <joao.tiz@expertise.dev>
 *          Lucas Cavalcante de Sousa <lucas@expertise.dev>
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

#ifndef XDP2GEN_AST_CONSUMER_FACTORY_H
#define XDP2GEN_AST_CONSUMER_FACTORY_H

#include <iostream>
#include <optional>
#include <string>

#include <clang/Tooling/Tooling.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>

namespace xdp2gen
{
namespace ast_consumer
{

template <typename T>
struct frontend_factory_for_consumer : clang::tooling::FrontendActionFactory {
    std::unique_ptr<T> consumer;

    template <typename... Args>
    frontend_factory_for_consumer(Args &&...args)
    {
        consumer = std::make_unique<T>(std::forward<Args>(args)...);
    }

    frontend_factory_for_consumer()
    {
        consumer = std::make_unique<T>();
    }

    struct frontend : clang::SyntaxOnlyAction {
        std::unique_ptr<T> consumer;

        frontend(std::unique_ptr<T> consumer)
            : consumer(std::move(consumer))
        {
        }

        std::unique_ptr<clang::ASTConsumer>
        CreateASTConsumer(clang::CompilerInstance &ci,
                          ::llvm::StringRef InFile) override
        {
            return std::move(consumer);
        }
    };

    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<frontend>(std::move(consumer));
    }
};

}
}

#endif
