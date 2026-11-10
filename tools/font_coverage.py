#!/usr/bin/env python3
"""Every character the Chinese interface tables draw must exist in one of the two faces the UI uses.

The launcher and menu merge a CJK face into Baloo 2 (runtime/ui/font_cjk.h). The face is shipped in the APK as a
subset - android/lasrecomp/app/src/main/assets/res/fonts/ui-cjk.otf - so a character added to the tables later is
a box on the phone unless the subset is rebuilt. This fails the build instead.

  python3 tools/font_coverage.py            # check
  python3 tools/font_coverage.py --list     # also print the codepoints the subset must contain

Rebuild the subset with (fontTools, Noto Sans CJK SC = face 2):

  pyftsubset NotoSansCJK-Regular.ttc --font-number=2 --unicodes-file=/tmp/uni.txt \\
      --output-file=android/lasrecomp/app/src/main/assets/res/fonts/ui-cjk.otf \\
      --layout-features='' --no-hinting --desubroutinize --drop-tables+=DSIG --name-IDs='*' --name-languages='*'

where /tmp/uni.txt is the comma-separated U+XXXX list printed by --list.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUNDLED = os.path.join(ROOT, "android/lasrecomp/app/src/main/assets/res/fonts/ui-cjk.otf")
BALOO = os.path.join(ROOT, "third_party/fonts/Baloo2/Baloo2-SemiBold.ttf")
TABLES = [
    os.path.join(ROOT, "runtime/ui/i18n_zh_hans.inc"),
    os.path.join(ROOT, "runtime/ui/i18n_zh_hant.inc"),
]


def cmap_of(path):
    try:
        from fontTools.ttLib import TTFont
    except ImportError:
        sys.exit("fonttools is missing: pip install fonttools")
    font = TTFont(path, lazy=True)
    cover = set()
    for table in font["cmap"].tables:
        cover |= set(table.cmap.keys())
    return cover


def main():
    for p in [BUNDLED, BALOO] + TABLES:
        if not os.path.exists(p):
            sys.exit("missing: %s" % p)

    bundled = cmap_of(BUNDLED)
    baloo = cmap_of(BALOO)

    # The symbols the launcher's font ranges ask for (launcher.mm). U+25B8 is listed there but is in neither face
    # upstream either, so it is not a regression and is left out here.
    fixed = [0x2039, 0x203A, 0x2192, 0x2713]

    missing, seen, rows = [], set(), 0
    for path in TABLES:
        name = os.path.basename(path)
        for lineno, line in enumerate(open(path, encoding="utf-8"), 1):
            s = line.strip()
            if not s.startswith('{"'):
                continue
            rows += 1
            fields = re.findall(r'"((?:[^"\\]|\\.)*)"', s)
            if len(fields) < 2:
                continue
            for ch in fields[1]:
                cp = ord(ch)
                if cp in seen:
                    continue
                seen.add(cp)
                if cp in bundled or cp in baloo:
                    continue
                missing.append((cp, name, lineno))

    for cp in fixed:
        if cp not in bundled and cp not in baloo:
            missing.append((cp, "runtime/ui/font_cjk.h", 0))

    if "--list" in sys.argv:
        need = sorted({c for c in seen if c > 0x1F} | set(range(0x20, 0x7F)) |
                      {0x00A0, 0x00D7, 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026})
        print(",".join("U+%04X" % c for c in need))

    if missing:
        print("FAIL: %d character(s) the tables use are in neither face" % len(missing))
        for cp, where, lineno in sorted(missing):
            print("  U+%04X %r  (%s%s)" % (cp, chr(cp), where, ":%d" % lineno if lineno else ""))
        print("Rebuild the bundled subset (see the header of this file) or change the wording.")
        return 1

    print("OK: %d table rows, %d distinct characters, all covered by the bundled subset or Baloo 2" %
          (rows, len(seen)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
