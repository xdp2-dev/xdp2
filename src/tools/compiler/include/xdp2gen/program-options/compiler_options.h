#ifndef PROTO_XDP2GEN_PROGRAM_OPTIONS_H
#define PROTO_XDP2GEN_PROGRAM_OPTIONS_H

#include <iostream>
#include <optional>

#include <boost/program_options.hpp>

#include "xdp2gen/program-options/log_handler.h"

namespace xdp2gen
{

class xdp2_input_flag_handler {
public:
    xdp2_input_flag_handler(int argc, char **argv)
        : _op_desc("Options")
    {
        try {
            _op_desc.add_options()("help,h", "Help")(
                "input,i", boost::program_options::value<std::string>(),
                ".c file input - Required")(
                "ll,l", boost::program_options::value<std::string>(),
                ".ll IR file correspondent to the input .c file - only required for .json output")(
                "output,o", boost::program_options::value<std::string>(),
                "Output file, must include supported extension: .json, .c, .xdp.h, .dot - Required")(
                "verbose,v",
                "Output steps taken by the compiler during compilation.")(
                "disable-warnings", "Disable compilation warnings.")(
                "include,I",
                boost::program_options::value<std::vector<std::string>>()
                    ->multitoken(),
                "Additional include directories to use")(
                "resource-path", boost::program_options::value<std::string>(), "CLANG's resource path");

            boost::program_options::store(
                boost::program_options::parse_command_line(argc, argv,
                                                           _op_desc),
                _vm);
            boost::program_options::notify(_vm);

        } catch (boost::program_options::error const &ex) {
            plog::log(std::cerr) << ex.what() << std::endl;
        }
    }

    bool is_in_well_formed_state()
    {
        auto pretty_print_bool = [](bool val) -> std::string {
            return val ? "true" : "false";
        };

        bool result = false;

        if (verify_flag_existence("help"))
            return result;

        std::string input_file;
        std::string output_file;

        bool are_basic_flags_defined = get_flag_value("input", input_file) &&
                                       get_flag_value("output", output_file);

        plog::log(std::cout)
            << "are_basic_flags_defined "
            << pretty_print_bool(are_basic_flags_defined) << std::endl;

        if (are_basic_flags_defined) {
            plog::log(std::cout)
                << "input_file " << _vm["input"].as<std::string>() << std::endl;
            plog::log(std::cout) << "output_file " << output_file << std::endl;

            bool are_names_valid = _validate_file_name(input_file) &&
                                   _validate_file_name(output_file);

            plog::log(std::cout)
                << "are_names_valid " << pretty_print_bool(are_names_valid)
                << std::endl;

            if (are_names_valid) {
                std::string output_ext = _get_file_extension(output_file);

                bool are_ext_valid =
                    _validate_input_ext(_get_file_extension(input_file)) &&
                    _validate_output_ext(output_ext);

                plog::log(std::cout)
                    << "are_ext_valid " << pretty_print_bool(are_ext_valid)
                    << std::endl;

                if (are_ext_valid) {
                    if (output_ext == ".json") {
                        std::string ll_file;

                        bool is_ll_flag_set = get_flag_value("ll", ll_file);

                        if (is_ll_flag_set) {
                            bool is_ll_file_name_fine =
                                _validate_file_name(ll_file);

                            bool is_ll_file_correct =
                                is_ll_file_name_fine &&
                                _validate_ll_ext(_get_file_extension(ll_file));

                            if (is_ll_file_correct)
                                result = true;
                        }

                    } else
                        result = true;
                }
            }
        }

        return result;
    }

    bool verify_flag_existence(std::string const &flag)
    {
        bool result = false;

        if (_vm.count(flag)) {
            result = true;
        }

        return result;
    }

    bool verify_flag_existence(std::string const &&flag)
    {
        bool result = false;

        if (_vm.count(flag)) {
            result = true;
        }

        return result;
    }

    template <typename T>
    bool get_flag_value(std::string const &flag, T &dst)
    {
        bool result = false;

        if (_vm.count(flag)) {
            dst = _vm[flag].as<T>();
            result = true;
        }

        return result;
    }

    template <typename T>
    bool get_flag_value(std::string const &&flag, T &dst)
    {
        bool result = false;

        if (_vm.count(flag)) {
            dst = _vm[flag].as<T>();
            result = true;
        }

        return result;
    }

    template <typename T>
    optional<T> get_flag_value(std::string const &flag)
    {
        optional<T> result;

        if (_vm.count(flag)) {
            result = _vm[flag].as<T>();
        }

        return result;
    }

    template <typename T>
    optional<T> get_flag_value(std::string const &&flag)
    {
        optional<T> result;

        if (_vm.count(flag)) {
            result = _vm[flag].as<T>();
        }

        return result;
    }

    friend inline std::ostream &operator<<(std::ostream &os,
                                           xdp2_input_flag_handler const &pifh)
    {
        os << pifh._op_desc << std::endl;

        return os;
    }

private:
    std::string _get_file_extension(std::string const &file)
    {
        std::size_t ext_init_pos = file.rfind(".");

        if (ext_init_pos <= file.length() - 2 &&
            file[ext_init_pos + 1] == 'h') {
std:
            size_t tmp_pos = file.rfind(".", ext_init_pos - 1);

            if (tmp_pos != std::string::npos)
                ext_init_pos = tmp_pos;
        }

        return file.substr(ext_init_pos, std::string::npos);
    }

    bool _validate_file_name(std::string const &file)
    {
        bool result = false;

        plog::log(std::cout) << "file " << file << std::endl;

        std::size_t ext_init_pos = file.rfind(".");

        plog::log(std::cout) << "ext_init_pos " << ext_init_pos << std::endl;

        bool is_there_an_extension = ext_init_pos != std::string::npos &&
                                     ext_init_pos < file.length() - 1;

        plog::log(std::cout)
            << "is_there_an_extension " << is_there_an_extension << std::endl;

        bool is_there_a_name = ext_init_pos != std::string::npos &&
                               ext_init_pos > 0 &&
                               file[ext_init_pos - 1] != '/';

        plog::log(std::cout)
            << "is_there_a_name " << is_there_a_name << std::endl;

        if (is_there_a_name && is_there_an_extension)
            result = true;

        return result;
    }

    bool _validate_input_ext(std::string const &ext)
    {
        if (ext == ".c")
            return true;
        else
            return false;
    }

    bool _validate_output_ext(std::string const &ext)
    {
        if (ext == ".json" || ext == ".c" || ext == ".xdp.h" ||
            ext == ".dot" || ext == ".p4")
            return true;
        else
            return false;
    }

    bool _validate_ll_ext(std::string const &ext)
    {
        if (ext == ".ll")
            return true;
        else
            return false;
    }

    boost::program_options::variables_map _vm;
    boost::program_options::options_description _op_desc;
};

}

#endif
