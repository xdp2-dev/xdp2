#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class


std::vector<std::string> macros_defined{};

  auto macros = std::vector<std::string>{
    "XDP2_DECL_PARSE_NODE(node)",
    "XDP2_DECL_TLVS_PARSE_NODE(node)",
    "XDP2_MAKE_PROTO_TABLE(table_name, ...)",
    "XDP2_MAKE_PARSER_TABLE(table_name, ...)", //TODO: add interpretation
    "XDP2_MAKE_TLV_TABLE(table_name, ...)",
    "XDP2_MAKE_FLAG_FIELDS_TABLE(table_name, ...)",
    "XDP2_MAKE_TLV_PARSE_NODE(node, proto_tlv_node, metadata, handler)",
    "XDP2_MAKE_FLAG_FIELD_PARSE_NODE(node, name, metadata)",
    "XDP2_MAKE_PARSE_NODE(node, name, metadata, pointer, table)",
    "XDP2_MAKE_OVERLAY_PARSE_NODE(node, name, metadata, handler, overlay_node)",
    "XDP2_MAKE_TLVS_PARSE_NODE(node, name, metadata, pointer, "
    "table)",
    "XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(node, name, metadata, pointer, table, flag_fields_table)",
    "XDP2_MAKE_FLAG_FIELDS_OVERLAY_PARSE_NODE(node, name, metadata, pointer, overlay_node, flag_fields_table)",
    "XDP2_MAKE_LEAF_FLAG_FIELDS_PARSE_NODE(node, name, metadata, pointer, flag_fields_table, post_flag_handle)",
    "XDP2_MAKE_LEAF_PARSE_NODE(node, name, metadata, pointer)",
    "XDP2_MAKE_LEAF_TLVS_PARSE_NODE(node, name, metadata, "
    "pointer, table)",
    "XDP2_PARSER_ADD(name, description, node_addr)",
    "XDP2_PARSER_EXT(parser, description, node_addr)",
    "XDP2_PARSER(parser, description, node_addr)",
    "XDP2_PARSER_XDP(parser, description, node_addr)",
	"XDP2_MAKE_TLV_OVERLAY_PARSE_NODE(node_name, "
	"metadata_func, handler_func, "
	"overlay_table, overlay_type_func, "
	"unknown_overlay_ret, overlay_wildcard_node)",
  };

class custom_directives_hooks
:   public boost::wave::context_policies::default_preprocessing_hooks
{
public:

// Ignores #include directives
    template <typename ContextType>
    bool found_include_directive(
        const ContextType &context,
        const std::string &filename, bool include_next)
    {
        return true;
    }

// macro definition hooks
    template <typename ContextT, typename TokenT, typename ParametersT, typename DefinitionT>
    void defined_macro(
        ContextT const &ctx, TokenT const &name,
        bool is_functionlike, ParametersT const &parameters,
        DefinitionT const &definition, bool is_predefined)
    {
        if(!is_predefined && is_functionlike){
            std::string macro_name = name.get_value().c_str();
            macros_defined.push_back(macro_name);
        }
    }
};

void parse_file(std::string filename)
{
    boost::wave::util::file_position_type current_position;

    try {
        std::ifstream instream(filename);
        std::string instring;

        if (!instream.is_open()) {
            std::cerr << "Could not open input file " << std::endl;
        }
        instream.unsetf(std::ios::skipws);
        instring = std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                                std::istreambuf_iterator<char>());

        typedef boost::wave::cpplexer::lex_token<> token_type;

        typedef boost::wave::cpplexer::lex_iterator<token_type> lex_iterator_type;

        typedef boost::wave::context<std::string::iterator, lex_iterator_type,
                boost::wave::iteration_context_policies::load_file_to_string,
                custom_directives_hooks
            > context_type;

        boost::wave::language_support const lang_opts =
        (boost::wave::language_support)(
            boost::wave::support_option_variadics |
            boost::wave::support_option_long_long |
            boost::wave::support_option_no_character_validation |
            boost::wave::support_option_convert_trigraphs |
            boost::wave::support_option_insert_whitespace);
        context_type ctx (instring.begin(), instring.end(), filename.c_str());
        ctx.set_language(lang_opts);

        context_type::iterator_type first = ctx.begin();
        context_type::iterator_type last = ctx.end();

        while (first != last) {
            current_position = (*first).get_position();
            ++first;
        }
    }
    catch (boost::wave::cpp_exception const& e) {
    // some preprocessing error
        std::cerr
            << e.file_name() << "(" << e.line_no() << "): "
            << e.description() << std::endl;
    }
    catch (std::exception const& e) {
    // use last recognized token to retrieve the error position
        std::cerr
            << current_position.get_file()
            << "(" << current_position.get_line() << "): "
            << "exception caught: " << e.what()
            << std::endl;
    }
    catch (...) {
    // use last recognized token to retrieve the error position
        std::cerr
            << current_position.get_file()
            << "(" << current_position.get_line() << "): "
            << "unexpected exception caught." << std::endl;
    }

}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <test_file>\n"
        << "\n When test_file is specified the macros in the test_file\n"
        << "will be compared if exist in macros vector inside this code\n";
    return 1;
  }
    parse_file(argv[1]);

    for (auto macro_defined : macros_defined){

        if (macro_defined.find("__") == std::string::npos){
            if (macro_defined.find("XDP2_MAKE") != std::string::npos){

                //find if macros defined on parser.h exist on xdp2-compiler
                bool found = false;
                for (auto macro : macros){
		    //extracts macro name
                    macro = macro.substr(0, macro.find("(", 0));

                    if (macro == macro_defined)
                        found = true;
                }
                if(!found)
                    std::cerr << macro_defined << " Not exist" << std::endl;
            }
        }
    }

    return 0;
}
