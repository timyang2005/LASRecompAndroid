# 启动器中文化说明（LASRecompAndroid fork）

本分支 `feat/zh-launcher` 把**启动器 / 应用界面**翻译成简体中文和繁体中文。
**不改动游戏本体文本**：RomFS 自带中文，由游戏内的 Language 选项控制，与界面语言是两套独立的设置。

`main` 保持为上游 `mamcjr/LASRecompAndroid` 的纯镜像，所有改动都在 `feat/zh-launcher`，便于后续同步上游。
构建与发版规范见 [`docs/CI-RELEASE.md`](CI-RELEASE.md)。

## 改动清单

| 文件 | 改动 |
|---|---|
| `runtime/ui/i18n_zh_hans.inc` / `i18n_zh_hant.inc` | 新增。简繁各 **878 条**，键逐条对齐。旧仓库移植 + 补翻 + 两轮遗漏复查，并清理了 323 条上游已改文案、运行时再也查不到的失效键 |
| `runtime/ui/i18n.mm` | 新增两张中文表；`tr()` 按 `settings_ui_language()` 分派（中文 / 葡语 / 多语 / 英文回落）；新增 `i18n_cjk_text()` |
| `runtime/ui/i18n.h` | 声明 `i18n_cjk_text()`；更新注释 |
| `runtime/settings.h` / `.c` | 新增 `UI_LANG_*` 枚举、`g_ui_language_names`、`g_settings.ui_language`、`settings_ui_language()`；`larecomp.ini` 读写 `ui_language` |
| `runtime/ui/font_cjk.h` | 新增。把打包的 CJK 字体以 `MergeMode` 合并进每个刚加载的 ImGui 字体，字形范围按中文表逐字精确生成 |
| `runtime/ui/launcher.mm` / `menu.mm` | 字号档位后合并 CJK 字体；游戏内菜单新增「界面语言 / App language」选项 |
| `runtime/ui/inventory_overlay.mm` | 同伴卡片里 3 处直接 `AddText` 的英文（Companion / Choose / kShort[]）改为走 `tr()` |
| `res/values-zh-rCN/strings.xml` / `values-zh-rTW/strings.xml` | 新增。Java 弹窗、快捷方式、通知渠道共 14 条（与 `values/strings.xml` 逐条对应） |
| `android/lasrecomp/app/src/main/assets/res/fonts/ui-cjk.otf` | 新增。Noto Sans CJK SC 子集（OFL 1.1），只含界面用到的字 |
| `tools/i18n_check.py` | 新增。键对齐 / 重复键 / 格式符 / UTF-8 + **双基准覆盖率**（葡语表键 ∪ 源码 `tr()` 实际查表键） |
| `tools/font_coverage.py` | 新增。表里用到、但打包字体与 Baloo 2 都没有的字 → 构建失败 |
| `android/lasrecomp/version.properties` | 新增。版本号唯一来源，见 `docs/CI-RELEASE.md` |
| `.github/workflows/android-apk.yml` / `i18n-check.yml` | 新增。构建签名发版 / 静态校验 |

## 语言判定

`settings_ui_language()`（`runtime/settings.c`）返回当前生效的界面语言：

| 条件 | 结果 |
|---|---|
| `larecomp.ini` 中 `ui_language` 为 1..9 | 直接采用（菜单里手动选择，立即生效） |
| `ui_language = 0`（Auto，默认） | 读手机首选语言（SDL `SDL_GetPreferredLocales`） |
| `zh` + 地区 TW / HK / MO | `UI_LANG_ZH_HANT`（繁体） |
| 其他 `zh` | `UI_LANG_ZH_HANS`（简体） |
| `pt` / `es` / `fr` / `de` / `it` / `nl` | 沿用原有的葡语表 / 多语表 |
| 其他语言、桌面构建 | 英文（保持原行为） |

测试可用环境变量覆盖系统语言：`LARECOMP_HOST_LANG=zh`、`zh-TW`、`zh-HK`、`pt`、`en` 等。
`settings_host_portuguese()` / `settings_host_language()` 仍保留（旧调用点用），但改为复用同一个判定，不再各自读一次 locale。

## 字体（不做会全是豆腐块）

启动器主字体 Baloo 2 只含 ASCII。`ui_merge_cjk_font()` 在每个 `AddFontFromFileTTF` **之后立即**调用
（ImGui 的 `MergeMode` 把新字体字形合并进前一个字体），字体来源按优先级：

1. **APK 自带** `assets/res/fonts/ui-cjk.otf`（Noto Sans CJK SC 子集，292 KB，OFL 1.1）。
   Java 侧把 `assets/res` 复制到 `files/res` 并 `setenv("LARECOMP_RES_DIR", ...)`，native 从这里读；
2. 系统字体兜底（`/system/fonts/NotoSansCJK-Regular.ttc`、`MiSans-Regular.ttf` 等），但**必须先过 `sfnt_ok()`**：
   ROM 字体可能被裁剪 / 虚拟化成 stb_truetype 解析不了的形态，release 构建里 ImGui 的断言是空的，会直接 SIGSEGV。
   不合格的字体直接跳过，中文退化成方块但不会崩；
