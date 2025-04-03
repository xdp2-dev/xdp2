#pragma once
#ifndef PROTO_GRAPH_H
#define PROTO_GRAPH_H

// Standard
// - Io manip
#include <iostream>
// - Data manip
#include <optional>
// - Data types
#include <string>

#include "xdp2gen/graph.h"

// Clang
#include <clang/Tooling/Tooling.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <xdp2gen/program-options/log_handler.h>

template <typename G>
struct graph_info {
    G *graph;
    std::vector<xdp2gen::parser<G>> *roots;
    std::vector<xdp2gen::table> tlv_tables;
    std::vector<xdp2gen::table> parser_tables;
    std::vector<xdp2gen::table> flag_fields_tables;
    std::vector<xdp2gen::tlv_node> tlv_nodes;
    std::vector<xdp2gen::flag_fields_node> flag_fields_nodes;
};

template <typename G>
class xdp2_graph_consumer : public clang::ASTConsumer {
private:
    graph_info<G> &consumed_data;

public:
    xdp2_graph_consumer(graph_info<G> &consumed_data)
	: consumed_data{ consumed_data }
    {
    }

    virtual bool HandleTopLevelDecl(clang::DeclGroupRef D) override
    {
	if (D.isSingleDecl()) {
	    auto decl = D.getSingleDecl();
	    if (const auto *ND = llvm::dyn_cast<clang::NamedDecl>(decl)) {
	      plog::log(std::cout)  << "Decl name: " << ND->getNameAsString() << "\n";
	    } else {
	      plog::log(std::cout) << "Decl has no name.\n";
	    }


	    if (decl->getKind() == clang::Decl::Var) {
	      plog::log(std::cout) << " == Var" << std::endl;

		auto var_decl = clang::dyn_cast<clang::VarDecl>(decl);
		auto type = var_decl->getType().getAsString();
		plog::log(std::cout) << "type |" << type << "|" << std::endl;

		if (var_decl->hasInit()) {
		    plog::log(std::cout) << " TYPE DECL: " << type << std::endl;
		    if (type == "const struct xdp2_parse_user_node") {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_parse_node(var_decl);

		    } else if (type == "const struct xdp2_parser") {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE YYDECL: " << type << std::endl;

			_process_xdp2_parser(var_decl);

		    } else if (type == "const struct xdp2_parser_def") {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_parser_def(var_decl);

		    }

		    else if (type ==
			     "const struct xdp2_parse_flag_fields_node") {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_parse_node(var_decl);

		    } else if (type ==
			       "const struct xdp2_parse_flag_field_node") {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_parse_flag_field_node(var_decl);

		    } else if (type == "const struct xdp2_parse_tlvs_node") {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_parse_node(var_decl);

		    } else if (type == "const struct xdp2_parse_tlv_node") {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_parse_tlv_node(var_decl);

		    } else if (type.find(
				   "const struct xdp2_proto_table_entry[]") !=
			       std::string::npos) {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_table(var_decl,
					     consumed_data.parser_tables);

			if (!consumed_data.parser_tables.empty())
			  plog::log(std::cout)
			    << consumed_data.parser_tables.back() << std::endl;

		    } else if (type.find(
				   "const struct xdp2_proto_tlvs_table_entry") !=
			       std::string::npos) {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_table(var_decl,
					     consumed_data.tlv_tables);

		    } else if (
			type.find(
			    "const struct xdp2_proto_flag_fields_table_entry") !=
			std::string::npos) {
			if (plog::is_display_log())
			    var_decl->dump();
			plog::log(std::cout)
			    << " TYPE DECL: " << type << std::endl;

			_process_xdp2_table(var_decl,
					     consumed_data.flag_fields_tables);
		    }
		}
	    }
	}

	return true;
    }

private:
    G &_get_graph_ref(G *g)
    {
	return (*g);
    }
    std::vector<xdp2gen::parser<G>> &
    _get_roots_ref(std::vector<xdp2gen::parser<G>> *roots)
    {
	return (*roots);
    }

