// Merge a CJK font into an ImGui font that was just added with AddFont.
//
// Baloo 2 SemiBold (the launcher / menu face) covers ASCII only: without this, every Chinese string drawn with it
// is a box. ImGui's MergeMode attaches the new font's glyphs to the *previously added* font, so call this directly
// after each AddFontFromFileTTF that may draw Chinese (see launcher.mm init, menu.mm init).
//
// The glyph ranges are exact per-character ranges built from i18n_cjk_text() (not a contiguous Unicode block), so
// the atlas holds only the ~1.3k glyphs the translations actually use. They are static: ImFontConfig::GlyphRanges
// is still read when the atlas is built, long after this function returns.
//
// FONT SOURCE - the APK ships one (assets/res/fonts/ui-cjk.otf, a subset of Noto Sans CJK SC with exactly the
// glyphs the tables use, OFL 1.1). The phone's own fonts are only a fallback:
//  * a ROM font is not a font we control. Xiaomi ships MiSans, whose variable (CFF2) build stb_truetype cannot
//    parse, and a collection whose first font offset points past the end of the buffer takes the app down in
//    ImFontAtlasBuildWithStbTruetype instead of failing (see sfnt_ok below);
//  * the bundled subset is 292 KB read once, against 19-30 MB of system font - and it loads before the atlas is
//    built, not on the first frame.
//
// SFNT CHECK - ImGui's guards do not exist in a release build: IM_ASSERT compiles out, so "font_data_size > 100",
// "stbtt_InitFont() failed to parse FontData" and "FontData is incorrect, or FontNo cannot be found" are all no-ops
// and the bad buffer is indexed anyway. Measured with this ImGui (1.91.9): a ttcf header with an out-of-range offset
// (what a trimmed or virtualised ROM font looks like) exits 139 = SIGSEGV in an -O2 -DNDEBUG build, prints nothing,
// and leaves the log ending at the Vulkan init lines - exactly the report from the phone. So every candidate is
// validated here first, and a rejected font is simply not merged (Chinese shows as boxes, nothing crashes).
//
// SIZE CAP - the launcher's three sizes are 52.5 / 96.2 / 211.6 px (Baloo 2 path: 30*1.75, 52*1.85, 92*2.3).
// Merging the 1.3k glyphs at 211 px alone needs ~61M px: the atlas came out 4096x32768 (ImGui's TEX_HEIGHT_MAX),
// 537 MB as RGBA32, past any phone GPU's maxImageDimension. Capping at CJK_MAX_PX keeps it in check, and the
// 211 px face is not merged at all: it draws "LAS Recompiled", which is English.
// Measured with this ImGui and the bundled subset: launcher 2 merged faces = 2048x4096 RGBA 32 MB (upstream, no
// Chinese: 1024x2048 = 8 MB); a single merged face is 1024x2048 = 8 MB.
#pragma once
#include "imgui.h"
#include "i18n.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <android/log.h>  // (liblog is already linked: see CMakeLists target_link_libraries)
#endif

// One line to both places: the log file (stderr is redirected to <data>/lasrecomp.log on Android, see
// platform/android.c) and logcat. The file survives the crash, which is the point.
inline void cjk_log(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fprintf(stderr, "[cjk] %s\n", buf);
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_INFO, "LASRecompCJK", "%s", buf);
#endif
}

// Chinese above this height is rasterised at this height and scaled up by ImGui (per-font SizePixels).
static const float CJK_MAX_PX = 80.0f;

// How the merge is done, from the build (-DLAS_CJK_MODE, build.gradle.kts -PlasCjkMode):
//   0  never merge  - the Chinese tables still translate, the text draws as boxes (bisect: is it the font at all?)
//   1  normal       - see SIZE CAP
//   2  one size     - only faces up to 60 px, rasterised at 40 px (bisect: is it the atlas size?)
#ifndef LAS_CJK_MODE
#define LAS_CJK_MODE 1
#endif

#if defined(__ANDROID__)
static unsigned int sfnt_u32(const unsigned char *p) {
    return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | p[3];
}
static unsigned int sfnt_u16(const unsigned char *p) { return ((unsigned)p[0] << 8) | p[1]; }

