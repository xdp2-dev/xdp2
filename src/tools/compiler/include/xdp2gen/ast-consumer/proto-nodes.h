#pragma once
#ifndef PROTO_NODES_H
#define PROTO_NODES_H

// Standard
// - Io manip
#include <iostream>
// - Data manip
#include <optional>
// - Data types
#include <string>

// Clang
#include <clang/Tooling/Tooling.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <xdp2gen/program-options/log_handler.h>

struct xdp2_proto_node_extract_data {
    std::string decl_name;
    std::optional<std::string> name;
    std::optional<std::size_t> min_len;
    std::optional<std::string> len;
    std::optional<std::string> next_proto;
    std::optional<std::string> tlv_len;
    std::optional<std::string> tlv_type;
    std::optional<std::string> tlv_start_offset;
    std::optional<__u8> pad1_val;
    std::optional<__u8> padn_val;
    std::optional<__u8> eol_val;
    std::optional<bool> pad1_enable;
    std::optional<bool> padn_enable;
    std::optional<bool> eol_enable;
    std::optional<std::string> start_fields_offset;
    std::optional<std::string> get_flags;
    std::optional<std::string> flag_fields_name;
    std::optional<bool> overlay;
    std::optional<bool> encap;

    friend inline std::ostream &
    operator<<(std::ostream &os, xdp2_proto_node_extract_data const &data)
    {
        os << "------------PROTO NODE DATA------------"
           << "\n";
        os << "name: " << data.name.value_or("empty") << "\n";
        os << "min_len: " << data.min_len.value_or(0) << "\n";
        os << "len: " << data.len.value_or("empty") << "\n";
        os << "next_proto: " << data.next_proto.value_or("empty") << "\n";
        if (data.tlv_type.has_value())
            os << "tlv_type: " << data.tlv_type.value() << "\n";
        if (data.tlv_len.has_value())
            os << "tlv_len: " << data.tlv_len.value() << "\n";
        if (data.tlv_start_offset.has_value())
            os << "tlv_start_offset: " << data.tlv_start_offset.value() << "\n";
        if (data.pad1_enable.has_value()) {
            os << "pad1_enable: "
               << (data.pad1_enable.value() ? "true" : "false") << "\n";

            if (data.pad1_enable.value())
                os << "pad1_value: " << data.pad1_val.value() << "\n";
        }
        if (data.padn_enable.has_value()) {
            os << "padn_enable: "
               << (data.padn_enable.value() ? "true" : "false") << "\n";

            if (data.padn_enable.value())
                os << "padn_value: " << data.padn_val.value() << "\n";
        }
        if (data.eol_enable.has_value()) {
            os << "eol_enable: " << (data.eol_enable.value() ? "true" : "false")
               << "\n";

            if (data.eol_enable.value())
                os << "eol_value: " << data.eol_val.value() << "\n";
        }
        if (data.start_fields_offset.has_value())
            os << "start_fields_offset: " << data.start_fields_offset.value()
               << "\n";
        if (data.get_flags.has_value())
            os << "get_flags: " << data.get_flags.value() << "\n";
        if (data.flag_fields_name.has_value())
            os << "flag_fields_name: " << data.flag_fields_name.value() << "\n";

        os << "-----------------END-------------------";

        return os;
    }
};

