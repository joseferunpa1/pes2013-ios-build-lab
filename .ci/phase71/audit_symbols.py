#!/usr/bin/env python3
"""Audit a partial Mach-O link without mistaking it for a full runtime link."""
from pathlib import Path
import re
import sys

root = Path(sys.argv[1])
checks = []
for arch in ('arm64', 'x86_64'):
    def symbols(name):
        path = root / f'{name}-{arch}.txt'
        return path.read_text(errors='replace') if path.exists() else ''

    loader = symbols('loader-symbols')
    guest = symbols('guest-symbols')
    combined = symbols('combined-symbols')
    undefined = symbols('combined-undefined')

    def defs(text):
        return set(re.findall(r'^\s*[0-9a-fA-F]+\s+[A-TV-Z]\s+(_[A-Za-z0-9_$\.]+)\s*$', text, re.M))

    def undefs(text):
        return set(re.findall(r'^\s*U\s+(_[A-Za-z0-9_$\.]+)\s*$', text, re.M))

    loader_defs = defs(loader)
    guest_defs = defs(guest)
    combined_defs = defs(combined)
    unresolved = undefs(undefined)

    required_loader = {'_ps3_indirect_call', '_ppu_recomp_register', '_ppu_register_function'}
    required_guest = {'_function_table', '_function_table_count'}
    check = {
        'arch': arch,
        'loader_exports_present': required_loader <= loader_defs,
        'generated_table_present': required_guest <= guest_defs,
        'combined_exports_present': (required_loader | required_guest) <= combined_defs,
        'mutual_symbols_resolved': not ((required_loader | required_guest) & unresolved),
        'remaining_undefined': len(unresolved),
        'remaining_examples': sorted(unresolved)[:30],
    }
    checks.append(check)
    lines = [
        f'ARCH={arch}',
        f'LOADER_PRODUCTION_ENTRYPOINTS={"YES" if check["loader_exports_present"] else "NO"}',
        f'GUEST_FUNCTION_TABLE={"YES" if check["generated_table_present"] else "NO"}',
        f'COMBINED_ENTRYPOINTS={"YES" if check["combined_exports_present"] else "NO"}',
        f'MUTUAL_SYMBOLS_RESOLVED={"YES" if check["mutual_symbols_resolved"] else "NO"}',
        f'REMAINING_RUNTIME_UNDEFINED_COUNT={len(unresolved)}',
        'REMAINING_RUNTIME_EXAMPLES=' + ','.join(check['remaining_examples']),
    ]
    (root / f'audit-{arch}.txt').write_text('\n'.join(lines) + '\n')

passing = all(
    item['loader_exports_present'] and
    item['generated_table_present'] and
    item['combined_exports_present'] and
    item['mutual_symbols_resolved']
    for item in checks
)
summary = 'PHASE71_SYMBOL_INTEGRATION=' + ('PASS' if passing else 'FAIL') + '\n'
summary += '\n'.join((root / f'audit-{arch}.txt').read_text().strip() for arch in ('arm64','x86_64')) + '\n'
(root / 'audit-summary.txt').write_text(summary)
print(summary)
sys.exit(0 if passing else 1)
