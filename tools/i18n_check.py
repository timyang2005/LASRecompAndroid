#!/usr/bin/env python3
"""i18n 键覆盖率与一致性校验（LASRecompAndroid 启动器中文化）。

在仓库根目录运行：python3 tools/i18n_check.py

检查内容：
  1. 简/繁中文表的键是否逐条对齐、有无重复键、译文里的格式符是否与英文一致；
  2. 葡语表 / 多语表的键（新仓库 UI 的英文基准）与源码里 tr() 及各 opt_*/header/hint/choice/toggle
     调用的字面量，合并成「需要翻译的键」；
  3. 输出中文表未覆盖的键，供补翻；
  4. UTF-8 有效性。

退出码：发现键不对齐 / 重复键 / 格式符不一致时为 1（CI 用）；有未覆盖键时为 0，但会列清单。
"""

import argparse
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UI = os.path.join(ROOT, "runtime", "ui")

# 会被 tr() 查表的调用：函数名即其字符串参数的去向
CALL_RE = re.compile(
    r"\b(tr|header|hint|choice|toggle|opt_section|opt_toggle|opt_choice|opt_slider|opt_int|"
    r"opt_button|opt_radio|opt_action|opt_text|opt_begin)\s*\("
)
PAIR_RE = re.compile(r'\{\s*"((?:[^"\\]|\\.)*)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\}\s*,?', re.S)
# 不含空格标志：避免把 "100% is ..." 里的 "% i" 误判为格式符
FORMAT_RE = re.compile(r"%(?:\d+\$)?[-+0#]*\d*(?:\.\d+)?[diouxXeEfgGcs%]")

ESCAPES = {"n": 10, "t": 9, "r": 13, "0": 0, "a": 7, "b": 8, "f": 12, "v": 11,
           "\\": 92, '"': 34, "'": 39}
HEX = "0123456789abcdefABCDEF"


def decode_c(s):
    """C 字符串字面量 -> 真实文本。按字节解码，支持 \\xNN（如源码里的 ‹ › 用 \\xe2\\x80\\xb9 写）
    和 UTF-8 中文；\\xNN 若被逐字解码会与中文表键对不上，造成假的「缺失」。"""
    out, i = bytearray(), 0
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            n = s[i + 1]
            if n == "x":  # 十六进制转义（可能连续多个字节组成一个 UTF-8 字符）
                j = i + 2
                while j < len(s) and s[j] in HEX:
                    j += 1
                if j > i + 2:
                    out.append(int(s[i + 2:j], 16))
                    i = j
                    continue
            if n in ESCAPES:
                out.append(ESCAPES[n])
            else:
                out.extend(n.encode("utf-8"))
            i += 2
            continue
        out.extend(s[i].encode("utf-8"))
        i += 1
    return out.decode("utf-8", errors="replace")


# 葡语表 / 多语表里遗留的 C 相邻字符串「首段片段」：源码把几段拼成一个 desc/hint，
# 运行时 tr() 收到的是拼合后的完整串，这些片段键永远查不到（葡语表同样如此）。
# 中文表只收录完整串，故把它们从「必须覆盖」里剔除。
DEAD_KEYS = {
    "Frees the space that release takes. The release in play, saves and ",
    "Sets up another release of the game (1.0.0, 1.0.1 or 1.1.0) from its own folder, next to ",
}


def is_ui_text(k):
    """源码扫描出的字面量里，哪些是真正会显示给用户、因而必须有译文的。"""
    if not k or len(k) < 2 or k.startswith("#"):
        return False
    if k.startswith("/") or k.endswith((".ttf", ".png", ".zip", ".so", ".ini", ".json", ".bin", ".txt", ".h", ".mm")):
        return False
    if k.startswith("[") or "http" in k or "application/" in k:
        return False
    # 纯格式串（%d / 32 之类）：数字与标点，不含成词的英文
    if not re.search(r"[A-Za-z]{2}", k):
        return False
    return True


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def parse_table(path):
    """解析 {"English", "译文"}, 条目（允许跨行、允许注释）。"""
    rows = []
    for m in PAIR_RE.finditer(read(path)):
        rows.append((decode_c(m.group(1)), decode_c(m.group(2))))
    return rows


def parse_inc_keys(path):
    """葡语表 / 多语表：取每行第一个字符串作为英文键。"""
    keys = []
    for m in re.finditer(r'\{\s*"((?:[^"\\]|\\.)*)"', read(path)):
        keys.append(decode_c(m.group(1)))
    return keys


def scan_call(src, start):
    """从 '(' 之后扫描到配对的 ')'，按顶层逗号分组，返回每组内 C 字符串字面量的拼接结果。"""
    depth, i = 1, start
    groups, cur = [], []
    n = len(src)
    while i < n and depth > 0:
        c = src[i]
        if c == '"':  # C 字符串字面量
            j, buf = i + 1, []
            while j < n:
                if src[j] == "\\" and j + 1 < n:
                    buf.append(src[j : j + 2])
                    j += 2
                elif src[j] == '"':
                    j += 1
                    break
                else:
                    buf.append(src[j])
                    j += 1
            cur.append(decode_c("".join(buf)))
            i = j
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                break
        elif c == "," and depth == 1:
            if cur:
                groups.append("".join(cur))
                cur = []
            i += 1
            continue
        # 非空白字符出现在两段字符串之间 => 不是 C 相邻拼接（如 cond ? "A" : "B"），拆成两个候选键
        if c not in " \t\r\n" and cur:
            groups.append("".join(cur))
            cur = []
        i += 1
    if cur:
        groups.append("".join(cur))
    return groups


