// Merge the phone's CJK font into an ImGui font that was just added with AddFont.
//
// Baloo 2 SemiBold (the launcher / menu face) covers ASCII only: without this, every Chinese string drawn with it
// is a box. ImGui's MergeMode attaches the new font's glyphs to the *previously added* font, so call this directly
// after each AddFontFromFileTTF that may draw Chinese (see launcher.mm init, menu.mm init).
//
// The glyph ranges are exact per-character ranges built from i18n_cjk_text() (not a contiguous Unicode block), so
// the atlas holds only the ~1.3k glyphs the translations actually use. They are static: ImFontConfig::GlyphRanges
// is still read when the atlas is built, long after this function returns.
//
// MEMORY — three things keep this from killing the app at start-up on a phone:
//  * the font file is read ONCE into a static buffer and shared by every merge (AddFontFromFileTTF would keep its
//    own copy per call: a 19 MB NotoSansCJK .ttc, three sizes, ~57 MB before the atlas even exists);
//  * the merge is skipped for a font that is not there yet (Atlas->Fonts empty: MergeMode indexes the previous
//    font, and in a release build that assert compiles out);
//  * SIZE CAP, see below.
//
// SIZE CAP — the launcher's three sizes were 52.5 / 96.2 / 211.6 px (Baloo 2 path: 30*1.75, 52*1.85, 92*2.3).
// Merging 1.3k CJK glyphs at 211 px alone needs ~61M px: the atlas came out 4096x32768 (ImGui's TEX_HEIGHT_MAX),
// 537 MB as RGBA32, past any phone GPU's maxImageDimension. Rendering Chinese at most CJK_MAX_PX keeps it in
// check, and the 211 px title is not merged at all: it draws "LAS Recompiled", which is English.
// Measured with this ImGui (1.91.9) and NotoSansCJK-Regular.ttc: launcher 4096x4096 RGBA = 64 MB before, and after
// the two changes above 2048x4096 = 32 MB - with the upstream (no Chinese) build at 1024x2048 = 8 MB.
#pragma once
#include "imgui.h"
#include "i18n.h"

#include <stdio.h>
#include <stdlib.h>  // getenv
#include <vector>

#if defined(__ANDROID__)
#include <android/log.h>  // (liblog is already linked: see CMakeLists target_link_libraries)
#define CJK_LOG(...) __android_log_print(ANDROID_LOG_INFO, "LASRecompCJK", __VA_ARGS__)
#else
#define CJK_LOG(...) ((void)0)
#endif

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
// The font file and the exact glyph ranges, loaded once (the start screen's callbacks run on Java's threads).
struct UiCjkFont {
    std::vector<char> bytes;
    ImVector<ImWchar> ranges;
    const char *path = NULL;

    UiCjkFont() {
        static const char *const candidates[] = {
            "/system/fonts/NotoSansCJK-Regular.ttc", "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/NotoSansSC-VF.ttf", "/system/fonts/NotoSansCJKsc-Regular.otf",
            // Xiaomi (HyperOS/MIUI) ships MiSans instead of Noto
            "/system/fonts/MiSans-Regular.ttf", "/system/fonts/MiSansVF.ttf",
            "/system/fonts/DroidSansFallback.ttf", "/system/fonts/NotoSansCJK-Regular.otf",
        };
        const char *override_font = getenv("LARECOMP_CJK_FONT");  // (test) any TTF/OTF/TTC with CJK glyphs
        if (override_font && *override_font && read(override_font)) path = override_font;
        if (!path)
            for (const char *p : candidates)
                if (read(p)) { path = p; break; }
        if (!path) {  // no CJK font on this device: English / Portuguese keep working, Chinese shows as boxes
            CJK_LOG("no system CJK font found; Chinese will render as boxes");
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
            if (cp >= 0x2000 && cp <= 0xFFFF) cps.push_back((ImWchar)cp);  // CJK, CJK punctuation, fullwidth forms
        }
        for (size_t i = 1; i < cps.size(); i++)  // insertion sort: ~1.3k entries, no <algorithm> in this header
            for (size_t j = i; j > 0 && cps[j - 1] > cps[j]; j--) { ImWchar t = cps[j]; cps[j] = cps[j - 1]; cps[j - 1] = t; }
        ImWchar last = 0;
        for (ImWchar cp : cps)
            if (cp != last) { ranges.push_back(cp); ranges.push_back(cp); last = cp; }
        ranges.push_back(0);
        CJK_LOG("font=%s bytes=%d glyphs=%d", path, (int)bytes.size(), (int)(ranges.Size / 2));
    }

    bool read(const char *p) {
        FILE *f = fopen(p, "rb");
        if (!f) return false;
        if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
        long n = ftell(f);
        if (n < 100) { fclose(f); return false; }  // (not a font: ImGui asserts on a tiny buffer)
        rewind(f);
        bytes.resize((size_t)n);
        size_t got = fread(bytes.data(), 1, (size_t)n, f);
        fclose(f);
        if (got != (size_t)n) { bytes.clear(); return false; }
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
    cfg.SizePixels = s;    // MergeMode uses this, not the size_pixels argument alone
    cfg.FontDataOwnedByAtlas = false;  // (the bytes live in the static above: the atlas must not free them)
    cfg.GlyphRanges = f.ranges.Data;
    cfg.FontNo = 0;
    ImFont *merged = atlas->AddFontFromMemoryTTF(f.bytes.data(), (int)f.bytes.size(), s, &cfg, NULL);
    CJK_LOG("merge requested=%.1f actual=%.1f glyphs=%d %s",
            size, s, (int)(f.ranges.Size / 2), merged ? "ok" : "FAILED");
#endif
}