    template <typename N>
    void _handle_init_list_expr_xdp2_parser(
	const clang::InitListExpr *initializer_list_expr, const clang::RecordDecl *records,
	N &node, std::string const &parent_name)
    {
	assert(initializer_list_expr != nullptr);
	plog::log(std::cout) << "_handle_init_list_expr_xdp2_parser getNumInits() "
			     << initializer_list_expr->getNumInits() << std::endl;
	// Iterates trougth all Expr in init list
	for (size_t i = 0; i < initializer_list_expr->getNumInits(); ++i) {
	    // Current analised Expr
	    clang::Expr *value = initializer_list_expr->getInits()[i];

	    plog::log(std::cout) << "Dump value" << std::endl;

	    // Current analised Decl
	    auto cur_record = std::next(records->field_begin(), i);

	    // Ignore clang internal casts
	    value = value->IgnoreImplicitAsWritten();

	    std::string field_name = cur_record->getNameAsString();

	    // In case of InitListExpr Expr be found
	    if (value->getStmtClass() == clang::Stmt::InitListExprClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::InitListExprClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

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
		    << "  - Recursively calling _handle_init_list_expr_parse_node " << __func__
		    << std::endl;

		// Performs a recursive call to process new exprInitList
		_handle_init_list_expr_xdp2_parser(value_list, record, node,
						    field_name);

	    }

	    else if (value->getStmtClass() == clang::Stmt::UnaryOperatorClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::UnaryOperatorClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

		bool is_cur_field_of_interest =
		    (field_name == "parser" || field_name == "root_node" ||
		     field_name == "okay_node" || field_name == "fail_node" ||
		     field_name == "atencap_node");

		clang::VarDecl *var = nullptr;

		if (is_cur_field_of_interest) {
		    clang::UnaryOperator *unary_op =
			clang::dyn_cast<clang::UnaryOperator>(value);

		    auto handler = [&] (clang::VarDecl* var) {
			    if (field_name == "parser") {
				node.parser_name = var->getNameAsString();
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "root_node") {
				auto g = _get_graph_ref(consumed_data.graph);

				plog::log(std::cout) << "root_node field. Searching for " << var->getNameAsString() << std::endl;

				auto pv = xdp2gen::search_vertex_by_name(
				    g, var->getNameAsString());

				plog::log(std::cout) << "pv " << (pv ? "[yes]" : "[no]") << std::endl;

				if (pv)
				    node.root = *pv;

				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "okay_node") {
				auto g = _get_graph_ref(consumed_data.graph);

				auto pv = xdp2gen::search_vertex_by_name(
				    g, var->getNameAsString());

				if (pv) {
				    node.okay_target = *pv;
				    node.okay_target_set = true;
				}

				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "fail_node") {
				auto g = _get_graph_ref(consumed_data.graph);

				auto pv = xdp2gen::search_vertex_by_name(
				    g, var->getNameAsString());

				if (pv) {
				    node.fail_target = *pv;
				    node.fail_target_set = true;
				}

				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "atencap_node") {
				auto g = _get_graph_ref(consumed_data.graph);

				auto pv = xdp2gen::search_vertex_by_name(
				    g, var->getNameAsString());

				if (pv) {
				    node.encap_target = *pv;
				    node.encap_target_set = true;
				}
			    }
		    };
		    if (clang::DeclRefExpr *value_decl_ref =
			    clang::dyn_cast<clang::DeclRefExpr>(
				unary_op->getSubExpr())) {
			if (clang::VarDecl *var =
				clang::dyn_cast<clang::VarDecl>(
				    value_decl_ref->getDecl())) {
			    handler(var);
			}
		    }

		    if (clang::MemberExpr *member_expr_ref =
			    clang::dyn_cast<clang::MemberExpr>(
				unary_op->getSubExpr())) {
		      if (clang::DeclRefExpr *value_decl_ref =
			      clang::dyn_cast<clang::DeclRefExpr>(
				  member_expr_ref->getBase())) {
			if (clang::VarDecl *var =
				clang::dyn_cast<clang::VarDecl>(
				    value_decl_ref->getDecl())) {
			    handler(var);
			}
		      }
		    }
		}
            } else if (value->getStmtClass() == clang::Stmt::UnaryExprOrTypeTraitExprClass) {
              plog::log(std::cout)
                << "DEBUG: "
                << "clang::Stmt::UnaryExprOrTypeTraitExprClass" << std::endl;

              auto *UETT = llvm::dyn_cast<clang::UnaryExprOrTypeTraitExpr>(value);
              assert (UETT != nullptr);
              if (UETT && UETT->isValueDependent() == false) {
                llvm::APSInt value = UETT->EvaluateKnownConstInt(cur_record->getASTContext());

                if (field_name == "frame_size") {
                  node.frame_size = value.getZExtValue();
                  plog::log(std::cout)
                    << "  bit width: " << value.getBitWidth()
                    << std::endl;
                  plog::log(std::cout)
                    << "  literal_value: " << value.getZExtValue()
                    << std::endl;
                } else if (field_name == "metameta_size") {
                  node.metameta_size = value.getZExtValue();
                  plog::log(std::cout)
                    << "  bit width: " << value.getBitWidth()
                    << std::endl;
                  plog::log(std::cout)
                    << "  literal_value: " << value.getZExtValue()
                    << std::endl;
                }
              }
	    } else if (value->getStmtClass() ==
		       clang::Stmt::IntegerLiteralClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::IntegerLiteralClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

		if (parent_name == "config") {
		    clang::IntegerLiteral *int_literal =
			clang::dyn_cast<clang::IntegerLiteral>(value);

		    auto v = value->getIntegerConstantExpr(
			cur_record->getASTContext());

		    if (v) {
			if (field_name == "max_nodes") {
			    node.max_nodes = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "max_encaps") {
			    node.max_encaps = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "max_frames") {
			    node.max_frames = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "metameta_size") {
			    node.metameta_size = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "frame_size") {
			    node.frame_size = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;
			} else if (field_name == "num_counters") {
			    node.num_counters = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;
			} else if (field_name == "num_keys") {
			    node.num_keys = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;
			}
		    }
		}
	    }
	}
    }

