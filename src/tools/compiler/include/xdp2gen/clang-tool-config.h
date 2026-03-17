// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2024 SiXDP2 Inc.
 *
 * ClangTool configuration utilities.
 *
 * Provides a unified configuration structure and helper functions
 * to ensure consistent ClangTool setup across all uses. This fixes
 * the Nix build issue where extract_struct_constants created a
 * ClangTool without system include paths.
 *
 * See: documentation/nix/optimized_parser_extraction_defect.md
 */

#ifndef XDP2GEN_CLANG_TOOL_CONFIG_H
#define XDP2GEN_CLANG_TOOL_CONFIG_H

#include <optional>
#include <string>

#include <clang/Tooling/Tooling.h>

namespace xdp2gen {

/*
 * Configuration for ClangTool instances.
 *
 * Encapsulates all paths and settings needed for consistent
 * ClangTool behavior across different environments (Ubuntu, Nix).
 */
struct clang_tool_config {
    // Clang resource directory (stddef.h, stdarg.h, etc.)
    std::optional<std::string> resource_dir;

    // Clang builtin headers path (-isystem)
    std::optional<std::string> clang_include_path;

    // Glibc headers path (-isystem)
    std::optional<std::string> glibc_include_path;

    // Linux kernel headers path (-isystem)
    std::optional<std::string> linux_headers_path;

    /*
     * Load configuration from environment variables.
     *
     * Reads:
     *   XDP2_C_INCLUDE_PATH     -> clang_include_path
     *   XDP2_GLIBC_INCLUDE_PATH -> glibc_include_path
     *   XDP2_LINUX_HEADERS_PATH -> linux_headers_path
     *
     * The resource_dir is set from XDP2_CLANG_RESOURCE_PATH macro if defined.
     */
    static clang_tool_config from_environment();

    /*
     * Check if any system include paths are configured.
     */
    bool has_system_includes() const;

    /*
     * Format configuration for debug logging.
     */
    std::string to_string() const;
};

/*
 * Apply configuration to a ClangTool instance.
 *
 * Adds argument adjusters for:
 *   -resource-dir (if configured)
 *   -isystem paths (clang builtins, glibc, linux headers)
 *
 * Order: resource-dir first, then isystem paths.
 * The isystem paths are added in reverse order at BEGIN so the
 * final order is: clang builtins, glibc, linux headers.
 */
void apply_config(clang::tooling::ClangTool &tool,
                  clang_tool_config const &config);

} // namespace xdp2gen

#endif // XDP2GEN_CLANG_TOOL_CONFIG_H
