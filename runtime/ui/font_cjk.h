// Merge the phone's CJK font into an ImGui font that was just added with AddFont.
//
// Baloo 2 SemiBold (the launcher / menu face) covers ASCII only: without this, every Chinese string drawn with it
// is a box. ImGui's MergeMode attaches the new font's glyphs to the *previously added* font, so call this directly
// after each AddFontFromFileTTF that may draw Chinese (see launcher.mm init, menu.mm init).
//
// The glyph ranges are exact per-character ranges built from i18n_cjk_text() (not a contiguous Unicode block), so
// the atlas holds only the ~1.5k glyphs the translations actually use. They are static: ImFontConfig::GlyphRanges
// is still read when the atlas is built, long after this function returns.
#pragma once
#include "imgui.h"
#include "i18n.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>  // getenv
#include <vector>

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
        if (!font_path) return;  // no CJK font on this device: English / Portuguese keep working

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
    }
    if (!font_path) return;

    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.GlyphRanges = ranges.Data;
    cfg.FontNo = 0;
    atlas->AddFontFromFileTTF(font_path, size, &cfg, NULL);
#endif
}