    void _process_xdp2_parser(clang::VarDecl *var_decl)
    {
      plog::log(std::cout) << __func__ << " " << __FILE__ << ":" << __LINE__ << std::endl;
	const clang::Expr *initializer_expr = var_decl->getAnyInitializer();

	if (initializer_expr && initializer_expr->getStmtClass() ==
	    clang::Stmt::InitListExprClass) {
	  plog::log(std::cout) << __func__ << " Is InitListExprClass" << std::endl;
	    // Extracts current decl name from proto node structure
	    std::string name = var_decl->getNameAsString();
	    if (name.size() > 2 && name.substr(0, 2) == "__")
	      name.erase(name.begin(), std::next(name.begin(), 2));

	    xdp2gen::parser<G> node;

	    node.parser_name = name;

	    // // Extracts current analised InitListExpr
	    const clang::InitListExpr *initializer_list_expr =
		clang::dyn_cast<clang::InitListExpr>(initializer_expr);

	    plog::log(std::cout) << __func__ << " initializer_list_expr " << initializer_list_expr << std::endl;

	    // // Extracts current analised InitListDecl
	    if (auto recordType =
		initializer_list_expr->getType()->getAs<clang::RecordType>()) {
	      plog::log(std::cout) << __func__ << " recordType is true" << std::endl;
	      clang::RecordDecl *initializer_list_decl =
		recordType->getDecl();

	      _handle_init_list_expr_xdp2_parser(
						 initializer_list_expr, initializer_list_decl, node, name);

	      if (!node.parser_name.empty()) {
		node.dummy = false;
		node.ext = false;

		consumed_data.roots->push_back(node);
	      }
	    }
	} else if(initializer_expr) {
	  plog::log(std::cout) << "Not InitListExprClass " << initializer_expr->getStmtClassName() << std::endl;
	}
	else {
	  plog::log(std::cout) << "!initializer_expr" << std::endl;
	}
    }

    void _process_xdp2_parser_def(clang::VarDecl *var_decl)
    {
	const clang::Expr *initializer_expr = var_decl->getAnyInitializer();

	if (initializer_expr && initializer_expr->getStmtClass() ==
	    clang::Stmt::InitListExprClass) {
	    // Extracts current decl name from proto node structure
	    std::string name = var_decl->getNameAsString();

	    xdp2gen::parser<G> node;

	    // // Extracts current analised InitListExpr
	    const clang::InitListExpr *initializer_list_expr =
		clang::dyn_cast<clang::InitListExpr>(initializer_expr);

	    // // Extracts current analised InitListDecl
	    clang::RecordDecl *initializer_list_decl =
		initializer_list_expr->getType()
		    ->getAs<clang::RecordType>()
		    ->getDecl();

	    _handle_init_list_expr_xdp2_parser(
		initializer_list_expr, initializer_list_decl, node, name);

	    if (!node.parser_name.empty()) {
		node.dummy = true;
		node.ext = false;

		consumed_data.roots->push_back(node);
	    }
	}
    }

