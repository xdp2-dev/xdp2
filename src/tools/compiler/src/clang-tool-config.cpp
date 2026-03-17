// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2024 SiXDP2 Inc.
 *
 * ClangTool configuration implementation.
 */

#include "xdp2gen/clang-tool-config.h"
#include "xdp2gen/program-options/log_handler.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

// Stringification macro for compile-time paths
#define XDP2_STRINGIFY_A(X) #X
#define XDP2_STRINGIFY(X) XDP2_STRINGIFY_A(X)

namespace xdp2gen {

clang_tool_config clang_tool_config::from_environment()
{
    clang_tool_config config;

    // Resource directory from compile-time macro
#ifdef XDP2_CLANG_RESOURCE_PATH
    config.resource_dir = XDP2_STRINGIFY(XDP2_CLANG_RESOURCE_PATH);
#endif

    // System include paths from environment variables
    // These are set by the Nix derivation for proper header resolution
    if (char const *val = std::getenv("XDP2_C_INCLUDE_PATH")) {
        config.clang_include_path = val;
    }
    if (char const *val = std::getenv("XDP2_GLIBC_INCLUDE_PATH")) {
        config.glibc_include_path = val;
    }
    if (char const *val = std::getenv("XDP2_LINUX_HEADERS_PATH")) {
        config.linux_headers_path = val;
    }

    return config;
}

bool clang_tool_config::has_system_includes() const
{
    return clang_include_path.has_value() ||
           glibc_include_path.has_value() ||
           linux_headers_path.has_value();
}

std::string clang_tool_config::to_string() const
{
    std::ostringstream oss;
    oss << "clang_tool_config {\n";

    if (resource_dir) {
        oss << "  resource_dir: " << *resource_dir << "\n";
    } else {
        oss << "  resource_dir: (not set)\n";
    }

    if (clang_include_path) {
        oss << "  clang_include_path: " << *clang_include_path << "\n";
    }
    if (glibc_include_path) {
        oss << "  glibc_include_path: " << *glibc_include_path << "\n";
    }
    if (linux_headers_path) {
        oss << "  linux_headers_path: " << *linux_headers_path << "\n";
    }

    oss << "}";
    return oss.str();
}

void apply_config(clang::tooling::ClangTool &tool,
                  clang_tool_config const &config)
{
    plog::log(std::cout) << "[clang-tool-config] Applying configuration:\n"
                         << config.to_string() << std::endl;

    // Resource directory (required for clang builtins like stddef.h)
    if (config.resource_dir) {
        tool.appendArgumentsAdjuster(
            clang::tooling::getInsertArgumentAdjuster(
                {"-resource-dir", config.resource_dir->c_str()},
                clang::tooling::ArgumentInsertPosition::BEGIN));
    }

    // System include paths
    // Add in reverse order at BEGIN so final order is correct:
    // clang builtins -> glibc -> linux headers
    //
    // This ensures macros like __cpu_to_be16() from linux headers
    // are properly resolved when parsing proto table initializers.

    if (config.linux_headers_path) {
        plog::log(std::cout) << "[clang-tool-config] Adding -isystem "
                             << *config.linux_headers_path << std::endl;
        tool.appendArgumentsAdjuster(
            clang::tooling::getInsertArgumentAdjuster(
                {"-isystem", config.linux_headers_path->c_str()},
                clang::tooling::ArgumentInsertPosition::BEGIN));
    }

    if (config.glibc_include_path) {
        plog::log(std::cout) << "[clang-tool-config] Adding -isystem "
                             << *config.glibc_include_path << std::endl;
        tool.appendArgumentsAdjuster(
            clang::tooling::getInsertArgumentAdjuster(
                {"-isystem", config.glibc_include_path->c_str()},
                clang::tooling::ArgumentInsertPosition::BEGIN));
    }

    if (config.clang_include_path) {
        plog::log(std::cout) << "[clang-tool-config] Adding -isystem "
                             << *config.clang_include_path << std::endl;
        tool.appendArgumentsAdjuster(
            clang::tooling::getInsertArgumentAdjuster(
                {"-isystem", config.clang_include_path->c_str()},
                clang::tooling::ArgumentInsertPosition::BEGIN));
    }
}

} // namespace xdp2gen
