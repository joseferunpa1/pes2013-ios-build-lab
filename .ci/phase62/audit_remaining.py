#!/usr/bin/env python3
"""Heuristic source audit. Does not claim a symbol is defined just because it is mentioned."""
import argparse
import re
from pathlib import Path

ap=argparse.ArgumentParser()
ap.add_argument('--source-root',required=True)
ap.add_argument('--missing',required=True)
ap.add_argument('--out',required=True)
a=ap.parse_args()
root=Path(a.source_root)
out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
missing=sorted(set(s.strip() for s in Path(a.missing).read_text().splitlines() if s.strip()))
files=[p for p in root.rglob('*') if p.suffix.lower() in {'.c','.cpp','.cc','.cxx','.m','.mm','.h','.hpp','.s','.S'.lower()} and p.is_file()]
# Do not claim these regexes prove definitions. Scan source and preserve line context for review.
rows=['symbol\tfile\tline\tclassification\tsnippet']
summary=['symbol\tcandidate_definition_lines\tother_mentions\tcandidate_files']
for name in missing:
    rx=re.compile(r'(?<![A-Za-z0-9_])'+re.escape(name)+r'(?![A-Za-z0-9_])')
    candidates=[]; others=[]
    for path in files:
        try:
            lines=path.read_text(encoding='utf-8',errors='replace').splitlines()
        except OSError:
            continue
        for i,line in enumerate(lines):
            if not rx.search(line): continue
            # Find likely definitions using an adjacent brace, initializer, or symbol label.
            cleaned=line.strip(); nxt=' '.join(lines[i+1:i+4]).strip() if i+1<len(lines) else ''
            looks_func=bool(re.search(r'\b'+re.escape(name)+r'\s*\(',line) and ('{' in line or (not line.rstrip().endswith(';') and re.match(r'\s*\{',nxt))))
            looks_var=bool(re.search(r'\b'+re.escape(name)+r'\s*(?:\[[^\]]*\])?\s*=',line))
            looks_label=bool(re.match(r'\s*_?'+re.escape(name)+r'\s*:',line))
            kind='POSSIBLE_DEFINITION' if looks_func or looks_var or looks_label else 'MENTION_ONLY'
            snippet=re.sub(r'\s+',' ',cleaned)[:200].replace('\t',' ')
            entry=(name,path.as_posix(),i+1,kind,snippet)
            (candidates if kind=='POSSIBLE_DEFINITION' else others).append(entry)
    # Write all candidate definitions, and cap mentions to prevent huge reports.
    for e in candidates+others[:25]: rows.append('\t'.join(map(str,e)))
    summary.append(f'{name}\t{len(candidates)}\t{len(others)}\t{len({v[1] for v in candidates})}')
(out/'matches.tsv').write_text('\n'.join(rows)+'\n')
(out/'summary.tsv').write_text('\n'.join(summary)+'\n')
(out/'report.txt').write_text(f'MISSING_SYMBOLS={len(missing)}\nSCANNED_FILES={len(files)}\nAUDIT_COMPLETE=YES\nNOTE=Text heuristics only. Check nm, target compatibility and actual runtime semantics before integrating.\n')
print((out/'report.txt').read_text())