    template <typename N>
    void _handle_init_list_expr_parse_node(
	const clang::InitListExpr *initializer_list_expr, const clang::RecordDecl *records,
	N &node, std::string const &parent_name)
    {
	// Iterates trougth all Expr in init list
	for (size_t i = 0; i < initializer_list_expr->getNumInits(); ++i) {
	    // Current analised Expr
	    const clang::Expr *value = initializer_list_expr->getInits()[i];

	    // Current analised Decl
	    auto cur_record = std::next(records->field_begin(), i);

	    // Ignore clang internal casts
	    value = value->IgnoreImplicitAsWritten();

	    std::string field_name = cur_record->getNameAsString();

	    // In case of InitListExpr Expr be found
	    if (value->getStmtClass() == clang::Stmt::InitListExprClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::InitListExprClass" << std::endl;

		plog::log(std::cout) << "field_name: " << field_name << std::endl;

		const clang::InitListExpr *value_list =
		    clang::dyn_cast<clang::InitListExpr>(value);

		// Extracts declaration list of current initializer list
		clang::RecordDecl *record = value_list->getType()
						.getTypePtr()
						->getAs<clang::RecordType>()
						->getDecl();

		record->print(llvm::errs());

		plog::log(std::cout)
		    << "type of struct " << value_list->getType().getAsString()
		    << std::endl;

		plog::log(std::cout)
		  << "	- Recursively calling _handle_init_list_expr_parse_node " << __func__
		    << std::endl;

		// Performs a recursive call to process new exprInitList
		_handle_init_list_expr_parse_node(value_list, record, node,
						  field_name);

	    }

	    else if (value->getStmtClass() == clang::Stmt::UnaryOperatorClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::UnaryOperatorClass" << std::endl;

		plog::log(std::cout) << "field_name: " << field_name << std::endl;

		bool is_cur_field_of_interest =
		    (field_name == "text_name" ||
		     field_name == "proto_table" ||
		     field_name == "wildcard_node" ||
		     field_name == "tlv_wildcard_node" ||
		     field_name == "metadata_table" ||
		     field_name == "thread_funcs" ||
		     field_name == "tlv_proto_table" ||
		     field_name == "flag_fields_proto_table");

		if (is_cur_field_of_interest) {
		    const clang::UnaryOperator *unary_op =
			clang::dyn_cast<clang::UnaryOperator>(value);

		    auto handler = [&] (clang::VarDecl *var) {
			    if (field_name == "text_name") {
				node.parser_node = var->getNameAsString();
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name ==
				       "flag_fields_proto_table") {
				node.flag_fields_table = var->getNameAsString();
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "wildcard_node") {
				node.wildcard_proto_node =
				    var->getNameAsString();
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "tlv_wildcard_node") {
				node.tlv_wildcard_node = var->getNameAsString();
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "proto_table") {
				node.table = var->getNameAsString();
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "tlv_proto_table") {
				node.tlv_table = var->getNameAsString();
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    } else if (field_name == "metadata_table") {
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;

			    }

			    else if (field_name == "thread_funcs") {
				plog::log(std::cout)
				    << " variable name: "
				    << var->getNameAsString() << std::endl;
			    }
			    else if (field_name == "proto_def") {
				 node.parser_node = var->getNameAsString();
				 plog::log(std::cout)
				   << " variable name: "
				   << var->getNameAsString() << std::endl;
			    }
		    };
		    
		    if (clang::MemberExpr *member_expr_ref =
			    clang::dyn_cast<clang::MemberExpr>(
				unary_op->getSubExpr())) {
			if (clang::DeclRefExpr *value_decl_ref =
				clang::dyn_cast<clang::DeclRefExpr>(
				    member_expr_ref->getBase())) {
			     if (clang::VarDecl *var =
				    clang::dyn_cast<clang::VarDecl>(
					value_decl_ref->getDecl())) {
				 handler(var);
			     }
			}
		    }
		    if (clang::DeclRefExpr *value_decl_ref =
			    clang::dyn_cast<clang::DeclRefExpr>(
				unary_op->getSubExpr())) {
			if (clang::VarDecl *var =
				clang::dyn_cast<clang::VarDecl>(
				    value_decl_ref->getDecl())) {
			    handler(var);
			}
		    }
		}

	    }

	    else if (value->getStmtClass() == clang::Stmt::DeclRefExprClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::DeclRefExprClass" << std::endl;

		plog::log(std::cout) << "field_name: " << field_name << std::endl;

		const clang::DeclRefExpr *value_decl_ref =
		    clang::dyn_cast<clang::DeclRefExpr>(value);

		const clang::FunctionDecl *function =
		    clang::dyn_cast<clang::FunctionDecl>(
			value_decl_ref->getDecl());

		if (function) {
		    if (field_name == "extract_metadata") {
			std::string function_name = function->getNameAsString();

			node.metadata = function_name;

			plog::log(std::cout)
			    << "  variable name: " << function_name
			    << std::endl;

		    }

		    else if (field_name == "unknown_ret") {
			std::string function_name = function->getNameAsString();

			plog::log(std::cout)
			    << "  variable name: " << function_name
			    << std::endl;
		    }
		} else if (value->isIntegerConstantExpr(
			       cur_record->getASTContext())) {
		    auto v = value->getIntegerConstantExpr(
			cur_record->getASTContext());

		    if (field_name == "unknown_ret") {
			if (v) {
			    node.unknown_ret = v->getExtValue();
			    plog::log(std::cout)
				<< "unknown ret" << *node.unknown_ret
				<< std::endl;
			} else {
			    plog::log(std::cout)
				<< "	! NOT ABLE TO EVALUATE INTEGRAL LITERAL"
				<< std::endl;
			}
		    }
		}
	    }

	    else if (value->getStmtClass() == clang::Stmt::StringLiteralClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::StringLiteralClass" << std::endl;

		plog::log(std::cout) << "field_name: " << field_name << std::endl;

		const clang::StringLiteral *value_str =
		    clang::dyn_cast<clang::StringLiteral>(value);

		if (field_name == "func_text" &&
		    value_str->getString().str() != "((void*)0)" &&
		    value_str->getString().str() != "NULL") {
		    if (parent_name == "handle_proto")
			node.handler = value_str->getString().str();
		    else if (parent_name == "post_handle_proto")
			node.post_handler = value_str->getString().str();

		    plog::log(std::cout)
			<< "  variable name: " << node.handler << std::endl;
		}

	    } else if (value->getStmtClass() ==
		       clang::Stmt::IntegerLiteralClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::IntegerLiteralClass" << std::endl;

		plog::log(std::cout) << "field_name: " << field_name << std::endl;

		const clang::IntegerLiteral *int_literal =
		    clang::dyn_cast<clang::IntegerLiteral>(value);

		auto v =
		    value->getIntegerConstantExpr(cur_record->getASTContext());

		if (parent_name == "handle_proto") {
		    if (v) {
			if (field_name == "blockers") {
			    node.handler_blockers = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;
			} else if (field_name == "watchers") {
			    node.handler_watchers = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;
			}
		    }
		}

		if (parent_name == "config") {
		    if (v) {
			if (field_name == "max_loop") {
			    node.max_loop = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "max_non") {
			    node.max_non = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "max_plen") {
			    node.max_plen = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "max_c_pad") {
			    node.max_c_pad = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "disp_limit_exceed") {
			    node.disp_limit_exceed = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;

			} else if (field_name == "exceed_loop_cnt_is_err") {
			    node.exceed_loop_cnt_is_err = v->getZExtValue();
			    plog::log(std::cout)
				<< "  bit width: " << v->getBitWidth()
				<< std::endl;
			    plog::log(std::cout)
				<< "  literal_value: " << v->getZExtValue()
				<< std::endl;
			}
		    }
		}
	    }
	}
    }

