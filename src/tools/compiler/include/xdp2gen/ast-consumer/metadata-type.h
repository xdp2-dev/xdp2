#ifndef XDP2GEN_AST_CONSUMER_METADATA_TYPE_H
#define XDP2GEN_AST_CONSUMER_METADATA_TYPE_H

#include <xdp2gen/pattern_match.h>
#include <xdp2gen/ast-consumer/factory.h>

#include <iostream>
#include <optional>
#include <string>

#include <clang/Tooling/Tooling.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <xdp2gen/program-options/log_handler.h>

namespace xdp2gen
{
namespace ast_consumer
{

struct metadata_record_consumer : public clang::ASTConsumer {
    std::string function_name;
    // std::vector<metadata_record>* records;
    std::map<std::pair<std::size_t, std::size_t>, std::string> *record_map =
        nullptr;
    xdp2gen::clang_ast::metadata_record *metadata_record = nullptr;
    xdp2gen::clang_ast::metadata_record *global_metadata_record = nullptr;

    metadata_record_consumer(
        std::string function_name
        // , std::vector<metadata_record>* records
        ,
        std::map<std::pair<std::size_t, std::size_t>, std::string> *record_map,
        xdp2gen::clang_ast::metadata_record *metadata_record,
        xdp2gen::clang_ast::metadata_record *global_metadata_record
        /*, clang::ASTContext* ast_context*/)
        : function_name(function_name)
        // , records(records)
        , record_map(record_map)
        , metadata_record(metadata_record)
        , global_metadata_record(global_metadata_record)
    /*, ast_context(ast_context)*/
    {
    }

    void HandleTranslationUnit(clang::ASTContext &ast_context) final
    {
        std::vector<xdp2gen::ast_consumer::metadata_record> records;
        auto Decls = ast_context.getTranslationUnitDecl()->decls();
        for (auto &Decl : Decls) {
            if (auto const *fd = clang::dyn_cast<clang::FunctionDecl>(Decl);
                fd && fd->getNameAsString() == function_name) {
                std::cout << "Building graph for declaration of function "
                          << function_name << std::endl;
                clang_ast_graph ast_graph(Decl);
                auto matches =
                    xdp2gen::ast_consumer::metadata_ast_patterns
                        .match_all<xdp2gen::ast_consumer::metadata_record>(
                            ast_graph, { 0 });
                for (auto mvar : matches) {
                    if (auto const *m = std::get_if<0>(&mvar)) {
                        std::cout << "Record: " << *m << std::endl;
                        records.push_back(*m);
                    }
                }
            }
        }

        clang::RecordDecl *record;
        std::string record_name;

        for (auto r : records) {
            plog::log(std::cout) << "[" << r << "]" << std::endl;

            if (record_name.empty()) {
                record_name = r.record_name;
                record = r.record;

                plog::log(std::cout)
                    << __LINE__ << " record: " << record_name << std::endl;

                *metadata_record =
                    xdp2gen::clang_ast::metadata_record_fields(record);

                if (global_metadata_record->fields.size() <
                    metadata_record->fields.size()) {
                    *global_metadata_record = *metadata_record;
                }

                plog::log(std::cout)
                    << "records saved " << *metadata_record << std::endl;
            } else if (record_name != r.record_name)
                plog::log(std::cerr)
                    << "Using different metadata structures for outputing"
                    << std::endl;
            auto key =
                get_offset_size_from_field(*metadata_record, r.field_name);
            if (key) {
                record_map->insert(std::make_pair(*key, r.field_name));
                plog::log(std::cout)
                    << "found offset " << key->first << " and size "
                    << key->second << std::endl;
            } else {
                plog::log(std::cout) << "not found offset and size for "
                                     << r.field_name << std::endl;
            }
        }
    }
};

std::map<std::pair<std::size_t, std::size_t>, std::string>
metadata_records(clang::tooling::ClangTool &tool, std::string function_name,
                 xdp2gen::clang_ast::metadata_record &metadata_record,
                 xdp2gen::clang_ast::metadata_record &global_metadata_record)
{
    std::map<std::pair<std::size_t, std::size_t>, std::string> record_map;
    ast_consumer::frontend_factory_for_consumer<metadata_record_consumer>
        factory(function_name, &record_map, &metadata_record,
                &global_metadata_record);
    tool.run(&factory);

    return record_map;
}

}
}

#endif
