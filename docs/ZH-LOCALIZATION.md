# 启动器中文化说明（LASRecompAndroid fork）

本分支 `feat/zh-launcher` 把**启动器 / 应用界面**翻译成简体中文和繁体中文，依据
[zh-launcher-i18n-plan.md](zh-launcher-i18n-plan.md) 的方案实施。**不改动游戏本体文本**：RomFS 自带中文，由游戏内的
Language 选项控制，与界面语言是两套独立的设置。

## 改动清单

| 文件 | 改动 |
|---|---|
| `runtime/ui/i18n_zh_hans.inc` / `i18n_zh_hant.inc` | 新增。移植旧仓库 `29873f7` 的简繁各 891 条，并为新仓库补翻 262 条，共 **1153 条**，键逐条对齐 |
| `runtime/ui/i18n.mm` | 新增两张中文表；`tr()` 改为按 `settings_ui_language()` 分派（中文 / 葡语 / 多语 / 英文回落）；新增 `i18n_cjk_text()` |
| `runtime/ui/i18n.h` | 声明 `i18n_cjk_text()`；更新注释 |
| `runtime/settings.h` / `.c` | 新增 `UI_LANG_*` 枚举、`g_ui_language_names`、`g_settings.ui_language`、`settings_ui_language()`；`larecomp.ini` 读写 `ui_language` |
| `runtime/ui/font_cjk.h` | 新增。把手机系统 CJK 字体以 `MergeMode` 合并进每个刚加载的 ImGui 字体，字形范围按中文表逐字精确生成 |
| `runtime/ui/launcher.mm` / `menu.mm` | 三档 / 四档字号后合并 CJK 字体；游戏内菜单新增「界面语言 / App language」选项 |
| `res/values-zh-rCN/strings.xml` / `values-zh-rTW/strings.xml` | 新增。Java 弹窗、快捷方式、通知渠道共 14 条 |
| `tools/i18n_check.py` | 新增。键对齐 / 重复键 / 格式符 / 覆盖率 / UTF-8 静态校验 |
| `.github/workflows/i18n.yml` | 新增。push/PR 跑校验；手动触发可打 APK |

`main` 保持为上游 `mamcjr/LASRecompAndroid` 的纯镜像，所有改动都在 `feat/zh-launcher`，便于后续 rebase 上游更新。

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
注意 `settings_host_portuguese()` / `settings_host_language()` 仍保留，但改为复用同一个判定，不再各自读一次 locale。

## 字体（不做会全是豆腐块）

启动器主字体 Baloo 2 只含 ASCII。`ui_merge_cjk_font()` 在每个 `AddFontFromFileTTF` **之后立即**调用
（ImGui 的 `MergeMode` 把新字体字形合并进前一个字体），依次尝试：

```
/system/fonts/NotoSansCJK-Regular.ttc
/system/fonts/NotoSansSC-Regular.otf
/system/fonts/NotoSansSC-VF.ttf
/system/fonts/NotoSansCJKsc-Regular.otf
/system/fonts/DroidSansFallback.ttf
/system/fonts/NotoSansCJK-Regular.otf
```

`LARECOMP_CJK_FONT=<path>` 可指定其它字体用于测试。字形范围由 `i18n_cjk_text()` 中的非 ASCII 字符**逐字**生成
`{cp, cp, ...}` 精确范围（约 1.5k 个码点），不是整段 Unicode 区间，避免图集被无关汉字撑大。设备上找不到 CJK 字体时
保持英文 / 葡语原样，不会崩溃。

## 明确不动的部分

- 游戏文本语言（`g_settings.language`、`g_language_codes`、游戏内 Language 选项、RomFS）；
- 游戏 Mod / 汉化补丁、存档数据；
- 上游 README 中的 Releases / Issues / 赞助链接（仍指向 `mamcjr`，本 fork 未开启 Issues）；
- Java 侧纯技术性异常文本（日志、`IOException` 消息）。面向用户的弹窗已在 `strings.xml` 中文化；由 Java 回传给
  原生层显示的消息（`nativeSaveTransferDone` / `nativeImportFailure`）走 `tr()` 查表，已被中文表覆盖。

## 验证

```bash
python3 tools/i18n_check.py            # 键对齐 / 重复 / 格式符 / 覆盖率 / UTF-8
python3 tools/i18n_check.py --missing-out /tmp/missing.txt
```

真机 / 模拟器验证项：

- `LARECOMP_HOST_LANG=zh` 启动：启动器、设置页、游戏内菜单全为简体；`zh-TW` 为繁体；
- 无豆腐块、长句在 52 / 92 号字下不溢出截断；
- 菜单「界面语言」手动切换后立即生效，重启后仍保持（`larecomp.ini` 的 `ui_language`）；
- Java 弹窗（导入存档、驱动错误、快捷方式）为中文；
- 游戏内文本仍由游戏 Language 选项控制，未被改动。

## 构建与 CI

APK 需要 Windows Git Bash 或 Linux + Android SDK：**NDK `30.0.16248370`、CMake `3.30.4`、Python 3.12**，步骤见
README 的 *Building from source*（`android/deps/build_deps.sh` 等）。CI 中 `i18n` 任务在 ubuntu-latest 上跑静态校验，
必定可用；`apk` 任务仅 `workflow_dispatch` 手动触发，Linux 上的原生依赖构建未经上游验证。

## 已知限制

1. **Java 资源跟随系统语言**：应用内手动选择界面语言只影响原生层，Java 弹窗仍按系统语言选资源。若要完全对齐，
   可在 Android 13+ 调用 `LocaleManager.setApplicationLocales`。
2. **字体图集**：三档大字号合并全部汉字时图集可能较大；若标题档超限，可去掉 `launcher.mm` 中 `g_font_title` 的
   `ui_merge_cjk_font` 调用，标题回落到系统字体。
3. **键必须逐字匹配**：`tr()` 按英文原文精确查表，标点 / 空格 / 大小写不同都会回落英文，改动 UI 文案后请重跑
   `i18n_check.py`。
4. ImGui ID 不受影响：`option_rows.mm` / `menu.mm` 用英文原串 `PushID(label)`，切换语言不会改变控件 ID。
