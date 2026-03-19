"""Noise constants, path classifiers, filtering, and deduplication.

Adapted for XDP2's C codebase — path patterns and exclusions
are specific to this project's directory structure.
"""

from finding import Finding


# Third-party code — findings are not actionable
# Note: /nix/store/ prefix is stripped by normalize_path before filtering
THIRD_PARTY_PATTERNS = [
    'thirdparty/', 'cppfront/',
]

# Generated files — findings are not actionable
GENERATED_FILE_PATTERNS = [
    'parser_*.p.c',       # xdp2-compiler generated parser code
    '*.template.c',       # Template files before xdp2-compiler processing
    '_pmacro_gen.h',      # Packet macro generator output
    '_dtable.h',          # Decision table output
    '_stable.h',          # State table output
]

EXCLUDED_CHECK_IDS = {
    # Known false positive categories
    'normalCheckLevelMaxBranches',
    # Cppcheck noise — tool limitations, not code bugs
    'missingIncludeSystem',
    'missingInclude',
    'unmatchedSuppression',
    'checkersReport',
    'syntaxError',                  # Can't parse complex macro constructs
    'preprocessorErrorDirective',   # Intentional #error guards / macro expansion failures
    'unknownMacro',                 # Doesn't understand project macros (LIST_FOREACH, pmacro)
    # Cppcheck false positives in idiomatic C
    'arithOperationsOnVoidPointer', # GNU C extension (sizeof(void)==1), intentional in networking code
    'subtractPointers',             # container_of style pointer arithmetic
    # Clang-tidy build errors (not real findings)
    'clang-diagnostic-error',
    # _FORTIFY_SOURCE warnings (build config, not code bugs)
    '-W#warnings',
    '-Wcpp',
}

EXCLUDED_MESSAGE_PATTERNS = [
    '_FORTIFY_SOURCE',
]

TEST_PATH_PATTERNS = ['src/test/', '/test/']

SECURITY_PATHS = ['src/lib/', 'src/include/xdp2/']


def _match_generated(path: str) -> bool:
    """Check if file matches a generated file pattern (supports * glob)."""
    import fnmatch
    name = path.rsplit('/', 1)[-1] if '/' in path else path
    return any(fnmatch.fnmatch(name, pat) for pat in GENERATED_FILE_PATTERNS)


def is_generated(path: str) -> bool:
    return _match_generated(path)


def is_third_party(path: str) -> bool:
    for pat in THIRD_PARTY_PATTERNS:
        if pat in path:
            return True
    # Files not under src/ are likely third-party or generated
    return not path.startswith('src/')


def is_test_code(path: str) -> bool:
    return any(pat in path for pat in TEST_PATH_PATTERNS)


def is_security_sensitive(path: str) -> bool:
    return any(pat in path for pat in SECURITY_PATHS)


def filter_findings(findings: list) -> list:
    """Remove third-party code and known false positive categories."""
    return [
        f for f in findings
        if not is_third_party(f.file)
        and not is_generated(f.file)
        and f.check_id not in EXCLUDED_CHECK_IDS
        and f.line > 0
        and not any(pat in f.message for pat in EXCLUDED_MESSAGE_PATTERNS)
    ]


def deduplicate(findings: list) -> list:
    """Deduplicate findings by (file, line, check_id).

    clang-tidy reports the same header finding once per translation unit.
    Keep first occurrence only.
    """
    seen = set()
    result = []
    for f in findings:
        key = f.dedup_key()
        if key not in seen:
            seen.add(key)
            result.append(f)
    return result
