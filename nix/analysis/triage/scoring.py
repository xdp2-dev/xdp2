"""Priority scoring, cross-tool correlation, and high-confidence filtering.

Adapted for XDP2's C codebase — check IDs and noise patterns
are specific to C static analysis tools.
"""

from collections import defaultdict

from finding import Finding
from filters import is_security_sensitive, is_test_code


# flawfinder check_ids that are intentional patterns in system software
FLAWFINDER_NOISE = {
    'race.chmod', 'race.chown', 'race.access', 'race.vfork',
    'shell.system', 'shell.execl', 'shell.execlp', 'shell.execv', 'shell.execvp',
    'buffer.read', 'buffer.char', 'buffer.equal', 'buffer.memcpy',
    'buffer.strlen', 'buffer.getenv', 'buffer.wchar_t',
    'misc.open', 'random.random', 'tmpfile.mkstemp', 'access.umask',
    'format.snprintf', 'format.vsnprintf', 'misc.chroot',
}

# Bug-class check IDs that represent real correctness issues (not style)
BUG_CLASS_CHECKS = {
    # Correctness bugs
    'uninitMemberVar', 'unsignedLessThanZero',
    'core.UndefinedBinaryOperatorResult', 'core.NullDereference',
    'core.DivideZero', 'core.uninitialized',
    'core.uninitialized.Assign',
    # Bugprone clang-tidy checks
    'bugprone-use-after-move',
    'bugprone-sizeof-expression',
    'bugprone-integer-division',
    # Memory safety
    'unix.Malloc', 'unix.MismatchedDeallocator',
    'alpha.security.ArrayBoundV2',
}

# Style checks that shouldn't appear in high-confidence even in security code
STYLE_ONLY_CHECKS = {
    'constParameterPointer', 'constVariablePointer',
    'constParameter', 'constParameterCallback',
    'shadowVariable', 'shadowArgument', 'shadowFunction',
    'knownConditionTrueFalse', 'unusedStructMember',
    'variableScope',                    # Moving declarations is style, not bugs
    'bugprone-narrowing-conversions',   # size_t→ssize_t, uint→int: intentional in C networking code
}

# Prefixes excluded from high-confidence output
_HIGH_CONF_EXCLUDED_PREFIXES = (
    'readability-',
    'misc-include-cleaner', 'misc-use-internal-linkage',
    'misc-use-anonymous-namespace', 'misc-unused-parameters',
    'misc-const-correctness', 'misc-header-include-cycle',
    'misc-no-recursion',
    'cert-',
    # High-volume bugprone checks that are style/convention, not bugs
    'bugprone-reserved-identifier',         # __XDP2_PMACRO_* is project convention
    'bugprone-easily-swappable-parameters', # Style — can't change existing API
    'bugprone-assignment-in-if-condition',  # Intentional C pattern: if ((x = func()))
    'bugprone-macro-parentheses',           # Style — many macros are correct without extra parens
    'bugprone-implicit-widening-of-multiplication-result',  # False positives in packet offset math
)


def priority_score(f, category_counts: dict) -> int:
    """Higher score = higher priority. Range roughly 0-100."""
    score = 0

    # Severity
    if f.severity == 'error':
        score += 40
    elif f.severity == 'warning':
        score += 20

    # Security-sensitive location
    if is_security_sensitive(f.file):
        score += 20

    # Test code is lower priority
    if is_test_code(f.file):
        score -= 15

    # Small-count categories are anomalies (more likely real bugs)
    count = category_counts.get(f.check_id, 999)
    if count <= 3:
        score += 30
    elif count <= 10:
        score += 20
    elif count <= 30:
        score += 10

    return score


def find_cross_tool_hits(findings: list, tolerance: int = 3) -> list:
    """Find file:line pairs flagged by 2+ independent tools.

    Uses a tolerance of +/-N lines to account for minor line number differences.
    """
    # Group findings by file
    by_file = defaultdict(list)
    for f in findings:
        by_file[f.file].append(f)

    clusters = []
    for filepath, file_findings in by_file.items():
        # Sort by line
        file_findings.sort(key=lambda f: f.line)

        # For each pair of findings from different tools, check proximity
        for i, f1 in enumerate(file_findings):
            cluster = [f1]
            for f2 in file_findings[i + 1:]:
                if f2.line > f1.line + tolerance:
                    break
                if f2.tool != f1.tool:
                    cluster.append(f2)
            if len(set(f.tool for f in cluster)) >= 2:
                clusters.append(cluster)

    # Deduplicate overlapping clusters
    seen_keys = set()
    unique_clusters = []
    for cluster in clusters:
        key = frozenset((f.tool, f.file, f.line) for f in cluster)
        if key not in seen_keys:
            seen_keys.add(key)
            unique_clusters.append(cluster)

    return unique_clusters


def get_high_confidence(findings: list) -> list:
    """Return high-confidence findings as (Finding, score, is_cross) tuples, sorted by score."""
    # Compute category counts
    cat_counts = defaultdict(int)
    for f in findings:
        cat_counts[f.check_id] += 1

    # Find cross-tool correlations (excluding flawfinder noise)
    non_noise = [f for f in findings if f.check_id not in FLAWFINDER_NOISE]
    cross_hits = find_cross_tool_hits(non_noise)
    cross_locations = set()
    for cluster in cross_hits:
        for f in cluster:
            cross_locations.add((f.file, f.line))

    high_conf = []
    for f in findings:
        # Skip noise categories entirely from high-confidence
        if f.check_id in FLAWFINDER_NOISE or f.check_id in STYLE_ONLY_CHECKS:
            continue
        # Skip excluded prefixes (style, not bugs)
        if any(f.check_id.startswith(p) for p in _HIGH_CONF_EXCLUDED_PREFIXES):
            continue

        score = priority_score(f, cat_counts)
        is_cross = (f.file, f.line) in cross_locations
        is_bug_class = f.check_id in BUG_CLASS_CHECKS
        is_small_cat = cat_counts[f.check_id] <= 3
        # Only cppcheck/clang-analyzer error-severity in security code
        is_security_bug = (is_security_sensitive(f.file) and f.severity == 'error'
                           and f.tool in ('cppcheck', 'clang-analyzer', 'gcc-analyzer'))

        if is_cross or is_bug_class or (is_small_cat and score >= 60) or is_security_bug:
            high_conf.append((f, score, is_cross))

    # Sort by score descending
    high_conf.sort(key=lambda x: -x[1])
    return high_conf
