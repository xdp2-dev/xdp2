#pragma once
#ifndef PROTO_TABLES_H
#define PROTO_TABLES_H

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

// XDP2
#include <xdp2gen/program-options/log_handler.h>
#include <xdp2gen/assert.h>

struct xdp2_proto_table_extract_data {
    using entries_type = std::vector<std::pair<unsigned int, std::string>>;

    std::string decl_name;
    unsigned int num_ents;
    std::string entries_name;
    entries_type entries;

    friend inline std::ostream &
    operator<<(std::ostream &os, xdp2_proto_table_extract_data const &data)
    {
        os << "---------START PROTO TABLE DATA---------"
           << "\n";
        os << "name: " << data.decl_name << "\n";
        os << "num_ents: " << data.num_ents << "\n";
        os << "ents_name: " << data.entries_name << "\n";
        os << "entries: "
           << "\n";

        for (auto [key, proto_name] : data.entries) {
            os << "  - "
               << "key: " << key << ", proto: " << proto_name << "\n";
        }

        plog::log(std::cout) << "-----------------END-------------------";

        return os;
    }
};

class xdp2_proto_table_consumer : public clang::ASTConsumer {
private:
    std::vector<xdp2_proto_table_extract_data> &consumed_data;

public:
    xdp2_proto_table_consumer(
        std::vector<xdp2_proto_table_extract_data> &consumed_data)
        : consumed_data{ consumed_data }
    {
    }

public:
    virtual bool HandleTopLevelDecl(clang::DeclGroupRef D) override
    {
        // [nix-patch] Process ALL declarations in the group, not just single decls.
        // XDP2_MAKE_PROTO_TABLE creates TWO declarations (__name entries array + name table)
        // which may be grouped together, causing isSingleDecl() to return false.
        for (auto *decl : D) {
            if (decl->getKind() == clang::Decl::Var) {
                auto var_decl = clang::dyn_cast<clang::VarDecl>(decl);
                auto type = var_decl->getType().getAsString();

                // [nix-debug] Log ALL VarDecls containing "table" in name or type
                std::string name = var_decl->getNameAsString();
                if (name.find("table") != std::string::npos ||
                    type.find("table") != std::string::npos) {
                    plog::log(std::cout)
                        << "[proto-tables-all] VarDecl: " << name
                        << " type=" << type
                        << " hasInit=" << (var_decl->hasInit() ? "yes" : "no")
                        << " isDefinition=" << (var_decl->isThisDeclarationADefinition() ==
                            clang::VarDecl::Definition ? "yes" : "no")
                        << std::endl;
                }

                // [nix-patch] Debug: Log all table-type VarDecls to diagnose extraction issues
                bool is_type_some_table =
                    (type == "const struct xdp2_proto_table" ||
                     type == "const struct xdp2_proto_tlvs_table" ||
                     type == "const struct xdp2_proto_flag_fields_table");

                if (is_type_some_table) {
                    plog::log(std::cout)
                        << "[proto-tables] Found table VarDecl: "
                        << var_decl->getNameAsString()
                        << " type=" << type
                        << " hasInit=" << (var_decl->hasInit() ? "yes" : "no")
                        << " stmtClass=" << (var_decl->hasInit() && var_decl->getInit()
                            ? var_decl->getInit()->getStmtClassName() : "N/A")
                        << std::endl;
                }

                if (is_type_some_table && var_decl->hasInit()) {
                    // Extracts current decl name from proto table structure
                    std::string table_decl_name = var_decl->getNameAsString();

                    plog::log(std::cout)
                        << " Analyzing table " << table_decl_name << std::endl;

                    clang::Expr *initializer_expr = var_decl->getInit();

                    if (initializer_expr->getStmtClass() ==
                        clang::Stmt::InitListExprClass) {
                        // Extracts current analyzed InitListExpr
                        clang::InitListExpr *initializer_list_expr =
                            clang::dyn_cast<clang::InitListExpr>(
                                initializer_expr);

                        // [nix-patch] Handle tentative definitions to prevent null pointer crash.
                        //
                        // PROBLEM: C tentative definitions like:
                        //   static const struct xdp2_proto_table ip_table;
                        // are created by XDP2_DECL_PROTO_TABLE macro before the actual definition.
                        //
                        // Different clang versions handle hasInit() differently for these:
                        //   - Ubuntu clang 18.1.3: hasInit() returns false (skipped entirely)
                        //   - Nix clang 18.1.8+:   hasInit() returns true with void-type InitListExpr
                        //
                        // When getAs<RecordType>() is called on void type, it returns nullptr.
                        // The original code then calls ->getDecl() on nullptr, causing segfault.
                        //
                        // SOLUTION: Check if RecordType is null and skip tentative definitions.
                        // The actual definition will be processed when encountered later in the AST.
                        //
                        // See: documentation/nix/phase6_segfault_defect.md for full investigation.
                        clang::QualType initType = initializer_list_expr->getType();
                        auto *recordType = initType->getAs<clang::RecordType>();
                        if (!recordType) {
                            // Skip tentative definitions - actual definition processed later
                            plog::log(std::cout) << "[proto-tables] Skipping tentative definition: "
                                << table_decl_name << " (InitListExpr type: "
                                << initType.getAsString() << ")" << std::endl;
                            continue;
                        }

                        // Extracts current analyzed InitListDecl
                        clang::RecordDecl *initializer_list_decl = recordType->getDecl();

                        // Proto table consumed infos
                        xdp2_proto_table_extract_data table_data;

                        // Set the decl name of proto node structure
                        table_data.decl_name = table_decl_name;

                        handle_init_list_expr(initializer_list_expr,
                                              initializer_list_decl,
                                              table_data);

                        // Inserts node data in result vector
                        consumed_data.push_back(table_data);
                    }
                }
            }
        }
        return true;
    }

public:
    void handle_init_list_expr(clang::InitListExpr *initializer_list_expr,
                               clang::RecordDecl *records,
                               xdp2_proto_table_extract_data &table_data)
    {
        // Iterates trougth all Expr in init list
        for (size_t i = 0; i < initializer_list_expr->getNumInits(); ++i) {
            // Current analyzed Expr
            clang::Expr *value = initializer_list_expr->getInits()[i];

            // Current analyzed Decl
            auto record = std::next(records->field_begin(), i);

            // Ignore clang internal casts
            value = value->IgnoreImplicitAsWritten();

            // In case of Unary Expr be found
            if (value->getStmtClass() == clang::Stmt::BinaryOperatorClass) {
                plog::log(std::cout)
                    << "DEBUG: "
                    << "*** BINARY EXPR REACHED ***" << std::endl;

                clang::UnaryExprOrTypeTraitExpr *value_unary =
                    clang::dyn_cast<clang::UnaryExprOrTypeTraitExpr>(value);

                // Get current analyzed field name
                const std::string field_name = record->getNameAsString();

                plog::log(std::cout)
                    << "  - BINARY NAME: " << field_name << std::endl;

                // In case of name be found
                if (field_name == "num_ents") {
                    auto v =
                        value->getIntegerConstantExpr(record->getASTContext());
                    if (v) {
                        plog::log(std::cout) << "    - BINARY EVALUATED VALUE: "
                                             << v->getExtValue() << std::endl;
                        table_data.num_ents =
                            v->getExtValue(); // <- Should put evaluated value here
                    } else {
                        plog::log(std::cout)
                            << "    ! NOT ABLE TO EVALUATE UNARY EXPR"
                            << std::endl;
                    }
                }
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

                if (field_name == "entries") {
                    clang::VarDecl *entries = clang::dyn_cast<clang::VarDecl>(
                        value_decl_ref->getDecl());

                    table_data.entries_name = entries->getNameAsString();

                    if (entries->hasInit()) {
                        clang::Expr *ents_init_expr = entries->getInit();

                        if (ents_init_expr->getStmtClass() ==
                            clang::Stmt::InitListExprClass) {
                            // Extracts current analyzed InitListExpr
                            clang::InitListExpr *ents_init_list_expr =
                                clang::dyn_cast<clang::InitListExpr>(
                                    ents_init_expr);

                            // Iterates in each element of the ents array
                            for (size_t j = 0;
                                 j < ents_init_list_expr->getNumInits(); ++j) {
                                // Current analyzed Expr
                                clang::Expr *ent_value =
                                    ents_init_list_expr->getInits()[j];

                                if (ent_value->getStmtClass() ==
                                    clang::Stmt::InitListExprClass) {
                                    clang::InitListExpr *ent =
                                        clang::dyn_cast<clang::InitListExpr>(
                                            ent_value);

                                    // Extracts current analyzed InitListDecl
                                    // Note: getAs<RecordType>() can return nullptr
                                    // for incomplete types or tentative definitions
                                    auto *ent_record_type =
                                        ent_value->getType()
                                            ->getAs<clang::RecordType>();
                                    if (!ent_record_type) {
                                        plog::log(std::cout)
                                            << "[proto-tables] Skipping entry "
                                            << "with null RecordType" << std::endl;
                                        continue;
                                    }
                                    clang::RecordDecl *ent_decl =
                                        XDP2_REQUIRE_NOT_NULL(
                                            ent_record_type->getDecl(),
                                            "entry RecordDecl from RecordType");

                                    // Extract current key / proto pair from init expr
                                    std::pair<unsigned int, std::string> entry =
                                        extract_proto_table_pair(ent, ent_decl);

                                    //
                                    // By: João M.
                                    // OBS: Maybe is interesting add some validation
                                    // here before push in vector, should look for
                                    // possible danger cases
                                    //
                                    table_data.entries.push_back(entry);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    std::pair<unsigned int, std::string>
    extract_proto_table_pair(clang::InitListExpr *initializer_list_expr,
                             clang::RecordDecl *record)
    {
        plog::log(std::cout) << "  - EXTRACTING KEY / NODE PAIR " << std::endl;

        unsigned int key_value = 0;
        std::string proto_name = "";

        // Current analyzed key Expr
        clang::Expr *key_expr = initializer_list_expr->getInits()[0];

        auto key = key_expr->getIntegerConstantExpr(record->getASTContext());

        if (key) {
            plog::log(std::cout)
                << "    - KEY EVALUATED VALUE: " << key->getExtValue()
                << std::endl;

            key_value = key->getExtValue();
        } else {
            plog::log(std::cout)
                << "    ! NOT ABLE TO EVALUATE KEY EXPR" << std::endl;
        }

        // Current analyzed proto Expr
        clang::Expr *proto_ref = initializer_list_expr->getInits()[1];

        proto_name = get_table_proto_name(proto_ref);

        return { key_value, proto_name };
    }

    std::string get_table_proto_name(clang::Expr *expr)
    {
        if (expr->getStmtClass() == clang::Stmt::DeclRefExprClass) {
            clang::DeclRefExpr *proto_ref =
                clang::dyn_cast<clang::DeclRefExpr>(expr);
            return proto_ref->getDecl()->getNameAsString();
        } else if (expr->getStmtClass() == clang::Stmt::MemberExprClass) {
            clang::MemberExpr *proto_ref =
                clang::dyn_cast<clang::MemberExpr>(expr);

            auto child = proto_ref->child_begin();

            clang::Expr *child_expr = clang::dyn_cast<clang::Expr>(*child);

            std::stringstream ss;

            ss << get_table_proto_name(child_expr) << "."
               << proto_ref->getMemberDecl()->getNameAsString();

            return ss.str();
        } else {
            auto child = expr->child_begin();

            clang::Expr *child_expr = clang::dyn_cast<clang::Expr>(*child);

            return get_table_proto_name(child_expr);
        }
    }
};

class extract_proto_table_struct_constants : public clang::SyntaxOnlyAction {
    using base = clang::SyntaxOnlyAction;

private:
    std::vector<xdp2_proto_table_extract_data> &consumed_data;

public:
    extract_proto_table_struct_constants(
        std::vector<xdp2_proto_table_extract_data> &consumed_data)
        : consumed_data{ consumed_data }
    {
    }

public:
    virtual bool BeginInvocation(clang::CompilerInstance &CI) override
    {
        clang::PreprocessorOptions &PPOpts = CI.getPreprocessorOpts();
        //assert(PPOpts.SingleFileParseMode);
        PPOpts.SingleFileParseMode = false;
        return true;
    }

    virtual void ExecuteAction() override
    {
        plog::log(std::cout)
            << "Execute action extract_proto_table_struct_constants"
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
            << "CreateASTConsumer xdp2_proto_table_consumer" << std::endl;

        return std::make_unique<xdp2_proto_table_consumer>(consumed_data);
    }
};

//
// Custom Factory for parameters passing
//

std::unique_ptr<clang::tooling::FrontendActionFactory>
extract_proto_table_struct_constants_factory(
    std::vector<xdp2_proto_table_extract_data> &consumed_data)
{
    class simple_frontend_action_factory
        : public clang::tooling::FrontendActionFactory {
    public:
        simple_frontend_action_factory(
            std::vector<xdp2_proto_table_extract_data> &options)
            : Options(options)
        {
        }

        virtual std::unique_ptr<clang::FrontendAction> create() override
        {
            return std::make_unique<extract_proto_table_struct_constants>(
                Options);
        }

    private:
        std::vector<xdp2_proto_table_extract_data> &Options;
    };

    return std::unique_ptr<clang::tooling::FrontendActionFactory>(
        new simple_frontend_action_factory(consumed_data));
}

#endif // !PROTO_TABLES_H