    void _process_xdp2_parse_node(clang::VarDecl *var_decl)
    {
	const clang::Expr *initializer_expr = var_decl->getAnyInitializer();

	if (initializer_expr && initializer_expr->getStmtClass() ==
	    clang::Stmt::InitListExprClass) {
	    // Extracts current decl name from proto node structure
	    std::string name = var_decl->getNameAsString();

	    G *g = consumed_data.graph;

	    auto &&node = (*g)[insert_node_by_name(*g, name).first];

	    plog::log(std::cout)
		<< "GRAPH SIZE - " << g->vertex_set().size() << std::endl;

	    node.name = name;

	    // Extracts current analised InitListExpr
	    const clang::InitListExpr *initializer_list_expr =
		clang::dyn_cast<clang::InitListExpr>(initializer_expr);

	    // Extracts current analised InitListDecl
	    clang::RecordDecl *initializer_list_decl =
		initializer_list_expr->getType()
		    ->getAs<clang::RecordType>()
		    ->getDecl();

	    _handle_init_list_expr_parse_node(
		initializer_list_expr, initializer_list_decl, node, name);
	} else {
	  plog::log(std::cout) << "initializer_expr->getStmtClass() " << initializer_expr->getStmtClassName() << std::endl;
	}
    }

    void _process_xdp2_table(clang::VarDecl *var_decl,
			      std::vector<xdp2gen::table> &tbl)
    {
	plog::log(std::cout) << "_process_xdp2_table" << std::endl;
	var_decl->print(llvm::errs());
	const clang::Expr *initializer_expr = var_decl->getAnyInitializer();
	auto definition = var_decl->getDefinition();
	auto hasInit = var_decl->hasInit();

	if (initializer_expr && initializer_expr->getStmtClass() ==
	    clang::Stmt::InitListExprClass) {
	    // Extracts current decl name from proto node structure
	    std::string name = var_decl->getNameAsString();

	    // Extracts current analised InitListExpr
	    const clang::InitListExpr *initializer_list_expr =
		clang::dyn_cast<clang::InitListExpr>(initializer_expr);

	    xdp2gen::table node;

	    node.name = name.substr(2);

	    for (std::size_t i = 0; i < initializer_list_expr->getNumInits();
		 ++i) {
		clang::Expr *value = initializer_list_expr->getInits()[i];

		if (clang::InitListExpr *value_list =
			clang::dyn_cast<clang::InitListExpr>(value)) {
		    xdp2gen::table::entry table_entry;

		    plog::log(std::cout)
			<< "list size - " << value_list->getNumInits()
			<< std::endl;

		    // Extracts declaration list of current initializer list
		    clang::RecordDecl *records =
			value_list->getType()
			    .getTypePtr()
			    ->getAs<clang::RecordType>()
			    ->getDecl();

		    auto value_record = std::next(records->field_begin(), 0);

		    auto node_record = std::next(records->field_begin(), 1);

		    plog::log(std::cout)
			<< "Field 1 - " << value_record->getNameAsString()
			<< std::endl;

		    auto v = value_list->getInits()[0]->EvaluateKnownConstInt(
			value_record->getASTContext());

		    plog::log(std::cout)
			<< "	- PAREN EVALUATED VALUE: " << v.getSExtValue()
			<< std::endl;

		    unsigned int tmp_flag = v.getZExtValue();

		    plog::log(std::cout)
			<< "tmp_flag sizeof " << sizeof(tmp_flag)
			<< " int const bitwidth " << v.getBitWidth()
			<< std::endl;

		    auto to_hex = [](unsigned int mask) -> std::string {
			std::ostringstream ss;
			ss << "0x" << std::hex << mask;
			return ss.str();
		    };

		    table_entry.left = to_hex(tmp_flag);

		    plog::log(std::cout)
			<< "inverted flag " << table_entry.left << std::endl;

		    plog::log(std::cout)
			<< "Field 2 - " << node_record->getNameAsString()
			<< std::endl;

		    clang::UnaryOperator *unary_op =
			clang::dyn_cast<clang::UnaryOperator>(
			    value_list->getInits()[1]);

		    if (clang::DeclRefExpr *value_decl_ref =
			    clang::dyn_cast<clang::DeclRefExpr>(
				unary_op->getSubExpr())) {
			if (clang::VarDecl *var =
				clang::dyn_cast<clang::VarDecl>(
				    value_decl_ref->getDecl())) {
			    table_entry.right = var->getNameAsString();

			    plog::log(std::cout)
				<< " variable name: " << var->getNameAsString()
				<< std::endl;
			}
		    }

		    if (!table_entry.left.empty() &&
			!table_entry.right.empty()) {
			node.entries.push_back(table_entry);
		    }
		}
	    }

	    if (!node.entries.empty())
		tbl.push_back(node);
	}
    }

