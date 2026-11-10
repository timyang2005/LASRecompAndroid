// The menus' own text on Android: the English string is looked up in the table for settings_ui_language()
// (ui/i18n_pt.inc, i18n_multi.inc, i18n_zh_hans.inc, i18n_zh_hant.inc); a missing key or an empty translation
// shows the English. English / desktop: tr() is the string itself. Display text only: labels used as ImGui IDs
// stay the English ones (PushID(label) in option_rows.mm / menu.mm), so switching language cannot change an ID.
#pragma once
#include <string>

const char *tr(const char *en);
// In capitals (section titles, row labels): ASCII, and the accented Latin-1 letters in UTF-8 (á -> Á, ç -> Ç)
std::string ui_upper(const char *s);
// (Android) All characters the Chinese tables use, as UTF-8; ui/font_cjk.h builds glyph ranges from it. "" elsewhere.
const char *i18n_cjk_text(void);
