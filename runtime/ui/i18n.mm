// See i18n.h.
#include "i18n.h"

#include <ctype.h>
#include <string.h>
#include <string>
#include <unordered_map>

extern "C" {
#include "../settings.h"
}

#ifdef __ANDROID__
static const char *const kPt[][2] = {
#include "i18n_pt.inc"
};
static const char *const kMulti[][6] = {  // English, then es, fr, de, it, nl (UI_LANG_ES..UI_LANG_NL)
#include "i18n_multi.inc"
};
static const char *const kZhHans[][2] = {  // English, 简体中文 (UI_LANG_ZH_HANS)
#include "i18n_zh_hans.inc"
};
static const char *const kZhHant[][2] = {  // English, 繁體中文 (UI_LANG_ZH_HANT)
#include "i18n_zh_hant.inc"
};

// Built once, thread-safe (the start screen's callbacks run on Java's threads). An empty translation falls back to
// the English, as a missing key does.
static const std::unordered_map<std::string, const char *> *make_map(const char *const (*rows)[2], size_t n) {
    auto *m = new std::unordered_map<std::string, const char *>();
    m->reserve(n * 2);
    for (size_t i = 0; i < n; i++)
        if (*rows[i][1]) m->emplace(rows[i][0], rows[i][1]);
    return m;
}
#endif

const char *tr(const char *en) {
#ifdef __ANDROID__
    if (!en || !*en) return en;
    const int lang = settings_ui_language();
    if (lang == UI_LANG_EN || lang <= UI_LANG_AUTO || lang >= N_UI_LANGUAGES) return en;

    static const auto *pt = make_map(kPt, sizeof kPt / sizeof kPt[0]);
    static const auto *hans = make_map(kZhHans, sizeof kZhHans / sizeof kZhHans[0]);
    static const auto *hant = make_map(kZhHant, sizeof kZhHant / sizeof kZhHant[0]);
    static const auto *multi = [] {
        auto *m = new std::unordered_map<std::string, const char *const *>();
        for (const auto &e : kMulti) m->emplace(e[0], e);
        return m;
    }();

    const std::unordered_map<std::string, const char *> *table = NULL;
    if (lang == UI_LANG_ZH_HANS) table = hans;
    else if (lang == UI_LANG_ZH_HANT) table = hant;
    else if (lang == UI_LANG_PT_BR) table = pt;
    else if (lang >= UI_LANG_ES && lang <= UI_LANG_NL) {  // kMulti: [0] English, [1..5] es/fr/de/it/nl
        auto it = multi->find(en);
        return it == multi->end() || !*it->second[lang - UI_LANG_ES + 1] ? en : it->second[lang - UI_LANG_ES + 1];
    }
    if (!table) return en;
    auto it = table->find(en);
    return it == table->end() ? en : it->second;
#else
    return en;
#endif
}

// Every character the Chinese tables use, as UTF-8: ui/font_cjk.h turns the non-ASCII ones into exact glyph ranges
// for the merged system CJK font (Baloo 2 covers ASCII only). Both tables, so switching scripts needs no rebuild.
const char *i18n_cjk_text(void) {
#ifdef __ANDROID__
    static const std::string all = [] {
        std::string s;
        for (const auto &e : kZhHans) s += e[1], s += '\n';
        for (const auto &e : kZhHant) s += e[1], s += '\n';
        return s;
    }();
    return all.c_str();
#else
    return "";
#endif
}

std::string ui_upper(const char *s) {
    std::string t = s ? s : "";
    for (size_t i = 0; i < t.size(); i++) {
        unsigned char c = (unsigned char)t[i];
        if (c < 0x80) t[i] = (char)toupper(c);
#ifdef __ANDROID__  // (the desktop's menus are English: bytes as before)
        else if (c == 0xC3 && i + 1 < t.size()) {  // U+00E0..U+00FE: the lowercase accented letters (but ÷, U+00F7)
            unsigned char d = (unsigned char)t[i + 1];
            if (d >= 0xA0 && d <= 0xBE && d != 0xB7) t[i + 1] = (char)(d - 0x20);
            i++;
        }
#endif
    }
    return t;
}