    template <typename N>
    void _handle_init_list_expr_flag_field_node(
	const clang::InitListExpr *initializer_list_expr, const clang::RecordDecl *records,
	N &node, std::string const &parent_name)
    {
	// Iterates trougth all Expr in init list
	for (size_t i = 0; i < initializer_list_expr->getNumInits(); ++i) {
	    // Current analised Expr
	    clang::Expr *value = initializer_list_expr->getInits()[i];

	    // Current analised Decl
	    auto cur_record = std::next(records->field_begin(), i);

	    // Ignore clang internal casts
	    value = value->IgnoreImplicitAsWritten();

	    std::string field_name = cur_record->getNameAsString();

	    // In case of InitListExpr Expr be found
	    if (value->getStmtClass() == clang::Stmt::InitListExprClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::InitListExprClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

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
		    << "  - Recursively calling _handle_init_list_expr_parse_node " << __func__
		    << std::endl;

		// Performs a recursive call to process new exprInitList
		_handle_init_list_expr_flag_field_node(value_list, record, node,
						       field_name);

	    }

	    else if (value->getStmtClass() == clang::Stmt::DeclRefExprClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::DeclRefExprClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

		clang::DeclRefExpr *value_decl_ref =
		    clang::dyn_cast<clang::DeclRefExpr>(value);

		clang::FunctionDecl *function =
		    clang::dyn_cast<clang::FunctionDecl>(
			value_decl_ref->getDecl());

		if (function) {
		    if (field_name == "extract_metadata") {
			std::string function_name = function->getNameAsString();

			node.metadata = function_name;

			plog::log(std::cout)
			    << "  variable name: " << function_name
			    << std::endl;
		    }
		}

	    }

	    else if (value->getStmtClass() == clang::Stmt::StringLiteralClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::StringLiteralClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

		clang::StringLiteral *value_str =
		    clang::dyn_cast<clang::StringLiteral>(value);

		if (field_name == "func_text" &&
		    value_str->getString().str() != "((void*)0)" &&
		    value_str->getString().str() != "NULL") {
		    if (parent_name == "handle_flag_field")
			node.handler = value_str->getString().str();

		    plog::log(std::cout)
			<< "  variable name: " << node.handler << std::endl;
		}
	    }
	}
    }

