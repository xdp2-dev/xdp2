"""CLI entry point for static analysis triage.

Usage:
    python -m triage <result-dir>                    # Full prioritized report
    python triage <result-dir> --summary             # Category summary
    python triage <result-dir> --high-confidence     # Likely real bugs only
    python triage <result-dir> --cross-ref           # Multi-tool correlations
    python triage <result-dir> --category <name>     # Drill into category
"""

import argparse
import os
import sys

from parsers import load_all_findings
from filters import filter_findings, deduplicate, is_test_code
from reports import (
    print_summary, print_high_confidence, print_cross_ref,
    print_category, print_full_report, write_all_reports,
)


def main():
    parser = argparse.ArgumentParser(
        description='Triage static analysis findings across multiple tools.'
    )
    parser.add_argument('result_dir', help='Path to analysis results directory')
    parser.add_argument('--summary', action='store_true',
                        help='Show category summary')
    parser.add_argument('--high-confidence', action='store_true',
                        help='Show only likely-real-bug findings')
    parser.add_argument('--cross-ref', action='store_true',
                        help='Show multi-tool correlations')
    parser.add_argument('--category', type=str,
                        help='Drill into a specific check category')
    parser.add_argument('--output-dir', type=str,
                        help='Write all reports as files to this directory')
    parser.add_argument('--include-test', action='store_true',
                        help='Include test code findings (excluded by default in high-confidence)')

    args = parser.parse_args()

    if not os.path.isdir(args.result_dir):
        print(f'Error: {args.result_dir} is not a directory', file=sys.stderr)
        sys.exit(1)

    # Load, filter, deduplicate
    raw = load_all_findings(args.result_dir)
    filtered = filter_findings(raw)
    findings = deduplicate(filtered)

    print(f'Loaded {len(raw)} raw -> {len(filtered)} filtered -> {len(findings)} dedup')

    if args.output_dir:
        write_all_reports(findings, args.output_dir)
    elif args.summary:
        print_summary(findings)
    elif args.high_confidence:
        if not args.include_test:
            findings = [f for f in findings if not is_test_code(f.file)]
        print_high_confidence(findings)
    elif args.cross_ref:
        print_cross_ref(findings)
    elif args.category:
        print_category(findings, args.category)
    else:
        print_full_report(findings)


if __name__ == '__main__':
    main()