def source_keys():
    keys = set()
    for path in sorted(glob.glob(os.path.join(UI, "*.mm")) + glob.glob(os.path.join(UI, "*.h"))):
        src = read(path)
        for m in CALL_RE.finditer(src):
            for g in scan_call(src, m.end()):
                g = g.strip()
                # 纯 ImGui ID / 格式串 / 路径 / 空串不是界面文本
                if not g or g.startswith("#") or len(g) < 2:
                    continue
                if g.startswith("/") or g.endswith((".ttf", ".png", ".zip", ".so", ".ini")):
                    continue
                keys.add(g)
    return keys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--missing-out", help="把未覆盖的键写入该文件（每行一条）")
    args = ap.parse_args()

    hans_path = os.path.join(UI, "i18n_zh_hans.inc")
    hant_path = os.path.join(UI, "i18n_zh_hant.inc")
    for p in (hans_path, hant_path):
        if not os.path.exists(p):
            print(f"[FAIL] 缺少 {p}", file=sys.stderr)
            return 1

    hans, hant = parse_table(hans_path), parse_table(hant_path)
    hans_keys = [k for k, _ in hans]
    hant_keys = [k for k, _ in hant]
    problems = []

    print(f"简体表 {len(hans)} 条 / 繁体表 {len(hant)} 条")
    if hans_keys != hant_keys:
        only_hans = [k for k in hans_keys if k not in set(hant_keys)]
        only_hant = [k for k in hant_keys if k not in set(hans_keys)]
        problems.append(f"简繁键不对齐：仅简体 {len(only_hans)} 条、仅繁体 {len(only_hant)} 条")
        for k in (only_hans + only_hant)[:10]:
            print(f"   - {k[:90]}")
    for name, rows in (("简体", hans), ("繁体", hant)):
        dup = {k for k in [r[0] for r in rows] if [r[0] for r in rows].count(k) > 1}
        if dup:
            problems.append(f"{name}表重复键 {len(dup)} 个")
            for k in list(dup)[:10]:
                print(f"   - 重复: {k[:90]}")
        for en, tr in rows:
            if not tr.strip():
                problems.append(f"{name}表空译文: {en[:60]}")
            if FORMAT_RE.findall(en) != FORMAT_RE.findall(tr):
                problems.append(f"{name}表格式符不一致: {en[:70]}")

    known = set()
    for f in ("i18n_pt.inc", "i18n_multi.inc"):
        p = os.path.join(UI, f)
        if os.path.exists(p):
            ks = parse_inc_keys(p)
            print(f"{f}: {len(ks)} 个键")
            known |= set(ks)
    src = source_keys()
    print(f"源码 tr()/opt_* 字面量: {len(src)} 个键")
    zh = set(hans_keys)
    zh_strip = {k.strip() for k in zh}  # 中文表键可能带前导空格 / \n（拼接片段键）

    # (1) 葡语表 / 多语表的键：上游逐条维护的英文基准，但需剔除运行时查不到的拼接片段死键
    required = known - {""} - DEAD_KEYS
    missing = sorted(required - zh)
    print(f"\n必须覆盖的键 {len(required)} 个：中文表命中 {len(required & zh)} 个，缺 {len(missing)} 个")
    if missing:
        print("--- 未在中文表中的键（前 40 条）---")
        for k in missing[:40]:
            print(f"   {k[:110]}")
        if len(missing) > 40:
            print(f"   ... 其余 {len(missing) - 40} 条")
        problems.append(f"中文表缺少 {len(missing)} 个必须覆盖的键")

    # (2) 运行时 tr() 真正传入的源码字面量：按 strip 归一化检查覆盖。比葡语表更可靠——
    #     葡语表本身不完整（缺 Mods / Marin / 60 fps 等），只靠它会漏掉中文下显示英文的文本。
    src_ui = {k for k in src if is_ui_text(k)}
    src_missing = sorted(k for k in src_ui if k.strip() not in zh_strip)
    print(f"源码界面文本 {len(src_ui)} 个键：中文表命中 {len(src_ui) - len(src_missing)} 个，缺 {len(src_missing)} 个")
    if src_missing:
        print("--- 运行时会显示英文的键（前 40 条）---")
        for k in src_missing[:40]:
            print(f"   {k[:110]}")
        problems.append(f"中文表缺少 {len(src_missing)} 个运行时会查表的源码键")

    all_missing = sorted(set(missing) | set(src_missing))
    if args.missing_out:
        with open(args.missing_out, "w", encoding="utf-8") as f:
            f.write("\n".join(all_missing) + "\n")
        print(f"\n完整缺口清单: {args.missing_out}")

    # 被判定为不需要翻译的源码字面量（格式串、路径、ID 等），仅作提示
    extra = sorted(src - src_ui)
    if extra:
        print(f"\n提示：另有 {len(extra)} 个非界面文本字面量（格式串 / ID / 路径等）不要求译文")

    # UTF-8
    for p in (hans_path, hant_path):
        try:
            read(p)
        except UnicodeDecodeError as e:
            problems.append(f"{p} 不是合法 UTF-8: {e}")

    if problems:
        print("\n[FAIL] " + f"{len(problems)} 个问题:")
        for p in problems[:20]:
            print("   - " + p)
        return 1
    print("\n[OK] 键对齐、无重复、格式符一致、UTF-8 有效")
    return 0


if __name__ == "__main__":
    sys.exit(main())