    void _process_xdp2_parse_flag_field_node(clang::VarDecl *var_decl)
    {
	const clang::Expr *initializer_expr = var_decl->getAnyInitializer();

	if (initializer_expr && initializer_expr->getStmtClass() ==
	    clang::Stmt::InitListExprClass) {
	    // Extracts current decl name from proto node structure
	    std::string name = var_decl->getNameAsString();

	    xdp2gen::flag_fields_node node;

	    node.name = name;
	    node.string_name = name;

	    // Extracts current analised InitListExpr
	    const clang::InitListExpr *initializer_list_expr =
		clang::dyn_cast<clang::InitListExpr>(initializer_expr);

	    // Extracts current analised InitListDecl
	    const clang::RecordDecl *initializer_list_decl =
		initializer_list_expr->getType()
		    ->getAs<clang::RecordType>()
		    ->getDecl();

	    _handle_init_list_expr_flag_field_node(
		initializer_list_expr, initializer_list_decl, node, name);

	    bool is_flag_field_node_empty =
		(node.name.empty() && node.string_name.empty());

	    // // Inserts node data in result vector
	    if (!is_flag_field_node_empty)
		consumed_data.flag_fields_nodes.push_back(node);
	}
    }

    template <typename N>
    void
    _handle_init_list_expr_tlv_node(const clang::InitListExpr *initializer_list_expr,
				    clang::RecordDecl *records, N &node,
				    std::string const &parent_name)
    {
	// Iterates trougth all Expr in init list
	for (size_t i = 0; i < initializer_list_expr->getNumInits(); ++i) {
	    // Current analised Expr
	    clang::Expr *value = initializer_list_expr->getInits()[i];

	    // Current analised Decl
	    auto cur_record = std::next(records->field_begin(), i);

	    // Ignore clang internal casts
	    value = value->IgnoreImplicitAsWritten();

	    std::string field_name = cur_record->getNameAsString();

	    // In case of InitListExpr Expr be found
	    if (value->getStmtClass() == clang::Stmt::InitListExprClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::InitListExprClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

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
		    << "  - Recursively calling _handle_init_list_expr_parse_node " << __func__
		    << std::endl;

		// Performs a recursive call to process new exprInitList
		_handle_init_list_expr_tlv_node(value_list, record, node,
						field_name);

	    }

	    else if (value->getStmtClass() == clang::Stmt::UnaryOperatorClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::UnaryOperatorClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

		bool is_cur_field_of_interest =
		    (field_name == "proto_tlv_node" ||
		     field_name == "overlay_table" ||
		     field_name == "overlay_wildcard_node");

		clang::VarDecl *var = nullptr;

		if (is_cur_field_of_interest) {
		    clang::UnaryOperator *unary_op =
			clang::dyn_cast<clang::UnaryOperator>(value);

		    clang::DeclRefExpr *value_decl_ref =
			clang::dyn_cast<clang::DeclRefExpr>(
			    unary_op->getSubExpr());

		    if (value_decl_ref) {
			var = clang::dyn_cast<clang::VarDecl>(
			    value_decl_ref->getDecl());
		    }
		}

		if (var)
		    if (field_name == "proto_tlv_node") {
			node.string_name = var->getNameAsString();
			plog::log(std::cout)
			    << " variable name: " << var->getNameAsString()
			    << std::endl;

		    } else if (field_name == "overlay_table") {
			node.overlay_table = var->getNameAsString();
			plog::log(std::cout)
			    << " variable name: " << var->getNameAsString()
			    << std::endl;

		    } else if (field_name == "overlay_wildcard_node") {
			node.wildcard_node = var->getNameAsString();
			plog::log(std::cout)
			    << " variable name: " << var->getNameAsString()
			    << std::endl;
		    }

	    }

	    else if (value->getStmtClass() == clang::Stmt::DeclRefExprClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::DeclRefExprClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

		clang::DeclRefExpr *value_decl_ref =
		    clang::dyn_cast<clang::DeclRefExpr>(value);

		clang::FunctionDecl *function =
		    clang::dyn_cast<clang::FunctionDecl>(
			value_decl_ref->getDecl());

		if (function) {
		    if (field_name == "extract_metadata") {
			std::string function_name = function->getNameAsString();

			node.metadata = function_name;

			plog::log(std::cout)
			    << "  variable name: " << function_name
			    << std::endl;

		    }

		    else if (field_name == "unknown_overlay_ret") {
			std::string function_name = function->getNameAsString();

			node.unknown_overlay_ret = function_name;

			plog::log(std::cout)
			    << "  variable name: " << function_name
			    << std::endl;
		    }
		}

	    }

	    else if (value->getStmtClass() == clang::Stmt::StringLiteralClass) {
		plog::log(std::cout)
		    << "DEBUG: "
		    << "clang::Stmt::StringLiteralClass" << std::endl;

		plog::log(std::cout) << field_name << std::endl;

		clang::StringLiteral *value_str =
		    clang::dyn_cast<clang::StringLiteral>(value);

		if (field_name == "func_text" &&
		    value_str->getString().str() != "((void*)0)" &&
		    value_str->getString().str() != "NULL") {
		    if (parent_name == "handle_tlv")
			node.handler = value_str->getString().str();

		    plog::log(std::cout)
			<< "  variable name: " << node.handler << std::endl;
		}
	    }
	}
    }

