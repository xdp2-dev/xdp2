# XDP2 C++ Style Guide

This document describes the C++ coding conventions and style guidelines for the XDP2 project. These guidelines are derived from the existing codebase patterns and should be followed for all C++ contributions.

## Table of Contents

1. [File Organization](#file-organization)
2. [Naming Conventions](#naming-conventions)
3. [Formatting](#formatting)
4. [Include Directives](#include-directives)
5. [Comments and Documentation](#comments-and-documentation)
6. [Classes and Structs](#classes-and-structs)
7. [Memory Management](#memory-management)
8. [Error Handling](#error-handling)
9. [Templates and Metaprogramming](#templates-and-metaprogramming)
10. [Const Correctness](#const-correctness)
11. [Modern C++ Features](#modern-c-features)
12. [Macros](#macros)
13. [Debugging](#debugging)
14. [Assertions](#assertions)
15. [Testing](#testing)

---

## File Organization

### Directory Structure

```
src/tools/compiler/
├── src/           # Implementation files (.cpp)
├── include/
│   └── xdp2gen/   # Public headers
│       ├── ast-consumer/   # Clang AST consumer headers
│       ├── llvm/           # LLVM IR analysis headers
│       ├── program-options/# CLI argument handling
│       ├── json/           # JSON metadata specs
│       └── clang-ast/      # Clang AST metadata
```

### File Extensions

| Extension | Usage |
|-----------|-------|
| `.h`      | Traditional C++ headers |
| `.hpp`    | Alternative C++ headers |
| `.h2`     | Cppfront source files |
| `.cpp`    | Implementation files |

### License Header

All source files must begin with the BSD-2-Clause-FreeBSD SPDX license header:

```cpp
// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2024 SiXDP2 Inc.
 *
 * Authors: [Author Name] <email@domain.com>
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
```

---

## Naming Conventions

### General Rules

Use `snake_case` consistently throughout the codebase. Avoid `PascalCase` or `camelCase`.

### Namespaces

Use hierarchical lowercase namespaces with `::` separators:

```cpp
namespace xdp2gen {
namespace ast_consumer {
    // ...
}
}
```

### Classes and Structs

Use `snake_case` for class and struct names:

```cpp
// Good
class llvm_graph { };
struct tlv_node { };
class xdp2_proto_node_consumer { };

// Bad
class LlvmGraph { };
struct TlvNode { };
```

### Functions

Use `snake_case` for all functions:

```cpp
// Good
void transfer_data_from_proto_node();
auto find_table_by_name(std::string const &name);
int extract_struct_constants();

// Bad
void TransferDataFromProtoNode();
auto findTableByName();
```

### Variables

Use `snake_case` for variables:

```cpp
// Good
std::string proto_node_data;
size_t curr_size;
std::vector<node_type> index_node_map;

// Bad
std::string protoNodeData;
size_t currSize;
```

### Member Variables

- Use plain `snake_case` for public members
- Prefix with single underscore `_` for protected members
- Prefix with double underscore `__` for private helper methods

```cpp
class example_class {
public:
    std::string public_data;

protected:
    std::string _protected_data;

private:
    std::string private_data;

    void __private_helper();  // Private helper method
};
```

### Type Aliases

- Use `_t` suffix for type aliases
- Use `_ref` suffix for reference wrapper types

```cpp
using python_object_t = std::unique_ptr<PyObject, python_object_deleter_t>;
using tlv_node_ref = std::reference_wrapper<tlv_node>;
using tlv_node_ref_const = std::reference_wrapper<tlv_node const>;
```

### Template Constants

Use `_v` suffix for variable templates:

```cpp
template <typename... Ts>
constexpr auto args_size_v = args_size<Ts...>::value;

template <typename Test, typename... Args>
constexpr bool one_of_v = one_of<Test, Args...>::value;
```

### Files

Use lowercase with hyphens for multi-word file names:

```
proto-tables.h
graph-consumer.cpp
program-options.h
```

---

## Formatting

### Indentation

Use 4 spaces for indentation. Do not use tabs.

```cpp
if (condition) {
    do_something();
    if (another_condition) {
        do_something_else();
    }
}
```

### Braces

Opening braces go on the same line:

```cpp
// Good
if (condition) {
    // ...
}

class my_class {
    // ...
};

void function() {
    // ...
}

// Bad
if (condition)
{
    // ...
}
```

### Line Length

Aim for 80-100 characters per line. Maximum 120 characters. Break long lines logically:

```cpp
// Good - break at logical points
if ((type == "const struct xdp2_proto_def" ||
     type == "const struct xdp2_proto_tlvs_def" ||
     type == "const struct xdp2_proto_flag_fields_def") &&
    var_decl->hasInit()) {
    // ...
}

// Good - break function parameters
void long_function_name(
    std::string const &first_parameter,
    std::vector<int> const &second_parameter,
    std::optional<std::string> third_parameter);
```

### Spacing

```cpp
// Space after control flow keywords
if (condition)
while (condition)
for (auto &item : container)

// No space before function call parentheses
function_name()
object.method()

// Space around binary operators
a + b
x == y
ptr != nullptr

// Space after commas
function(arg1, arg2, arg3)

// Space after semicolons in for loops
for (int i = 0; i < n; ++i)
```

### Pointers and References

Place the `*` and `&` with the type, not the variable name:

```cpp
// Good
int *ptr;
std::string const &ref;
T const *const_ptr;

// Bad
int* ptr;
int*ptr;
std::string const& ref;
```

---

## Include Directives

### Include Order

Organize includes in the following order, separated by blank lines:

1. Standard library headers
2. System headers
3. Third-party library headers (Boost, LLVM, Clang)
4. Project headers

```cpp
// Standard library
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>

// System headers
#include <arpa/inet.h>

// Third-party libraries
#include <boost/wave.hpp>
#include <boost/program_options.hpp>

#include <llvm/ADT/Twine.h>
#include <llvm/IR/Module.h>

#include <clang/Tooling/Tooling.h>

// Project headers
#include "xdp2gen/graph.h"
#include "xdp2gen/python_generators.h"
#include "xdp2gen/ast-consumer/graph_consumer.h"
```

### Header Guards

Use traditional `#ifndef` guards. `#pragma once` is acceptable but not preferred:

```cpp
#ifndef XDP2GEN_AST_CONSUMER_GRAPH_H
#define XDP2GEN_AST_CONSUMER_GRAPH_H

// Header content

#endif // XDP2GEN_AST_CONSUMER_GRAPH_H
```

### Compiler Version Compatibility

Handle compiler version differences with conditional compilation:

```cpp
#ifdef __GNUC__
#if __GNUC__ > 6
#include <optional>
namespace xdp2gen { using std::optional; }
#else
#include <experimental/optional>
namespace xdp2gen { using std::experimental::optional; }
#endif
#endif
```

---

## Comments and Documentation

### File-Level Documentation

After the license header, include a brief description of the file's purpose:

```cpp
// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/* ... license ... */

/*
 * This file implements the LLVM IR pattern matching functionality
 * for extracting TLV (Type-Length-Value) structures from compiled
 * protocol definitions.
 */
```

### Block Comments

Use `/* */` for multi-line documentation:

```cpp
/*
 * The following pattern matches a calculation of a tlv parameter value
 * that is performed by just loading the value of a memory region at an
 * offset of a struct pointer as the first argument of the function.
 *
 * This pattern would match the following LLVM block:
 * `%2 = getelementptr inbounds %struct.tcp_opt, ptr %0, i64 0, i32 1`
 */
```

### Inline Comments

Use `//` for single-line comments:

```cpp
// Process all declarations in the group
for (auto *decl : D) {
    process(decl);
}
```

### TODO Comments

Use consistent TODO format:

```cpp
// TODO: insert the asserts and exceptions later?
// TODO: maybe insert sorted to avoid repetition?
```

### Patch Documentation

When adding patches or debug code, use descriptive tags:

```cpp
// [nix-patch] Process ALL declarations in the group, not just single decls.
// XDP2_MAKE_PROTO_TABLE creates TWO declarations which may be grouped.

// [nix-debug] Added for troubleshooting segfault issue
```

---

## Classes and Structs

### Struct for Data

Use `struct` for plain data holders with public members:

```cpp
struct xdp2_proto_node_extract_data {
    std::string decl_name;
    std::optional<std::string> name;
    std::optional<std::size_t> min_len;
    std::optional<std::string> len;

    friend inline std::ostream &
    operator<<(std::ostream &os, xdp2_proto_node_extract_data const &data) {
        // Output implementation
        return os;
    }
};
```

### Class for Behavior

Use `class` when encapsulation is needed:

```cpp
class llvm_graph {
public:
    using node_type = ::llvm::Value const *;

    // Public interface
    size_t add_node(node_type node);
    bool has_edge(size_t from, size_t to) const;

private:
    static constexpr size_t npos = -1;

    ::llvm::BasicBlock const *bb_ptr = nullptr;
    size_t curr_size = 0;
    std::vector<node_type> index_node_map;

    size_t __increase_graph(node_type const &n, node_type ptr);
};
```

### Consumer Pattern

Inherit from Clang's `ASTConsumer` for AST processing:

```cpp
class xdp2_proto_node_consumer : public clang::ASTConsumer {
private:
    std::vector<xdp2_proto_node_extract_data> &consumed_data;

public:
    explicit xdp2_proto_node_consumer(
        std::vector<xdp2_proto_node_extract_data> &consumed_data)
        : consumed_data{ consumed_data } {}

    bool HandleTopLevelDecl(clang::DeclGroupRef D) override {
        // Process declarations
        return true;
    }
};
```

### Factory Pattern

Use factory classes for creating complex objects:

```cpp
template <typename T>
struct frontend_factory_for_consumer : clang::tooling::FrontendActionFactory {
    std::unique_ptr<T> consumer;

    template <typename... Args>
    explicit frontend_factory_for_consumer(Args &&...args)
        : consumer{ std::make_unique<T>(std::forward<Args>(args)...) } {}

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<frontend>(std::move(consumer));
    }
};
```

---

## Memory Management

### Smart Pointers

Use `std::unique_ptr` for exclusive ownership. Avoid raw `new`/`delete`:

```cpp
// Good
auto consumer = std::make_unique<xdp2_proto_node_consumer>(data);
std::unique_ptr<clang::FrontendAction> action = factory->create();

// Bad
auto *consumer = new xdp2_proto_node_consumer(data);
delete consumer;
```

### Custom Deleters

Use custom deleters for C API resources:

```cpp
using python_object_deleter_t = std::function<decltype(decref)>;
using python_object_t = std::unique_ptr<PyObject, python_object_deleter_t>;

auto make_python_object(PyObject *obj) {
    return python_object_t{ obj, decref };
}
```

### Reference Wrappers

Use `std::reference_wrapper` for storing references in containers:

```cpp
using tlv_node_ref = std::reference_wrapper<tlv_node>;
using unordered_tlv_node_ref_set = std::unordered_set<tlv_node_ref, tlv_node_hash>;
```

### Move Semantics

Implement move constructors and use `std::move` appropriately:

```cpp
pattern_match_factory(pattern_match_factory &&other)
    : patterns{ std::move(other.patterns) } {}

std::unique_ptr<clang::ASTConsumer>
CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef file) override {
    return std::move(consumer);
}
```

### External API Pointers

Raw pointers from external APIs (Clang/LLVM) are not owned by our code:

```cpp
// These pointers are managed by Clang/LLVM - do NOT delete
clang::RecordDecl *record;
::llvm::Value const *value;
```

---

## Error Handling

### Exceptions

Use `std::runtime_error` for error conditions:

```cpp
template <typename T>
auto ensure_not_null(T *t, std::string const &msg) {
    if (t == nullptr) {
        throw std::runtime_error(msg);
    }
    return t;
}
```

### Try-Catch Blocks

Catch exceptions at appropriate boundaries:

```cpp
try {
    auto res = xdp2gen::python::generate_root_parser_c(
        filename, output, graph, roots, record);
    if (res != 0) {
        plog::log(std::cout) << "failed python gen?" << std::endl;
        return res;
    }
} catch (const std::exception &e) {
    plog::log(std::cerr) << "Failed to generate " << output
                         << ": " << e.what() << std::endl;
    return 1;
}
```

### Return Codes

Use integer return codes for function success/failure (0 = success):

```cpp
int extract_struct_constants(
    std::string cfile,
    std::string llvm_file,
    std::vector<const char *> args,
    xdp2gen::graph_t &graph) {

    // ... implementation ...

    return 0;  // Success
}
```

### Logging

Use the project's logging utilities:

```cpp
plog::log(std::cout) << "Processing file: " << filename << std::endl;
plog::warning(std::cerr) << "<Warning> - Invalid input detected" << std::endl;
```

---

## Templates and Metaprogramming

### Type Traits

Define type traits following standard library conventions:

```cpp
template <typename... Ts>
struct args_size {
    static constexpr size_t value = sizeof...(Ts);
};

template <size_t I, typename... Ts>
struct select_type {
    using type = typename std::tuple_element<I, std::tuple<Ts...>>::type;
};

template <size_t I, typename... Ts>
using select_type_t = typename select_type<I, Ts...>::type;
```

### Variadic Templates

```cpp
template <typename Test, typename... Args>
struct one_of : std::disjunction<std::is_same<Test, Args>...> {};

template <typename Test, typename... Args>
constexpr bool one_of_v = one_of<Test, Args...>::value;
```

### C++20 Concepts

Use concepts for cleaner template constraints:

```cpp
template <typename N>
std::pair<node_type, size_t> __search_and_insert(N const *n)
    requires std::is_base_of_v<::llvm::Value, N> ||
             std::is_same_v<::llvm::BasicBlock, N> {
    // Implementation
}
```

### Template Pattern Matching

```cpp
template <typename match_type>
class pattern_match_factory {
    std::vector<match_type> patterns;

public:
    template <typename... Ts, typename G>
    std::vector<std::variant<Ts...>>
    match_all(G const &g, std::initializer_list<size_t> idxs) const {
        return match_all_aux<Ts...>(
            g, idxs, std::make_index_sequence<args_size_v<Ts...>>{});
    }
};
```

---

## Const Correctness

### Function Parameters

Use `const &` for input parameters that won't be modified:

```cpp
void validate_json_metadata(const nlohmann::ordered_json &data);

void process_data(xdp2_proto_node_extract_data const &data);
```

### Const Methods

Mark methods that don't modify state as `const`:

```cpp
class pattern_match_factory {
public:
    template <typename... Ts, typename G>
    std::vector<std::variant<Ts...>>
    match_all(G const &g, std::initializer_list<size_t> idxs) const;

    size_t size() const { return patterns.size(); }
};
```

### Const Local Variables

Use `const` for values that won't change:

```cpp
if (auto const *fd = clang::dyn_cast<clang::FunctionDecl>(decl);
    fd && fd->getNameAsString() == function_name) {
    // ...
}

for (auto const &item : container) {
    process(item);
}
```

### Pointer Const Placement

Place `const` after what it modifies:

```cpp
int const *ptr_to_const_int;        // Pointer to const int
int *const const_ptr_to_int;        // Const pointer to int
int const *const const_ptr_const;   // Const pointer to const int
```

---

## Modern C++ Features

### Prefer Modern Constructs

Use C++17/C++20 features when available:

```cpp
// Structured bindings
auto [key, value] = *map.begin();

// If with initializer
if (auto it = map.find(key); it != map.end()) {
    use(it->second);
}

// std::optional
std::optional<std::string> find_name(int id);

// Range-based for with references
for (auto const &item : container) {
    process(item);
}

// std::filesystem
namespace fs = std::filesystem;
if (fs::exists(path)) {
    // ...
}
```

### Initialization

Use brace initialization:

```cpp
std::vector<int> values{ 1, 2, 3, 4, 5 };
std::string name{ "example" };
```

### Auto

Use `auto` for complex types, but be explicit for simple ones:

```cpp
// Good uses of auto
auto it = container.begin();
auto result = complex_function_returning_template_type();
auto ptr = std::make_unique<complex_type>();

// Prefer explicit types for clarity
int count = 0;
std::string name = "test";
```

---

## Macros

### Minimize Macro Usage

Prefer C++ features over macros:

```cpp
// Prefer constexpr over #define
constexpr size_t MAX_SIZE = 1024;

// Prefer templates over macro functions
template <typename T>
constexpr T max(T a, T b) { return (a > b) ? a : b; }
```

### Acceptable Macro Uses

String stringification:

```cpp
#define XDP2_STRINGIFY_A(X) #X
#define XDP2_STRINGIFY(X) XDP2_STRINGIFY_A(X)
```

Conditional compilation:

```cpp
#ifdef XDP2_CLANG_RESOURCE_PATH
    Tool.appendArgumentsAdjuster(
        clang::tooling::getInsertArgumentAdjuster(
            "-resource-dir=" XDP2_STRINGIFY(XDP2_CLANG_RESOURCE_PATH)));
#endif
```

Header guards (see [Include Directives](#include-directives)).

---

## Debugging

### Logging with plog

The project uses a custom `plog` (program log) system for runtime logging. Logging can be enabled/disabled at runtime:

```cpp
#include "xdp2gen/program-options/log_handler.h"

// Basic logging
plog::log(std::cout) << "Processing file: " << filename << std::endl;

// Warning messages
plog::warning(std::cerr) << "<Warning> - Invalid input detected" << std::endl;

// Check if logging is enabled before expensive operations
if (plog::is_display_log()) {
    var_decl->dump();  // Only dump AST if logging enabled
}

// Control logging programmatically
plog::enable_log();
plog::disable_log();
plog::set_display_log(verbose_flag);
```

### Debug Flags (C-Style Protocol Code)

For low-level protocol code, use bit-flag based debug masks:

```cpp
// Define debug flags using XDP2_BIT macro
#define UET_DEBUG_F_PDC       XDP2_BIT(0)    // 0x1
#define UET_DEBUG_F_TRANS     XDP2_BIT(1)    // 0x2
#define UET_DEBUG_F_PACKET    XDP2_BIT(2)    // 0x4
#define UET_DEBUG_F_FEP       XDP2_BIT(3)    // 0x8

// Check debug flag before output
if (fep->debug_mask & UET_DEBUG_F_FEP) {
    // Debug output
}
```

### Debug Macros with Color Output

Use colored terminal output for debug messages:

```cpp
// Color definitions (from utility.h)
#define XDP2_TERM_COLOR_RED      "\033[1;31m"
#define XDP2_TERM_COLOR_GREEN    "\033[1;32m"
#define XDP2_TERM_COLOR_YELLOW   "\033[1;33m"
#define XDP2_TERM_COLOR_BLUE     "\033[1;34m"
#define XDP2_TERM_COLOR_MAGENTA  "\033[1;35m"
#define XDP2_TERM_COLOR_CYAN     "\033[1;36m"

// Debug macro pattern with color support
#define MODULE_DEBUG(CTX, ...) do { \
    if (!(CTX->debug_mask & MODULE_DEBUG_FLAG)) \
        break; \
    XDP2_CLI_PRINT_COLOR(CTX->debug_cli, COLOR, __VA_ARGS__); \
} while (0)
```

### Debug Tags in Comments

When adding temporary debug code, use descriptive tags:

```cpp
// [nix-debug] Added for troubleshooting segfault issue
plog::log(std::cout) << "[DEBUG] ptr value: " << ptr << std::endl;

// [debug] Temporary - remove after fixing issue #123
```

---

## Assertions

### Runtime Assertions

Use standard `assert()` for runtime invariant checks:

```cpp
#include <cassert>

// Check preconditions
assert(ptr != nullptr);
assert(index < container.size());

// Check invariants
assert(source < curr_size && target < curr_size);

// Document unexpected conditions
assert(!"ImplicitCastExpr should not have more than one child");
```

### Static Assertions

Use `static_assert` for compile-time checks:

```cpp
// Type constraints
static_assert(std::is_enum<ENUM_TYPE>::value, "ENUM_TYPE must be an enum!");
static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

// Size/alignment checks
static_assert(sizeof(header) == 16, "Header size mismatch");
```

### Build-Time Assertions (Kernel-Style)

For C code requiring kernel-style compile-time checks:

```cpp
#include "flowdis/build_bug.h"

// Fail build if condition is true
BUILD_BUG_ON(sizeof(struct my_struct) > 64);

// Power-of-two validation
BUILD_BUG_ON_NOT_POWER_OF_2(BUFFER_SIZE);

// With custom message
BUILD_BUG_ON_MSG(condition, "Descriptive error message");
```

### Null Safety with Cppfront

When using Cppfront (`.h2` files), use `cpp2::assert_not_null`:

```cpp
// Safe null dereference
auto range = CPP2_UFCS_0(children, (*cpp2::assert_not_null(expr)));

// Member access with null check
auto decl = CPP2_UFCS_0(getMemberDecl, (*cpp2::assert_not_null(member_expr)));
```

### Validation Functions

Create explicit validation functions for complex checks:

```cpp
void validate_json_metadata_ents_type(const nlohmann::ordered_json &ents) {
    for (auto const &elm : ents) {
        if (elm.contains("type") && elm.contains("length")) {
            auto type = elm["type"].get<std::string>();
            auto length = elm["length"].get<std::size_t>();
            if (type == "hdr_length" && length != 2) {
                plog::warning(std::cerr)
                    << "<Warning> - hdr_length type should have a size of 2 bytes"
                    << std::endl;
            }
        }
    }
}
```

### Null Pointer Checks

Always check pointers from external APIs before dereferencing:

```cpp
// Defensive null-checking pattern
auto *record_type = type.getAs<RecordType>();
if (record_type == nullptr) {
    // Handle null case - skip or log warning
    plog::log(std::cout) << "[WARNING] Skipping null RecordType" << std::endl;
    return;
}
auto *decl = record_type->getDecl();
```

---

## Testing

### Test Directory Structure

```
src/test/
├── parser/           # Parser unit tests
│   ├── test-parser-core.h
│   └── test-parser-out.h
├── bitmaps/          # Bitmap operation tests
│   └── test_bitmap.h
├── tables/           # Table lookup tests
│   ├── test_table.h
│   └── test_tables.h
├── falcon/           # Protocol-specific tests
│   └── test.h
├── uet/
│   └── test.h
└── router/
    └── test.h

nix/tests/            # Integration tests (Nix-based)
├── default.nix
├── simple-parser.nix
└── simple-parser-debug.nix
```

### Test Core Pattern

Use the plugin-style test framework for parser tests:

```cpp
struct test_parser_core {
    const char *name;
    void (*help)(void);
    void *(*init)(const char *args);
    const char *(*process)(void *pv, void *data, size_t len,
                           struct test_parser_out *out, unsigned int flags,
                           long long *ptr);
    void (*done)(void *pv);
};

// Test flags
#define CORE_F_NOCORE    0x1
#define CORE_F_HASH      0x2
#define CORE_F_VERBOSE   0x4
#define CORE_F_DEBUG     0x8

// Declare a test core
#define CORE_DECL(name) \
    struct test_parser_core test_parser_core_##name = { \
        .name = #name, \
        .help = name##_help, \
        .init = name##_init, \
        .process = name##_process, \
        .done = name##_done \
    }
```

### Test Output Structures

Define structured output for test results:

```cpp
struct test_parser_out_control {
    unsigned short int thoff;
    unsigned char addr_type;
};

#define ADDR_TYPE_OTHER 1
#define ADDR_TYPE_IPv4  2
#define ADDR_TYPE_IPv6  3
#define ADDR_TYPE_TIPC  4

struct test_parser_out_basic {
    unsigned short int n_proto;
    unsigned char ip_proto;
};
```

### Verbose Output Control

Use a global verbose flag for test output:

```cpp
extern int verbose;

// In test code
if (verbose >= 10) {
    printf("Debug: processing packet %d\n", packet_num);
}

// Different verbosity levels
if (verbose >= 1)  // Basic progress
if (verbose >= 5)  // Detailed info
if (verbose >= 10) // Debug output
```

### Test Status Codes

Define clear status enums for test results:

```cpp
enum test_status {
    NO_STATUS,
    HIT_FORWARD = 1000,
    HIT_DROP,
    HIT_NOACTION,
    MISS = -1U,
};

struct test_context {
    char *name;
    int status;
};

static inline void test_forward(struct test_context *ctx, int code) {
    if (verbose >= 10)
        printf("%s: Forward code: %u\n", ctx->name, code);
    ctx->status = code;
}
```

### Integration Tests (Nix)

Write integration tests as Nix shell scripts:

```nix
# nix/tests/simple-parser.nix
pkgs.writeShellApplication {
  name = "xdp2-test-simple-parser";

  text = ''
    set -euo pipefail

    echo "=== Test: simple_parser ==="

    # Test 1: Basic functionality
    echo "--- Test 1: Basic run ---"
    OUTPUT=$(./parser_notmpl "$PCAP" 2>&1) || {
      echo "FAIL: parser exited with error"
      exit 1
    }

    if echo "$OUTPUT" | grep -q "IPv6:"; then
      echo "PASS: Produced expected output"
    else
      echo "FAIL: Missing expected output"
      exit 1
    fi

    # Test 2: With optimization flag
    echo "--- Test 2: Optimized mode ---"
    OUTPUT_OPT=$(./parser_notmpl -O "$PCAP" 2>&1) || {
      echo "FAIL: Optimized mode failed"
      exit 1
    }

    echo "All tests passed!"
  '';
}
```

### Test Organization in Nix

Organize tests in a central `default.nix`:

```nix
# nix/tests/default.nix
{ pkgs, xdp2 }:
{
  simple-parser = import ./simple-parser.nix { inherit pkgs xdp2; };
  simple-parser-debug = import ./simple-parser-debug.nix { inherit pkgs xdp2; };

  # Run all tests
  all = pkgs.writeShellApplication {
    name = "xdp2-test-all";
    text = ''
      echo "=== Running all XDP2 tests ==="
      ${import ./simple-parser.nix { inherit pkgs xdp2; }}/bin/xdp2-test-simple-parser
      echo "=== All tests completed ==="
    '';
  };
}
```

### Test Naming Conventions

| Type | Location | Naming Pattern |
|------|----------|----------------|
| Unit test headers | `src/test/<module>/` | `test_<module>.h` or `test-<module>.h` |
| Test implementations | `src/test/<module>/` | `test_<module>.c` |
| Integration tests | `nix/tests/` | `<feature>.nix` |
| Test binaries | Build output | `test_<feature>` or `<feature>_test` |

---

## Summary

| Aspect | Convention |
|--------|------------|
| Namespaces | `lowercase::with::colons` |
| Classes/Structs | `snake_case` |
| Functions | `snake_case` |
| Variables | `snake_case` |
| Type aliases | `snake_case_t` |
| Constants | `constexpr` with `_v` suffix |
| Indentation | 4 spaces |
| Braces | Same line |
| Pointers | `T *ptr` |
| Comments | SPDX headers, `/* */` blocks, `//` inline |
| Errors | Exceptions (`std::runtime_error`) |
| Memory | `std::unique_ptr`, `std::make_unique` |
| Const | Extensive const correctness |
| Logging | `plog::log()`, `plog::warning()` |
| Assertions | `assert()`, `static_assert`, `BUILD_BUG_ON` |
| Tests | Plugin-style cores, Nix integration tests |

---

*This style guide is a living document. Update it as conventions evolve.*