3. `LARECOMP_CJK_FONT=<path>` 可指定其它字体用于测试。

字形范围由 `i18n_cjk_text()` 中的非 ASCII 字符**逐字**生成 `{cp, cp, ...}` 精确范围（约 1.3k 个码点），
不是整段 Unicode 区间，避免图集被无关汉字撑大。

### 字号上限（不做会启动即闪退）

中文表实际用到 **1323 个不同字符**（简繁合并去重）。启动器三档字号是 52.5 / 96.2 / **211.6**（`92 * 2.3`）。
三档都合并的话，仅标题档就需约 61M px，ImGui 最终产出 **4096×32768** 的图集（`TEX_HEIGHT_MAX`），
转 RGBA32 约 **537 MB**，远超手机 GPU 的 `GL_MAX_TEXTURE_SIZE`（通常 8192 / 16384）。

因此 `CJK_MAX_PX = 80`：超过 80px 的档位按 80px 栅格化、再由 ImGui 放大，`OversampleH/V` 固定为 1。
标题档 211.6px **不合并**——它只画 `LAS Recompiled`，英文。修复后：

| 图集 | 档位（px） | 尺寸 | RGBA |
|---|---|---|---|
| 启动器 | 52.5 / 80 | 2048×4096 | 32 MB |
| 游戏内菜单 | 34 / 39 / 56 / 72（均未超限） | 2048×4096 | 32 MB |

合并过程写 logcat，便于现场核对：

```bash
adb logcat -s LASRecompCJK
# [cjk] font=.../files/res/fonts/ui-cjk.otf bytes=... glyphs=1323
# [cjk] merge requested=211.6 actual=80.0 glyphs=1323 ok
```

## 明确不动的部分

- 游戏文本语言（`g_settings.language`、`g_language_codes`、游戏内 Language 选项、RomFS）；
- 游戏 Mod / 汉化补丁、存档数据；
- 上游 README 中的 Releases / Issues / 赞助链接；
- Java 侧纯技术性日志文本。面向用户的弹窗已在 `strings.xml` 中文化；由 Java 回传给原生层显示的**固定完整句**
  （`nativeSaveTransferDone` / `nativeImportFailure`）走 `tr()` 查表，已补入中文表。

## 译文维护

`tr()` 按英文原文**精确**查表（大小写、空格、标点都要一致），所以上游改一句文案，旧译文就查不到了。
`tools/i18n_check.py` 把「葡语表 ∪ 多语表 ∪ 源码里 `tr()/opt_*/header/hint/choice/toggle` 真正传入的字面量」
合并成必须覆盖的键集合，任何一条没译文都会让 CI 失败：

```
简体表 878 条 / 繁体表 878 条
必须覆盖的键 599 个：中文表命中 599，缺 0
源码界面文本 427 个键：中文表命中 427，缺 0
[OK] 键对齐、无重复、格式符一致、UTF-8 有效
```

维护要点：

- 上游新增界面文案 → 跑 `python tools/i18n_check.py`，缺的键会列出来（`--missing-out` 可写文件）；
- 新增译文里出现新字 → 跑 `python tools/font_coverage.py`，缺字形会失败；重建子集命令在脚本头部注释；
- 只有中文表收录、源码里已经不存在的键会被当成「死键」清理掉——上一轮已清 323 条
  （旧仓库移植时带的、上游后来改写过的文案）。

## 验证

```bash
python tools/i18n_check.py
python tools/font_coverage.py
```

真机 / 模拟器验证项：

- `LARECOMP_HOST_LANG=zh` 启动：启动器、设置页、游戏内菜单全为简体；`zh-TW` 为繁体；
- 无豆腐块、长句在 52 / 92 号字下不溢出截断；
- 菜单「界面语言」手动切换后立即生效，重启后仍保持（`larecomp.ini` 的 `ui_language`）；
- Java 弹窗（导入存档、驱动错误、快捷方式）为中文；
- 游戏内文本仍由游戏 Language 选项控制，未被改动。

## 已知限制

1. **Java 资源跟随系统语言**：应用内手动选择界面语言只影响原生层，Java 弹窗仍按系统语言选资源。若要完全对齐，
   可在 Android 13+ 调用 `LocaleManager.setApplicationLocales`。
2. **字体图集**：启动器两个档位合并后 2048×4096 / 32 MB（上游无中文时 8 MB）；若某机型超限，可去掉
   `menu.mm` 里 `g_menu_font_big` 的 `ui_merge_cjk_font` 调用，大字回落到系统字体。
3. **Java 动态拼接的错误文案仍为英文**：`SaveTransfer.java` / `ImportStorage.java` 里
   `"Could not open " + name`、`String.format(...)` 生成的整串会作为 `job_hint` / `g_save_msg` 传给 `tr()`，
   键里含运行时才有的文件名或数字，无法用固定键命中。要完全覆盖需把异常消息体系改成资源字符串。
4. **性能叠加层（perf_overlay.mm）保留英文**：调试用 HUD，几乎全是 `FPS %.0f`、`RAM %u MB` 这类格式串与技术缩写。
5. ImGui ID 不受影响：`option_rows.mm` / `menu.mm` 用英文原串 `PushID(label)`，切换语言不会改变控件 ID。