    void _process_xdp2_parse_tlv_node(clang::VarDecl *var_decl)
    {
	const clang::Expr *initializer_expr = var_decl->getAnyInitializer();

	if (initializer_expr->getStmtClass() ==
	    clang::Stmt::InitListExprClass) {
	    // Extracts current decl name from proto node structure
	    std::string name = var_decl->getNameAsString();

	    xdp2gen::tlv_node node;

	    node.name = name;

	    // Extracts current analised InitListExpr
	    const clang::InitListExpr *initializer_list_expr =
		clang::dyn_cast<clang::InitListExpr>(initializer_expr);

	    // Extracts current analised InitListDecl
	    clang::RecordDecl *initializer_list_decl =
		initializer_list_expr->getType()
		    ->getAs<clang::RecordType>()
		    ->getDecl();

	    _handle_init_list_expr_tlv_node(initializer_list_expr,
					    initializer_list_decl, node, name);

	    bool is_tlv_node_empty =
		(node.name.empty() && node.string_name.empty());

	    // // Inserts node data in result vector
	    if (!is_tlv_node_empty)
		consumed_data.tlv_nodes.push_back(node);
	}
    }
};

template <typename G>
class extract_graph_constants : public clang::SyntaxOnlyAction {
    using base = clang::SyntaxOnlyAction;

private:
    graph_info<G> &consumed_data;

public:
    extract_graph_constants(graph_info<G> &consumed_data)
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
	    << "CreateASTConsumer xdp2_graph_consumer" << std::endl;
	auto lo = ci.getLangOpts();
	if (lo.CPlusPlus) {
	  plog::log(std::cout) << "C++" << std::endl;
	} /*else if (lo.C89) {
	  plog::log(std::cout) << "C89" << std::endl;
	  } */else if (lo.C99) {
	  plog::log(std::cout) << "C99" << std::endl;
	} else if (lo.C11) {
	  plog::log(std::cout) << "C11" << std::endl;
	} else if (lo.C17) {
	  plog::log(std::cout) << "C17" << std::endl;
	}


	return std::make_unique<xdp2_graph_consumer<G>>(consumed_data);
    }
};

//
// Custom Factory for parameters passing
//
template <typename G>
std::unique_ptr<clang::tooling::FrontendActionFactory>
extract_graph_constants_factory(graph_info<G> &consumed_data)
{
    class simple_frontend_action_factory
	: public clang::tooling::FrontendActionFactory {
    public:
	simple_frontend_action_factory(graph_info<G> &options)
	    : Options(options)
	{
	}

	virtual std::unique_ptr<clang::FrontendAction> create() override
	{
	    return std::make_unique<extract_graph_constants<G>>(Options);
	}

    private:
	graph_info<G> &Options;
    };

    return std::unique_ptr<clang::tooling::FrontendActionFactory>(
	new simple_frontend_action_factory(consumed_data));
}

#endif // !PROTO_NODES_H
