#!/usr/bin/env python3
"""Replace long string literals in decompiler output with TEXT_<address>.

  textrefs.py < decomp.c > decomp_refs.c

The story, dialogue and tutorial text stays out of the source (see
tools/extract_data.py); this shows where each piece is used by address.
Strings of up to MAX_INLINE characters (menu labels, formats) are kept.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAX_INLINE = 40


def main():
    sys.path.insert(0, os.path.join(ROOT, '../../tools'))
    from extract_data import wide_strings
    from xbe import Xbe
    x = Xbe(os.path.join(ROOT, 'original/default.xbe'))
    by_text = {}
    for va, t in wide_strings(x):
        esc = t.replace('\\', '\\\\').replace('"', '\\"')
        by_text.setdefault(esc, '%x' % va)
    text = sys.stdin.read()

    def rep(m):
        s = m.group(1)
        if len(s) <= MAX_INLINE:
            return m.group(0)
        a = by_text.get(s)
        return f'TEXT_{int(a, 16):08X}' if a else 'TEXT_UNKNOWN'
    sys.stdout.write(re.sub(r'L"((?:[^"\\]|\\.)*)"', rep, text))


if __name__ == '__main__':
    main()
