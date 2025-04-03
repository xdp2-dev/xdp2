/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 SiXDP2 Inc.
 *
 * Authors: Pedro Daniel <pedro.daniel@expertisesolutions.com.br>
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

#ifndef XDP2GEN_AST_CONSUMER_FLAG_FIELDS_H_
#define XDP2GEN_AST_CONSUMER_FLAG_FIELDS_H_

// Standard
// - Io manip
#include <iostream>
// - Data manip
#include <optional>
#include <vector>
// - Data types
#include <string>
#include <linux/types.h>

// Clang
#include <clang/Tooling/Tooling.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <xdp2gen/program-options/log_handler.h>

struct flag_field {
    unsigned int flag;
    std::optional<unsigned int> mask;
    size_t size;

    friend inline std::ostream &operator<<(std::ostream &os,
                                           flag_field const &data)
    {
        os << "------------FLAG FIELD DATA------------"
           << "\n";
        os << "flag: " << data.flag << "\n";
        if (data.mask.has_value())
            os << "mask: " << data.mask.value() << "\n";
        os << "size: " << data.size << "\n";
        os << "-----------------END-------------------";

        return os;
    }
};

struct xdp2_flag_fields_extract_data {
    std::string name;
    std::optional<size_t> num_idx;
    std::vector<flag_field> fields;

    friend inline std::ostream &
    operator<<(std::ostream &os, xdp2_flag_fields_extract_data const &data)
    {
        os << "------------FLAG FIELDS DATA-----------"
           << "\n";
        os << "name: " << data.name << "\n";
        if (data.num_idx.has_value())
            os << "num_idx: " << data.num_idx.value() << "\n";
        for (auto &&flag_field : data.fields)
            os << flag_field << "\n";

        os << "-----------------END-------------------";

        return os;
    }
};

class xdp2_flag_fields_consumer : public clang::ASTConsumer {
private:
    std::vector<xdp2_flag_fields_extract_data> &consumed_data;

public:
    xdp2_flag_fields_consumer(
        std::vector<xdp2_flag_fields_extract_data> &consumed_data)
        : consumed_data{ consumed_data }
    {
    }

    virtual bool HandleTopLevelDecl(clang::DeclGroupRef D) override
    {
        if (D.isSingleDecl()) {
            auto decl = D.getSingleDecl();

            if (decl->getKind() == clang::Decl::Var) {
                auto var_decl = clang::dyn_cast<clang::VarDecl>(decl);
                auto type = var_decl->getType().getAsString();

                if (type == "const struct xdp2_flag_fields" &&
                    var_decl->hasInit()) {
                    plog::log(std::cout) << " TYPE DECL: " << type << std::endl;
                    plog::log(std::cout)
                        << " DECL NAME: " << var_decl->getNameAsString()
                        << std::endl;

                    // Extracts current decl name from proto node structure
                    std::string flag_field_decl_name =
                        var_decl->getNameAsString();

                    clang::Expr *initializer_expr = var_decl->getInit();

                    if (initializer_expr->getStmtClass() ==
                        clang::Stmt::InitListExprClass) {
                        // Extracts current analised InitListExpr
                        clang::InitListExpr *initializer_list_expr =
                            clang::dyn_cast<clang::InitListExpr>(
                                initializer_expr);

                        // Extracts current analised InitListDecl
                        clang::RecordDecl *initializer_list_decl =
                            initializer_list_expr->getType()
                                ->getAs<clang::RecordType>()
                                ->getDecl();

                        // Proto node consumed infos
                        xdp2_flag_fields_extract_data flag_field_data;

                        handle_init_list_expr(initializer_list_expr,
                                              initializer_list_decl,
                                              flag_field_data);

                        // Set the decl name of proto node structure
                        flag_field_data.name = flag_field_decl_name;

                        const auto is_empty_node =
                            [&flag_field_data]() -> bool {
                            return flag_field_data.fields.empty() &&
                                   flag_field_data.num_idx ==
                                       flag_field_data.fields.size();
                        };

                        // Inserts node data in result vector
                        if (!is_empty_node()) {
                            consumed_data.push_back(flag_field_data);

                            plog::log(std::cout)
                                << consumed_data.back() << std::endl;
                        }
                    }
                }
            }
        }

        return true;
    }

