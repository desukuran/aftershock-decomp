#!/usr/bin/env python3
"""Make decompiler output shorter to read: collapse 16-dword copy loops into
COPY(dst, src), drop identity-matrix fill lines and zero stores to locals.

  collapse.py < decomp.c
"""
import re
import sys

text = sys.stdin.read()
# p = src; q = dst; for (i = 0x10; i != 0; i--) { *q = *p; p++; q++; }
ref = r'((?:\(\w+ \*\))?[&\w.\[\]>-]+)'
loop = re.compile(
    r'\n(\s*)(\w+) = ' + ref + r';\n\s*(\w+) = ' + ref + r';\n\s*for \((\w+) = 0x10; \6 != 0; \6 = \6 \+ -1\) \{\n'
    r'\s*\*\4 = \*\2;\n\s*\2 = \2 \+ 1;\n\s*\4 = \4 \+ 1;\n\s*\}')
text = loop.sub(lambda m: f'\n{m.group(1)}COPY({m.group(5)}, {m.group(3)});', text)
out = []
for line in text.split('\n'):
    s = line.strip()
    if re.fullmatch(r'local_\w+(\.m\[\d\]\[\d\]|\[\d+\]) = (0|0\.0|1\.0f);', s):
        continue
    out.append(line)
sys.stdout.write('\n'.join(out))
