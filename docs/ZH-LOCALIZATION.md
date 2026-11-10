# 启动器中文化说明（LASRecompAndroid fork）

本分支 `feat/zh-launcher` 把**启动器 / 应用界面**翻译成简体中文和繁体中文，依据
[zh-launcher-i18n-plan.md](zh-launcher-i18n-plan.md) 的方案实施。**不改动游戏本体文本**：RomFS 自带中文，由游戏内的
Language 选项控制，与界面语言是两套独立的设置。

## 改动清单

| 文件 | 改动 |
|---|---|
| `runtime/ui/i18n_zh_hans.inc` / `i18n_zh_hant.inc` | 新增。移植旧仓库 `29873f7` 的简繁各 891 条，补翻 262 条，遗漏复查再补 50 条并删去 2 条半句死键，共 **1201 条**，键逐条对齐 |
| `runtime/ui/i18n.mm` | 新增两张中文表；`tr()` 改为按 `settings_ui_language()` 分派（中文 / 葡语 / 多语 / 英文回落）；新增 `i18n_cjk_text()` |
| `runtime/ui/i18n.h` | 声明 `i18n_cjk_text()`；更新注释 |
| `runtime/settings.h` / `.c` | 新增 `UI_LANG_*` 枚举、`g_ui_language_names`、`g_settings.ui_language`、`settings_ui_language()`；`larecomp.ini` 读写 `ui_language` |
| `runtime/ui/font_cjk.h` | 新增。把手机系统 CJK 字体以 `MergeMode` 合并进每个刚加载的 ImGui 字体，字形范围按中文表逐字精确生成 |
| `runtime/ui/launcher.mm` / `menu.mm` | 三档 / 四档字号后合并 CJK 字体；游戏内菜单新增「界面语言 / App language」选项 |
| `runtime/ui/inventory_overlay.mm` | 同伴卡片里 3 处直接 `AddText` 的英文（Companion / Choose / kShort[]）改为走 `tr()` |
| `res/values-zh-rCN/strings.xml` / `values-zh-rTW/strings.xml` | 新增。Java 弹窗、快捷方式、通知渠道共 14 条 |
| `tools/i18n_check.py` | 新增。键对齐 / 重复键 / 格式符 / UTF-8 + **双基准覆盖率**（葡语表键 ∪ 源码 `tr()` 实际查表键） |
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
- Java 侧纯技术性日志文本。面向用户的弹窗已在 `strings.xml` 中文化；由 Java 回传给原生层显示的**固定完整句**
  （`nativeSaveTransferDone` / `nativeImportFailure`）走 `tr()` 查表，已补入中文表（见下节）。

## 遗漏复查（第二轮）

首轮只以葡语表 / 多语表的 601 个键作为「英文基准」，会漏掉上游葡语表本身没收的界面文本。第二轮改为
**从源码直接提取 `tr()` / `opt_*` 真正传入的字符串**（含 C 相邻字符串拼接后的完整串），逐类核对，补了 50 条、
删了 2 条错误键：

| 类别 | 发现 | 处理 |
|---|---|---|
| C 相邻拼接长串 | `Sets up another release…`、`Frees the space that release takes…`、`menu.mm` 桌面版帧率说明：源码把 4 段拼成 1 个 `desc`，旧中文表存的是**首段片段**（译文也只翻了半句），运行时拿完整串查表必然落空 | 删 2 条半句死键，按源码精确提取的完整串（含 `‹ ›`）重新翻译 3 条 |
| 选项名 / 标签 | `Mods`、`Marin`、`BowWow`、`Rooster`、`60 fps`、`120 Hz` 是 `opt_choice` 的 `names[]` 或标签数组元素，helper 内部 `tr(names[i])` 会查表，但葡语表未收录 | 补 6 条 |
| 启动器状态 / 错误提示 | `set_job("Copying the game data image...")`、`NewStringUTF(...)` 回传的 romfs 复制 / 就地使用失败文案，最终经 `tr(job)` / `tr(job_hint)` 显示 | 补 10 条 |
| Java 回传固定句 | `nativeImportFailure` / `nativeSaveTransferDone` 的完整句（权限提示、存档校验失败等） | 补 19 条 |
| 直接绘制的英文 | `inventory_overlay.mm` 同伴卡片的 `Companion`、`Choose`、`kShort[]` 直接 `AddText`，根本没包 `tr()` | 源码加 `#include "i18n.h"` 并包 `tr()` |
| 品牌 / 技术名 / 格式串 | `Nintendo`、`AMD FSR 1`、`%d / 32` 等本来就应保留英文 | 显式登记为「译文 = 原文」，使严格覆盖检查无需白名单 |

同时加固 `tools/i18n_check.py`：修正 `\xNN` 十六进制转义解码（源码里的 `‹ ›`），并把**源码界面文本 426 个键**
纳入必须覆盖集合；葡语表里 2 条运行时查不到的拼接片段登记为 `DEAD_KEYS`。此后上游新增 UI 文案而未翻译，
CI 会直接失败。当前结果：简繁各 1201 条，葡语表基准 599/599、源码基准 426/426，全绿。

## 验证

```bash
python3 tools/i18n_check.py            # 键对齐 / 重复 / 格式符 / 双基准覆盖率 / UTF-8
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
5. **Java 动态拼接的错误文案仍为英文**：`SaveTransfer.java` / `ImportStorage.java` 里
   `"Could not open " + name`、`"Save transfer failed: " + e`、`String.format(...)` 生成的整串会作为
   `job_hint` / `g_save_msg` 传给 `tr()`，但键里含运行时才有的文件名或数字，无法用固定键命中。
   这些是低频错误详情。要完全覆盖需把 Java 异常消息体系改为资源字符串（`getString(R.string.xxx, args)`），
   因 `SaveTransfer` 是刻意不依赖 Android 的纯 Java 类，本次未改动，避免破坏其可测试性。
6. **性能叠加层（perf_overlay.mm）保留英文**：属于调试用 HUD，几乎全是 `FPS %.0f`、`RAM %u MB` 这类
   格式串与技术缩写，不做翻译。