    void handle_init_list_expr(clang::InitListExpr *initializer_list_expr,
                               clang::RecordDecl *records,
                               xdp2_flag_fields_extract_data &flag_field_data)
    {
        // Iterates trougth all Expr in init list
        for (size_t i = 0; i < initializer_list_expr->getNumInits(); ++i) {
            // Current analised Expr
            clang::Expr *value = initializer_list_expr->getInits()[i];

            // Current analised Decl
            auto record = std::next(records->field_begin(), i);

            plog::log(std::cout)
                << "Class: " << value->getStmtClassName() << std::endl;

            // Ignore clang internal casts
            value = value->IgnoreImplicitAsWritten();

            if (value->getStmtClass() == clang::Stmt::IntegerLiteralClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** INTEGRAL EXPR REACHED ****" << std::endl;

                clang::IntegerLiteral *value_unary =
                    clang::dyn_cast<clang::IntegerLiteral>(value);

                // Get current analised field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - INTEGRAL NAME: " << field_name << std::endl;

                auto v = value->getIntegerConstantExpr(record->getASTContext());

                plog::log(std::cout)
                    << "    - INTEGRAL EVALUATED VALUE: " << v->getExtValue()
                    << std::endl;

                if (field_name == "num_idx") {
                    auto v =
                        value->getIntegerConstantExpr(record->getASTContext());
                    if (v) {
                        plog::log(std::cout)
                            << "    - INTEGRAL EVALUATED VALUE: "
                            << v->getZExtValue() << std::endl;

                        flag_field_data.num_idx = v->getZExtValue();
                        ;

                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE INTEGRAL EXPR"
                            << std::endl;
                    }
                }

            }
            // In case of Unary Expr be found
            else if (value->getStmtClass() ==
                     clang::Stmt::UnaryExprOrTypeTraitExprClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** UNARY EXPR REACHED ****" << std::endl;

                clang::UnaryExprOrTypeTraitExpr *value_unary =
                    clang::dyn_cast<clang::UnaryExprOrTypeTraitExpr>(value);

                // Get current analised field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - UNARY NAME: " << field_name << std::endl;

                if (field_name == "size") {
                    auto v =
                        value->getIntegerConstantExpr(record->getASTContext());
                    if (v) {
                        plog::log(std::cout) << "    - UNARY EVALUATED VALUE: "
                                             << v->getZExtValue() << std::endl;

                        if (!flag_field_data.fields.empty()) {
                            flag_field_data.fields.back().size =
                                v->getZExtValue() * CHAR_BIT;
                        }

                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE UNARY EXPR"
                            << std::endl;
                    }
                }

            } else if (value->getStmtClass() == clang::Stmt::ParenExprClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "*** PAREN EXPR REACHED ***" << std::endl;

                // Get current analised field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - PAREN NAME: " << field_name << std::endl;

                // In case of name be found
                if (field_name == "flag") {
                    auto v =
                        value->getIntegerConstantExpr(record->getASTContext());
                    if (v) {
                        plog::log(std::cout) << "    - PAREN EVALUATED VALUE: "
                                             << v->getZExtValue() << std::endl;

                        if (!flag_field_data.fields.empty()) {
                            unsigned int tmp_flag = v->getZExtValue();

                            plog::log(std::cout)
                                << "tmp_flag sizeof " << sizeof(tmp_flag)
                                << " int const bitwidth " << v->getBitWidth()
                                << std::endl;

                            //apparently the information comes in big endian codification so we need invert
                            std::reverse(static_cast<char *>(
                                             static_cast<void *>(&tmp_flag)),
                                         static_cast<char *>(
                                             static_cast<void *>(&tmp_flag)) +
                                             sizeof(tmp_flag));

                            if (sizeof(tmp_flag) * CHAR_BIT >
                                v->getBitWidth()) {
                                plog::log(std::cout)
                                    << "right bitshifting" << std::endl;

                                tmp_flag =
                                    (tmp_flag >> (sizeof(tmp_flag) * CHAR_BIT -
                                                  v->getBitWidth()));
                            }

                            plog::log(std::cout)
                                << "inverted flag " << tmp_flag << std::endl;

                            flag_field_data.fields.back().flag = tmp_flag;
                        }

                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE PAREN EXPR"
                            << std::endl;
                    }
                }
            } else if (value->getStmtClass() ==
                       clang::Stmt::InitListExprClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** INIT LIST EXPR REACHED ****" << std::endl;

                clang::InitListExpr *value_list =
                    clang::dyn_cast<clang::InitListExpr>(value);

                plog::log(std::cout)
                    << "type of struct: " << value_list->getType().getAsString()
                    << " index " << i << std::endl;

                clang::RecordDecl *record;

                if (auto p = clang::dyn_cast<clang::ArrayType>(
                        value_list->getType())) {
                    plog::log(std::cout)
                        << "Element type: " << p->getElementType().getAsString()
                        << std::endl;

                    record = p->getElementType()
                                 .getTypePtr()
                                 ->getAs<clang::RecordType>()
                                 ->getDecl();

                } else {
                    record = value_list->getType()
                                 .getTypePtr()
                                 ->getAs<clang::RecordType>()
                                 ->getDecl();

                    if (value_list->getType().getAsString() ==
                        "struct xdp2_flag_field") {
                        flag_field_data.fields.push_back(flag_field{});
                    }
                }

                // Extracts declaration list of current initializer list

                plog::log(std::cout)
                    << "  - Recursively calling handle_init_list_expr"
                    << std::endl;

                // Performs a recursive call to process new exprInitList
                handle_init_list_expr(value_list, record, flag_field_data);
            }
        }
    }
};

