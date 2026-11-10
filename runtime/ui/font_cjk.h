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
// SIZE CAP — this is what keeps the atlas from exploding. The launcher's three sizes are 52.5 / 96.2 / 211.6 px
// (Baloo 2 path: 30*1.75, 52*1.85, 92*2.3). Merging 1.3k CJK glyphs at 211 px alone needs ~61M px, so the atlas
// would come out as 4096x32768 (ImGui's TEX_HEIGHT_MAX) — 537 MB as RGBA32, far past any phone GPU's
// GL_MAX_TEXTURE_SIZE (8192/16384). The texture then fails to upload and the app dies on the first frame.
// Rendering Chinese at most CJK_MAX_PX keeps the launcher atlas near 4096x8192 (134 MB RGBA) and the menu's
// (34/39/56/72 px, all below the cap) at 4096x4096 (67 MB).
#pragma once
#include "imgui.h"
#include "i18n.h"

#include <algorithm>
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
// 80 keeps launcher = 52.5 + 80 + 80 with ~1.3k glyphs at ~21M px -> 4096x8192 at worst.
static const float CJK_MAX_PX = 80.0f;

inline void ui_merge_cjk_font(ImFontAtlas *atlas, float size) {
#ifndef __ANDROID__
    (void)atlas; (void)size;
#else
    static ImVector<ImWchar> ranges;
    static const char *font_path;
    static bool inited = false;
    if (!inited) {
        inited = true;
        static const char *const candidates[] = {
            "/system/fonts/NotoSansCJK-Regular.ttc", "/system/fonts/NotoSansSC-Regular.otf",
            "/system/fonts/NotoSansSC-VF.ttf", "/system/fonts/NotoSansCJKsc-Regular.otf",
            // Xiaomi (HyperOS/MIUI) ships MiSans instead of Noto
            "/system/fonts/MiSans-Regular.ttf", "/system/fonts/MiSansVF.ttf",
            "/system/fonts/DroidSansFallback.ttf", "/system/fonts/NotoSansCJK-Regular.otf",
        };
        const char *override_font = getenv("LARECOMP_CJK_FONT");  // (test) any TTF/OTF/TTC with CJK glyphs
        if (override_font && *override_font) {
            if (FILE *f = fopen(override_font, "rb")) { fclose(f); font_path = override_font; }
        }
        if (!font_path) {
            for (const char *p : candidates)
                if (FILE *f = fopen(p, "rb")) { fclose(f); font_path = p; break; }
        }
        if (!font_path) {  // no CJK font on this device: English / Portuguese keep working, Chinese shows as boxes
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
        std::sort(cps.begin(), cps.end());
        cps.erase(std::unique(cps.begin(), cps.end()), cps.end());
        for (ImWchar cp : cps) { ranges.push_back(cp); ranges.push_back(cp); }
        ranges.push_back(0);
        CJK_LOG("font=%s glyphs=%d", font_path, (int)cps.size());
    }
    if (!font_path) return;

    const float s = size > CJK_MAX_PX ? CJK_MAX_PX : size;  // (see the SIZE CAP note above)
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.OversampleH = 1;   // (no horizontal oversampling: 1.3k glyphs would triple the atlas width)
    cfg.OversampleV = 1;
    cfg.SizePixels = s;    // MergeMode uses this, not the size_pixels argument alone
    cfg.GlyphRanges = ranges.Data;
    cfg.FontNo = 0;
    ImFont *merged = atlas->AddFontFromFileTTF(font_path, s, &cfg, NULL);
    CJK_LOG("merge requested=%.1f actual=%.1f glyphs=%d %s",
            size, s, (int)(ranges.Size / 2), merged ? "ok" : "FAILED");
#endif
}
