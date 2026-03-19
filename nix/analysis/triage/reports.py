"""Report formatting and output functions."""

import io
import os
from collections import defaultdict
from contextlib import redirect_stdout

from finding import Finding
from filters import is_test_code, is_security_sensitive
from scoring import priority_score, find_cross_tool_hits, get_high_confidence


def format_finding(f, score=None) -> str:
    score_str = f'[score={score}] ' if score is not None else ''
    return f'{score_str}{f.tool}: {f.file}:{f.line}: [{f.severity}] {f.check_id} -- {f.message}'


def print_summary(findings: list):
    """Print category summary after filtering, sorted by priority."""
    # Count by (tool, check_id)
    category_counts = defaultdict(lambda: {
        'count': 0, 'tools': set(), 'severities': set(),
        'prod': 0, 'test': 0, 'security': 0
    })

    for f in findings:
        cat = category_counts[f.check_id]
        cat['count'] += 1
        cat['tools'].add(f.tool)
        cat['severities'].add(f.severity)
        if is_test_code(f.file):
            cat['test'] += 1
        else:
            cat['prod'] += 1
        if is_security_sensitive(f.file):
            cat['security'] += 1

    # Sort: error first, then by count ascending (anomalies first)
    def sort_key(item):
        name, cat = item
        has_error = 'error' in cat['severities']
        return (not has_error, cat['count'], name)

    print(f'\n{"Category":<50} {"Count":>6} {"Prod":>5} {"Test":>5} {"Sec":>4} {"Sev":<8} {"Tools"}')
    print('-' * 110)

    for name, cat in sorted(category_counts.items(), key=sort_key):
        sev = '/'.join(sorted(cat['severities']))
        tools = ','.join(sorted(cat['tools']))
        print(f'{name:<50} {cat["count"]:>6} {cat["prod"]:>5} {cat["test"]:>5} '
              f'{cat["security"]:>4} {sev:<8} {tools}')

    total = sum(c['count'] for c in category_counts.values())
    prod = sum(c['prod'] for c in category_counts.values())
    test = sum(c['test'] for c in category_counts.values())
    print(f'\nTotal: {total} findings ({prod} production, {test} test) '
          f'across {len(category_counts)} categories')


def print_high_confidence(findings: list):
    """Print only likely-real-bug findings."""
    high_conf = get_high_confidence(findings)

    if not high_conf:
        print('No high-confidence findings.')
        return

    print(f'\n=== High-Confidence Findings ({len(high_conf)}) ===\n')

    for f, score, is_cross in high_conf:
        cross_marker = ' [CROSS-TOOL]' if is_cross else ''
        loc = 'security' if is_security_sensitive(f.file) else ('test' if is_test_code(f.file) else 'prod')
        print(f'  [{score:>3}] [{loc:<8}]{cross_marker}')
        print(f'        {f.tool}: {f.file}:{f.line}')
        print(f'        {f.check_id} [{f.severity}]')
        print(f'        {f.message}')
        print()


def print_cross_ref(findings: list):
    """Print multi-tool correlations."""
    clusters = find_cross_tool_hits(findings)

    if not clusters:
        print('No cross-tool correlations found.')
        return

    print(f'\n=== Cross-Tool Correlations ({len(clusters)} clusters) ===\n')

    for i, cluster in enumerate(clusters, 1):
        tools = sorted(set(f.tool for f in cluster))
        print(f'  Cluster #{i} -- {cluster[0].file}:{cluster[0].line} ({", ".join(tools)})')
        for f in cluster:
            print(f'    {f.tool}: [{f.severity}] {f.check_id} -- {f.message}')
        print()


def print_category(findings: list, category: str):
    """Print all findings for a specific category."""
    matches = [f for f in findings if f.check_id == category]

    if not matches:
        # Try partial match
        matches = [f for f in findings if category.lower() in f.check_id.lower()]

    if not matches:
        print(f'No findings matching category "{category}".')
        return

    # Group by check_id if partial match found multiple
    by_check = defaultdict(list)
    for f in matches:
        by_check[f.check_id].append(f)

    for check_id, check_findings in sorted(by_check.items()):
        print(f'\n=== {check_id} ({len(check_findings)} findings) ===\n')
        for f in sorted(check_findings, key=lambda x: (x.file, x.line)):
            loc = 'test' if is_test_code(f.file) else 'prod'
            print(f'  [{loc}] {f.file}:{f.line}')
            print(f'        {f.message}')
            print()


def print_full_report(findings: list):
    """Print all findings sorted by priority."""
    cat_counts = defaultdict(int)
    for f in findings:
        cat_counts[f.check_id] += 1

    scored = [(f, priority_score(f, cat_counts)) for f in findings]
    scored.sort(key=lambda x: (-x[1], x[0].file, x[0].line))

    print(f'\n=== All Findings ({len(scored)}) ===\n')

    for f, score in scored[:200]:  # Limit output
        print(format_finding(f, score))

    if len(scored) > 200:
        print(f'\n... and {len(scored) - 200} more (use --summary or --category to drill in)')


def capture_output(func, *args, **kwargs) -> str:
    """Capture stdout from a function call and return as string."""
    buf = io.StringIO()
    with redirect_stdout(buf):
        func(*args, **kwargs)
    return buf.getvalue()


def write_all_reports(findings: list, output_dir: str):
    """Write all report modes as files to output_dir."""
    os.makedirs(output_dir, exist_ok=True)

    with open(os.path.join(output_dir, 'summary.txt'), 'w') as f:
        f.write(capture_output(print_summary, findings))

    prod_findings = [f for f in findings if not is_test_code(f.file)]

    with open(os.path.join(output_dir, 'high-confidence.txt'), 'w') as f:
        f.write(capture_output(print_high_confidence, prod_findings))

    high_conf = get_high_confidence(prod_findings)
    with open(os.path.join(output_dir, 'count.txt'), 'w') as f:
        f.write(str(len(high_conf)))

    with open(os.path.join(output_dir, 'cross-ref.txt'), 'w') as f:
        f.write(capture_output(print_cross_ref, findings))

    with open(os.path.join(output_dir, 'full-report.txt'), 'w') as f:
        f.write(capture_output(print_full_report, findings))