class extract_flag_fields_struct_constants : public clang::SyntaxOnlyAction {
    using base = clang::SyntaxOnlyAction;

private:
    std::vector<xdp2_flag_fields_extract_data> &consumed_data;

public:
    extract_flag_fields_struct_constants(
        std::vector<xdp2_flag_fields_extract_data> &consumed_data)
        : consumed_data{ consumed_data }
    {
    }

public:
    virtual bool BeginInvocation(clang::CompilerInstance &CI) override
    {
        clang::PreprocessorOptions &PPOpts = CI.getPreprocessorOpts();
        PPOpts.SingleFileParseMode = false;
        return true;
    }

    virtual void ExecuteAction() override
    {
        plog::log(std::cout)
            << "Execute action extract_proto_node_struct_constants"
            << std::endl;

        this->base::ExecuteAction();
    }

    virtual bool usesPreprocessorOnly() const override
    {
        return false;
    }
    virtual bool hasCodeCompletionSupport() const override
    {
        return true;
    }

    virtual std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &ci,
                      llvm::StringRef InFile) override
    {
        plog::log(std::cout)
            << "CreateASTConsumer xdp2_flag_fields_consumer" << std::endl;

        return std::make_unique<xdp2_flag_fields_consumer>(consumed_data);
    }
};

//
// Custom Factory for parameters passing
//

std::unique_ptr<clang::tooling::FrontendActionFactory>
extract_flag_fields_struct_constants_factory(
    std::vector<xdp2_flag_fields_extract_data> &consumed_data)
{
    class simple_frontend_action_factory
        : public clang::tooling::FrontendActionFactory {
    public:
        simple_frontend_action_factory(
            std::vector<xdp2_flag_fields_extract_data> &options)
            : Options(options)
        {
        }

        virtual std::unique_ptr<clang::FrontendAction> create() override
        {
            return std::make_unique<extract_flag_fields_struct_constants>(
                Options);
        }

    private:
        std::vector<xdp2_flag_fields_extract_data> &Options;
    };

    return std::unique_ptr<clang::tooling::FrontendActionFactory>(
        new simple_frontend_action_factory(consumed_data));
}

#endif /* SXDP2GEN_AST_CONSUMER_FLAG_FIELDS_H_ */