// Can stb_truetype (the copy inside imgui_draw.cpp) parse this buffer? Mirrors what it needs, and nothing more:
// a single font or a collection pointing at one, a table directory that stays inside the buffer, a cmap and an
// outline table it understands (glyf+TrueType, or CFF version 1 - it has no CFF2 and no CFF). base is where the
// font's own header starts inside d (0 for a plain file, the face offset for a collection: its table offsets are
// still counted from the start of the file, which is what stb does). depth guards a collection of collections.
static bool sfnt_ok(const unsigned char *d, long n, long base, int depth) {
    if (!d || n < 12 || base < 0 || base > n - 12) return false;
    const unsigned char *h = d + base;
    if (memcmp(h, "ttcf", 4) == 0) {
        if (depth) return false;
        unsigned int num = sfnt_u32(h + 8);  // (0 faces, or offsets past the end: a trimmed / virtualised font)
        if (num == 0 || num > 64) return false;
        if (12 + (long)num * 4 > n - base) return false;
        unsigned int off = sfnt_u32(h + 12);
        if (off < 12 || (long)off > n - 12) return false;
        return sfnt_ok(d, n, (long)off, 1);
    }
    static const unsigned char truetype[4] = {0x00, 0x01, 0x00, 0x00};
    const bool bare = memcmp(h, truetype, 4) == 0 || memcmp(h, "true", 4) == 0 || memcmp(h, "typ1", 4) == 0;
    const bool otto = memcmp(h, "OTTO", 4) == 0;  // (OpenType wrapper: the outline is CFF, or CFF2)
    if (!bare && !otto) return false;
    int tables = (int)sfnt_u16(h + 4);
    if (tables <= 0 || tables > 512) return false;
    if (12 + (long)tables * 16 > n - base) return false;
    bool cmap = false, outline = false, cff2 = false;
    for (int i = 0; i < tables; i++) {
        const unsigned char *rec = h + 12 + i * 16;
        unsigned int off = sfnt_u32(rec + 8), len = sfnt_u32(rec + 12);  // (from the start of the file)
        if (memcmp(rec, "cmap", 4) == 0) cmap = true;
        if (memcmp(rec, "glyf", 4) == 0) outline = true;
        if (memcmp(rec, "CFF ", 4) == 0) outline = true;  // (note the space: CFF2 is a different tag)
        if (memcmp(rec, "CFF2", 4) == 0) cff2 = true;
        if (off > (unsigned)n || len > (unsigned)n - off) return false;  // (truncated file)
    }
    if (cff2 && !outline) return false;
    return cmap && outline;
}

// The font file and the exact glyph ranges, loaded once (the start screen's callbacks run on Java's threads).
struct UiCjkFont {
    std::vector<char> bytes;
    ImVector<ImWchar> ranges;
    std::string path_buf;  // (path points into this: a local char[] would dangle once the constructor returns)
    const char *path = NULL;

    UiCjkFont() {
        char bundled[512] = {0};
        const char *res = getenv("LARECOMP_RES_DIR");  // (the APK's res/ on internal storage: LASRecompActivity)
        if (res && *res) snprintf(bundled, sizeof bundled, "%s/fonts/ui-cjk.otf", res);

        // The phone's own fonts, only if the bundled one cannot be used. A ROM may rename, trim or virtualise any
        // of these; try_load rejects what stb_truetype cannot parse, so a wrong answer costs a box glyph, not a crash.
        static const char *const system_fonts[] = {
            "/system/fonts/NotoSansCJK-Regular.ttc", "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/NotoSansSC-VF.ttf", "/system/fonts/NotoSansCJKsc-Regular.otf",
            "/system/fonts/MiSans-Regular.ttf", "/system/fonts/MiSansVF.ttf",  // (Xiaomi: HyperOS / MIUI)
            "/system/fonts/DroidSansFallback.ttf", "/system/fonts/NotoSansCJK-Regular.otf",
        };

        const char *override_font = getenv("LARECOMP_CJK_FONT");  // (test) any TTF/OTF/TTC with CJK glyphs
        if (override_font && *override_font && try_load(override_font, "override")) path = override_font;
        if (!path && bundled[0] && try_load(bundled, "bundled")) { path_buf = bundled; path = path_buf.c_str(); }
        if (!path)
            for (const char *p : system_fonts)
                if (try_load(p, "system")) { path = p; break; }
        if (!path) {  // nothing usable: English / Portuguese keep working, Chinese shows as boxes
            cjk_log("no usable CJK font; Chinese will render as boxes");
            return;
        }

        // Exact ranges: one [cp, cp] pair per non-ASCII codepoint used by the Chinese tables, sorted and unique.
        const unsigned char *p = (const unsigned char *)i18n_cjk_text();
        auto next = [&p]() -> unsigned int {
            unsigned int c = *p++;
            if (c < 0x80) return c;
            int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
            c &= (1u << (6 - n)) - 1;
            for (int i = 0; i < n; i++) c = (c << 6) | (*p++ & 0x3F);
            return c;
        };
        std::vector<ImWchar> cps;
        while (*p) {
            unsigned int cp = next();
            // Everything above Latin-1: CJK, CJK punctuation, fullwidth forms - and the dashes / quotes / arrows
            // the tables use, which Baloo 2 does not have (its ranges stop at U+00FF).
            if (cp > 0xFF && cp <= 0xFFFF) cps.push_back((ImWchar)cp);
        }
        for (size_t i = 1; i < cps.size(); i++)  // insertion sort: ~1.3k entries, no <algorithm> in this header
            for (size_t j = i; j > 0 && cps[j - 1] > cps[j]; j--) { ImWchar t = cps[j]; cps[j] = cps[j - 1]; cps[j - 1] = t; }
        ImWchar last = 0;
        for (ImWchar cp : cps)
            if (cp != last) { ranges.push_back(cp); ranges.push_back(cp); last = cp; }
        ranges.push_back(0);
        cjk_log("font=%s bytes=%d glyphs=%d", path, (int)bytes.size(), (int)(ranges.Size / 2));
    }