class xdp2_proto_node_consumer : public clang::ASTConsumer {
private:
    std::vector<xdp2_proto_node_extract_data> &consumed_data;

public:
    xdp2_proto_node_consumer(
        std::vector<xdp2_proto_node_extract_data> &consumed_data)
        : consumed_data{ consumed_data }
    {
    }

public:
    virtual bool HandleTopLevelDecl(clang::DeclGroupRef D) override
    {
        if (D.isSingleDecl()) {
            auto decl = D.getSingleDecl();

            if (decl->getKind() == clang::Decl::Var) {
                auto var_decl = clang::dyn_cast<clang::VarDecl>(decl);
                auto type = var_decl->getType().getAsString();

                if ((type == "const struct xdp2_proto_def" ||
                     type == "const struct xdp2_proto_tlvs_def" ||
                     type == "const struct xdp2_proto_flag_fields_def") &&
                    var_decl->hasInit()) {
                    plog::log(std::cout) << " TYPE DECL: " << type << std::endl;

                    bool tlvs = (type == "const struct xdp2_proto_tlvs_def");

                    // Extracts current decl name from proto node structure
                    std::string proto_decl_name = var_decl->getNameAsString();

                    clang::Expr *initializerExpr = var_decl->getInit();

                    if (initializerExpr->getStmtClass() ==
                        clang::Stmt::InitListExprClass) {
                        // Extracts current analyzed InitListExpr
                        clang::InitListExpr *initializer_list_expr =
                            clang::dyn_cast<clang::InitListExpr>(
                                initializerExpr);

                        // Extracts current analyzed InitListDecl
                        if (initializer_list_expr->getType()
                                ->getAs<clang::RecordType>()) {
                            clang::RecordDecl *initializer_list_decl =
                                initializer_list_expr->getType()
                                    ->getAs<clang::RecordType>()
                                    ->getDecl();

                            // Proto node consumed infos
                            xdp2_proto_node_extract_data node_data;

                            // Set the decl name of proto node structure
                            node_data.decl_name = proto_decl_name;

                            handle_init_list_expr(initializer_list_expr,
                                                  initializer_list_decl,
                                                  node_data, tlvs);

                            const auto is_empty_node = [&node_data]() -> bool {
                                return !(node_data.name.has_value() ||
                                         node_data.min_len.has_value() ||
                                         node_data.len.has_value() ||
                                         node_data.next_proto.has_value());
                            };

                            // Inserts node data in result vector
                            if (!is_empty_node())
                                consumed_data.push_back(node_data);
                        }
                    }
                }
            }
        }
        return true;
    }

public:
    void handle_init_list_expr(clang::InitListExpr *initializer_list_expr,
                               clang::RecordDecl *records,
                               xdp2_proto_node_extract_data &node_data,
                               bool tlvs)
    {
        // Iterates trougth all Expr in init list
        for (size_t i = 0; i < initializer_list_expr->getNumInits(); ++i) {
            // Current analyzed Expr
            clang::Expr *value = initializer_list_expr->getInits()[i];

            // Current analyzed Decl
            auto record = std::next(records->field_begin(), i);

            // Ignore clang internal casts
            value = value->IgnoreImplicitAsWritten();

            std::string parent_struct =
                initializer_list_expr->getType().getAsString();

            if (value->getStmtClass() == clang::Stmt::IntegerLiteralClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** INTEGRAL EXPR REACHED ****" << std::endl;

                clang::IntegerLiteral *value_unary =
                    clang::dyn_cast<clang::IntegerLiteral>(value);

                // Get current analyzed field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - INTEGRAL NAME: " << field_name << std::endl;

                auto v = value->getIntegerConstantExpr(record->getASTContext());

                plog::log(std::cout)
                    << "    - INTEGRAL EVALUATED VALUE: " << v->getExtValue()
                    << std::endl;

                if (tlvs) {
                    if (field_name == "eol_enable")
                        node_data.eol_enable =
                            (v->getExtValue() == 1 ? true : false);
                    else if (field_name == "pad1_enable")
                        node_data.pad1_enable =
                            (v->getExtValue() == 1 ? true : false);
                    else if (field_name == "padn_enable")
                        node_data.padn_enable =
                            (v->getExtValue() == 1 ? true : false);
                    if (field_name == "eol_val")
                        node_data.eol_val = v->getExtValue();
                    else if (field_name == "pad1_val")
                        node_data.pad1_val = v->getExtValue();
                    else if (field_name == "padn_val")
                        node_data.padn_val = v->getExtValue();

                } else if (field_name == "overlay") {
                    if (v) {
                        node_data.overlay =
                            v->getExtValue(); // <- Should put evaluated value here
                        plog::log(std::cout) << "ovelay assigned "
                                             << *node_data.overlay << std::endl;
                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE INTEGRAL LITERAL"
                            << std::endl;
                    }

                } else if (field_name == "encap") {
                    if (v) {
                        node_data.encap =
                            v->getExtValue(); // <- Should put evaluated value here
                        plog::log(std::cout) << "encap assigned "
                                             << *node_data.encap << std::endl;
                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE INTEGRAL LITERAL"
                            << std::endl;
                    }

                } else if (field_name == "min_len" &&
                           parent_struct.find("xdp2_proto_def") !=
                               std::string::npos) {
                    if (v) {
                        node_data.min_len =
                            v->getExtValue(); // <- Should put evaluated value here
                        plog::log(std::cout)
                            << "min len " << *node_data.min_len << std::endl;
                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE INTEGRAL LITERAL"
                            << std::endl;
                    }
                }

            } else if (value->getStmtClass() ==
                       clang::Stmt::UnaryOperatorClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** UNARY OPERATOR REACHED ****" << std::endl;

                // Get current analyzed field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - UNARY OPERATOR NAME: " << field_name << std::endl;

                if (field_name == "flag_fields") {
                    clang::UnaryOperator *unary_op =
                        clang::dyn_cast<clang::UnaryOperator>(value);

                    clang::DeclRefExpr *value_decl_ref =
                        clang::dyn_cast<clang::DeclRefExpr>(
                            unary_op->getSubExpr());

                    if (value_decl_ref) {
                        clang::VarDecl *var = clang::dyn_cast<clang::VarDecl>(
                            value_decl_ref->getDecl());

                        if (var) {
                            node_data.flag_fields_name = var->getNameAsString();
                            plog::log(std::cout)
                                << " variable name: " << var->getNameAsString()
                                << std::endl;
                        }
                    }
                }

            } else if (value->getStmtClass() ==
                       clang::Stmt::BinaryOperatorClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** BINARY OPERATOR REACHED ****" << std::endl;

                // Get current analzyed field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - BINARY OPERATOR NAME: " << field_name << std::endl;

                if (field_name == "min_len" &&
                    parent_struct.find("xdp2_proto_def") !=
                        std::string::npos) {
                    //dyn_cast<::llvm::Value>(value)->print(::llvm::errors());
                    auto v =
                        value->getIntegerConstantExpr(record->getASTContext());
                    if (v) {
                        plog::log(std::cout) << "    - BINARY EVALUATED VALUE: "
                                             << v->getExtValue() << std::endl;
                        node_data.min_len =
                            v->getExtValue(); // <- Should put evaluated value here
                        plog::log(std::cout) << "min len assigned "
                                             << *node_data.min_len << std::endl;
                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE BINARY EXPR"
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

                // Get current analyzed field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - UNARY NAME: " << field_name << std::endl;

                // In case of name be found
                if (field_name == "min_len" &&
                    parent_struct.find("xdp2_proto_def") !=
                        std::string::npos) {
                    auto v =
                        value->getIntegerConstantExpr(record->getASTContext());
                    if (v) {
                        plog::log(std::cout) << "    - UNARY EVALUATED VALUE: "
                                             << v->getExtValue() << std::endl;
                        node_data.min_len =
                            v->getExtValue(); // <- Should put evaluated value here
                        plog::log(std::cout) << "min len assigned "
                                             << *node_data.min_len << std::endl;
                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE UNARY EXPR"
                            << std::endl;
                    }
                }
            }

            // In case of StringLiteral Expr be found
            if (value->getStmtClass() == clang::Stmt::StringLiteralClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** STRING LITERAL EXPR REACHED ****" << std::endl;

                clang::StringLiteral *valueStr =
                    clang::dyn_cast<clang::StringLiteral>(value);

                // Get current analyzed field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - STRING LITERAL NAME: " << field_name << std::endl;

                // In case of name be found
                if (field_name == "name") {
                    plog::log(std::cout)
                        << "    - STRING VALUE: " << valueStr->getString().str()
                        << std::endl;

                    auto pos = valueStr->getString().str().find(".");

                    node_data.name = valueStr->getString().str().substr(0, pos);
                }
            }

            // In case of InitListExpr Expr be found
            if (value->getStmtClass() == clang::Stmt::InitListExprClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "**** INIT LIST EXPR REACHED ****" << std::endl;

                clang::InitListExpr *value_list =
                    clang::dyn_cast<clang::InitListExpr>(value);

                // Extracts declaration list of current initializer list
                clang::RecordDecl *record = value_list->getType()
                                                .getTypePtr()
                                                ->getAs<clang::RecordType>()
                                                ->getDecl();

                plog::log(std::cout)
                    << "type of struct " << value_list->getType().getAsString()
                    << std::endl;

                plog::log(std::cout)
                    << "  - Recursively calling handle_init_list_expr"
                    << std::endl;

                bool is_initializing_tlv_related_data =
                    value_list->getType().getAsString().find("tlv") !=
                    std::string::npos;

                if (is_initializing_tlv_related_data)
                    plog::log(std::cout)
                        << "is a tlv related struct" << std::endl;

                // Performs a recursive call to process new exprInitList
                handle_init_list_expr(value_list, record, node_data,
                                      is_initializing_tlv_related_data);
            }

            // In case of DeclRefExpr Expr be found
            if (value->getStmtClass() == clang::Stmt::DeclRefExprClass) {
                plog::log(std::cout) << "DEBUG: "
                                     << "**** DECL REF EXPR REACHED, CLASS "
                                     << i << " ****" << std::endl;

                clang::DeclRefExpr *value_decl_ref =
                    clang::dyn_cast<clang::DeclRefExpr>(value);

                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - DECL REF EXPR NAME: " << field_name << std::endl;

                clang::FunctionDecl *function =
                    clang::dyn_cast<clang::FunctionDecl>(
                        value_decl_ref->getDecl());
                if (function) {
                    // Current analyzed function name
                    std::string function_name = function->getNameAsString();

                    plog::log(std::cout)
                        << "    - DECL REF EXPR VALUE NAME: " << function_name
                        << std::endl;

                    // In case of next_proto be found
                    if (field_name == "next_proto") {
                        node_data.next_proto = function_name;
                    }

                    // In case of next_proto be found
                    else if (field_name == "len") {
                        if (tlvs) {
                            plog::log(std::cout)
                                << "got tlv len function name" << std::endl;
                            node_data.tlv_len = function_name;

                        } else
                            node_data.len = function_name;

                    }

                    else if (field_name == "type") {
                        if (tlvs) {
                            node_data.tlv_type = function_name;
                        }

                    }

                    else if (field_name == "start_offset") {
                        if (tlvs) {
                            node_data.tlv_start_offset = function_name;
                        }

                    } else if (field_name == "start_fields_offset") {
                        node_data.start_fields_offset = function_name;

                    } else if (field_name == "get_flags") {
                        node_data.get_flags = function_name;
                    }
                }
            }
        }
    }
};

class extract_proto_node_struct_constants : public clang::SyntaxOnlyAction {
    using base = clang::SyntaxOnlyAction;

private:
    std::vector<xdp2_proto_node_extract_data> &consumed_data;

public:
    extract_proto_node_struct_constants(
        std::vector<xdp2_proto_node_extract_data> &consumed_data)
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
            << "CreateASTConsumer xdp2_proto_node_consumer" << std::endl;

        return std::make_unique<xdp2_proto_node_consumer>(consumed_data);
    }
};

//
// Custom Factory for parameters passing
//

std::unique_ptr<clang::tooling::FrontendActionFactory>
extract_proto_node_struct_constants_factory(
    std::vector<xdp2_proto_node_extract_data> &consumed_data)
{
    class simple_frontend_action_factory
        : public clang::tooling::FrontendActionFactory {
    public:
        simple_frontend_action_factory(
            std::vector<xdp2_proto_node_extract_data> &options)
            : Options(options)
        {
        }

        virtual std::unique_ptr<clang::FrontendAction> create() override
        {
            return std::make_unique<extract_proto_node_struct_constants>(
                Options);
        }

    private:
        std::vector<xdp2_proto_node_extract_data> &Options;
    };

    return std::unique_ptr<clang::tooling::FrontendActionFactory>(
        new simple_frontend_action_factory(consumed_data));
}

#endif // !PROTO_NODES_H
