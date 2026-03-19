# Static Analysis Exemptions

This document describes each exemption in the triage system and the
rationale for excluding it from high-confidence findings.

## Excluded Check IDs (`filters.py`)

These check IDs are removed entirely during filtering — their findings
never appear in triage output.

### `syntaxError`
- **Tool:** cppcheck
- **Count:** 14 (all test code)
- **Reason:** cppcheck's parser cannot handle XDP2's complex macro
  constructs (variadic macros, token pasting, nested expansion). These
  are parser failures in the tool, not syntax errors in the code. The
  code compiles successfully with GCC and Clang.

### `preprocessorErrorDirective`
- **Tool:** cppcheck
- **Count:** 8
- **Reason:** Two categories:
  1. **Intentional `#error` platform guards** — e.g.,
     `#error "Unsupported long size"` in `bitmap.h`,
     `#error "Endianness not identified"` in `proto_geneve.h`. These are
     compile-time assertions that fire only on unsupported platforms.
  2. **cppcheck macro expansion failures** — e.g., `pmacro.h` lines
     where cppcheck fails to expand `XDP2_SELECT_START` /
     `XDP2_VSTRUCT_VSCONST` due to complex `##` token pasting. The
     macros work correctly with real compilers.

### `unknownMacro`
- **Tool:** cppcheck
- **Count:** 2
- **Reason:** cppcheck doesn't recognize project-specific macros:
  - `LIST_FOREACH` (`dtable.c:789`) — standard BSD-style list traversal
    macro, defined in project headers.
  - `__XDP2_PMACRO_APPLYXDP2_PMACRO_NARGS` (`bitmap_word.h:544`) —
    internal macro helper from the pmacro system.
  These would require `--suppress=` or `--library=` configuration for
  cppcheck, which is not worth the maintenance burden.

### `arithOperationsOnVoidPointer`
- **Tool:** cppcheck
- **Count:** 25
- **Reason:** Void pointer arithmetic (`void *p; p += n`) is a GNU C
  extension where `sizeof(void) == 1`. This is used intentionally
  throughout the codebase:
  - `siphash.c` — hash function byte-level pointer walks
  - `obj_allocator.c` — memory pool object addressing
  - `parser.c` — packet header pointer advancement
  All three GCC, Clang, and the Linux kernel rely on this extension.
  The code is correct and compiles without warnings under `-std=gnu11`.

### `subtractPointers`
- **Tool:** cppcheck
- **Count:** 3
- **Reason:** Pointer subtraction in `cli.h:88,107` and `dtable.h:85`
  implements `container_of`-style macros — computing the offset of a
  member within a struct to recover the containing struct pointer. This
  is a standard C idiom used throughout Linux kernel code and system
  libraries. cppcheck flags it because the two pointers technically
  point to "different objects" (member vs. container), but the operation
  is well-defined in practice on all target platforms.

## Generated File Patterns (`filters.py`)

### `*.template.c`
- **Reason:** Template files under `src/templates/xdp2/` are input to
  `xdp2-compiler`, which processes them into final C source. They
  contain placeholder identifiers and incomplete type references that
  are resolved during code generation. Findings like
  `clang-diagnostic-implicit-int` and
  `clang-diagnostic-implicit-function-declaration` in these files are
  expected and not actionable.

## Scoring Adjustments (`scoring.py`)

These checks still appear in the full triage summary but are excluded
from the high-confidence list.

### `bugprone-narrowing-conversions` → `STYLE_ONLY_CHECKS`
- **Tool:** clang-tidy
- **Count:** 56 (was the single largest category in high-confidence)
- **Reason:** The vast majority are `size_t` → `ssize_t` and
  `unsigned int` → `int` conversions in packet parsing code where sizes
  and offsets are bounded by protocol constraints (e.g., packet length
  fits in `int`). These narrowing conversions are intentional and
  ubiquitous in C networking code. Previously listed in
  `BUG_CLASS_CHECKS`, which incorrectly elevated all 56 to
  high-confidence. Moved to `STYLE_ONLY_CHECKS` so they remain visible
  in the full report but don't overwhelm the actionable findings list.

### `variableScope` → `STYLE_ONLY_CHECKS`
- **Tool:** cppcheck
- **Count:** 30
- **Reason:** Suggestions to move variable declarations closer to first
  use. This is a style preference — the existing code follows C89-style
  declarations-at-top-of-block, which is a valid convention. Not a bug.

### `constParameter`, `constParameterCallback` → `STYLE_ONLY_CHECKS`
- **Tool:** cppcheck
- **Count:** 14
- **Reason:** Suggestions to add `const` to parameters that are not
  modified. Valid style improvement but not a correctness issue, and
  changing function signatures affects the public API.

### Excluded from High-Confidence via `_HIGH_CONF_EXCLUDED_PREFIXES`

#### `bugprone-reserved-identifier`
- **Count:** 642 (combined with `cert-dcl37-c,cert-dcl51-cpp`)
- **Reason:** XDP2 uses double-underscore prefixed identifiers
  (`__XDP2_PMACRO_*`, `___XDP2_BITMAP_WORD_*`) as internal macro
  helpers. This is the project's deliberate convention for namespace
  separation. While technically reserved by the C standard, these
  identifiers do not conflict with any compiler or library names.

#### `bugprone-easily-swappable-parameters`
- **Count:** 201
- **Reason:** Functions with multiple parameters of the same type (e.g.,
  `int offset, int length`). This is inherent to packet parsing APIs
  where multiple integer parameters represent distinct protocol fields.
  Cannot be changed without breaking the API.

#### `bugprone-assignment-in-if-condition`
- **Count:** 79
- **Reason:** `if ((x = func()))` is an intentional C idiom used
  throughout the codebase for error-checked assignment. This is standard
  practice in C systems code (Linux kernel, glibc, etc.).

#### `bugprone-macro-parentheses`
- **Count:** 329
- **Reason:** Suggestions to add parentheses around macro arguments.
  Many of XDP2's macros are protocol field accessors where the argument
  is always a simple identifier, making extra parentheses unnecessary.
  Some macros intentionally use unparenthesized arguments for token
  pasting or stringification.

#### `bugprone-implicit-widening-of-multiplication-result`
- **Count:** 139
- **Reason:** Multiplication results widened to `size_t` or `ssize_t`
  in packet offset calculations. The operands are bounded by protocol
  constraints (header sizes, field counts), so overflow is not possible
  in practice. False positives in packet parsing arithmetic.

#### `misc-no-recursion`
- **Count:** 29
- **Reason:** Recursion is used intentionally in protocol graph
  traversal (nested protocol headers, decision tables). The recursion
  depth is bounded by protocol nesting limits. Eliminating recursion
  would require significant refactoring with no safety benefit.