    // Read one candidate in full; keep it only if stb_truetype can parse it. A short read is a failure too (a font
    // served through FUSE can come back short), as is anything the atlas builder would trip over.
    bool try_load(const char *p, const char *what) {
        FILE *f = fopen(p, "rb");
        if (!f) return false;
        if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
        long n = ftell(f);
        if (n < 100) {  // (ImGui asserts on a tiny buffer - and does nothing in a release build)
            fclose(f);
            cjk_log("%s %s: %ld bytes, too small", what, p, n);
            bytes.clear();
            return false;
        }
        rewind(f);
        bytes.resize((size_t)n);
        size_t got = fread(bytes.data(), 1, (size_t)n, f);
        fclose(f);
        if (got != (size_t)n) {
            cjk_log("%s %s: short read %d of %ld", what, p, (int)got, n);
            bytes.clear();
            return false;
        }
        if (!sfnt_ok((const unsigned char *)bytes.data(), n, 0, 0)) {
            cjk_log("%s %s: %ld bytes, not a font stb_truetype can parse - skipped", what, p, n);
            bytes.clear();
            return false;
        }
        cjk_log("%s %s: %ld bytes, ok", what, p, n);
        return true;
    }
};

inline UiCjkFont &ui_cjk_font(void) {
    static UiCjkFont f;  // (C++11 magic static: built once, thread-safe)
    return f;
}
#endif

inline void ui_merge_cjk_font(ImFontAtlas *atlas, float size) {
#if !defined(__ANDROID__) || LAS_CJK_MODE == 0
    (void)atlas; (void)size;  // (no Android / bisect build: translate, but never touch the atlas)
#else
    UiCjkFont &f = ui_cjk_font();
    if (f.bytes.empty() || f.ranges.Size < 3) return;
    if (atlas->Fonts.Size == 0) return;  // (MergeMode attaches to the previous font: none yet, nothing to attach to)

#if LAS_CJK_MODE == 2
    if (size > 60.0f) return;  // (bisect build: one small face only)
    const float cap = 40.0f;
#else
    const float cap = CJK_MAX_PX;
#endif
    const float s = size > cap ? cap : size;  // (see the SIZE CAP note above)
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.OversampleH = 1;   // (no horizontal oversampling: 1.3k glyphs would triple the atlas width)
    cfg.OversampleV = 1;
    cfg.SizePixels = s;    // (MergeMode uses this, not the size_pixels argument alone)
    cfg.FontDataOwnedByAtlas = false;  // (the bytes live in the static above: the atlas must not free them)
    cfg.GlyphRanges = f.ranges.Data;
    cfg.FontNo = 0;
    // (the two sizes must agree: with SizePixels and size_pixels apart, ImGui drops most of the merged glyphs)
    ImFont *merged = atlas->AddFontFromMemoryTTF(f.bytes.data(), (int)f.bytes.size(), s, &cfg, NULL);
    cjk_log("merge requested=%.1f actual=%.1f glyphs=%d %s",
            size, s, (int)(f.ranges.Size / 2), merged ? "ok" : "FAILED");
#endif
}
