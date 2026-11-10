# 启动器中文化方案（LASRecompAndroid）

> 阶段成果整理。目标：把**启动器 / 应用界面**翻译成中文，**不碰游戏本体文本**（游戏 RomFS 自带中文，由游戏内的 Language 选项控制）。
>
> 研究基线：
> - 目标仓库：`timyang2005/LASRecompAndroid`（fork 自 `mamcjr/LASRecompAndroid`，main = `82da18b`）
> - 参考实现：旧仓库 `/workspace`（fork 自 `Nycolazs/LASRecomp-Android-Plus`，main = `29873f7`）
> - 完整备份：`timyang2005/LASRecomp-Android-Plus-backup`（private，2 分支 + 18 tags）

## 1. 结论摘要

1. **不需要从零翻译**：旧仓库已经有完整的简体/繁体中文表（各 891 条）、CJK 字体合并、系统语言自动识别和「App language」手动切换。
2. **但旧仓库 UI 代码已经大幅分叉**：`runtime/ui` 两个版本相差 37 个文件、+8958/−4610 行，**不能整体搬运**，只能做「i18n 子系统移植 + 新字符串补翻」。
3. 新仓库已有一套轻量 i18n：`runtime/ui/i18n.h/.mm` + `tr()` + 葡萄牙语表（578 条）+ 多语表（94 条 × es/fr/de/it/nl），**缺中文**。
4. 文本分三层：原生启动器（ImGui）、Java 弹窗/通知资源、游戏内文本（不动）。
5. 字体是关键前置：当前启动器用 Baloo2（只含 ASCII 范围），中文必须合并 Android 系统 CJK 字体，并按中文表生成字形范围，否则全是豆腐块。

## 2. 现状盘点

### 2.1 新仓库的 i18n 机制

| 组件 | 位置 | 说明 |
|---|---|---|
| 查表接口 | `runtime/ui/i18n.h` / `i18n.mm` | `tr(const char *en)` 按英文原文查表，查不到原样返回 |
| 葡语表 | `runtime/ui/i18n_pt.inc` | 578 条，`{"English", "Português"}` |
| 多语表 | `runtime/ui/i18n_multi.inc` | 94 条，列序 en/es/fr/de/it/nl |
| 语言判定 | `runtime/settings.c` `settings_host_language()` / `settings_host_portuguese()` | 通过 `SDL_GetPreferredLocales()` 读手机系统语言；无中文分支 |
| 大小写 | `runtime/ui/i18n.mm` `ui_upper()` | 只处理 ASCII + Latin-1 重音，中文不受影响 |

注意：设置里的 `Chinese (Simplified)`（`g_language_names`）是**游戏文本语言**，与界面语言无关，两者不能混用。

### 2.2 旧仓库已有的中文实现（可直接参考/移植）

| 内容 | 位置（旧仓库） | 规模 |
|---|---|---|
| 简体表 | `runtime/ui/i18n_zh_hans.h` | 891 条 |
| 繁体表 | `runtime/ui/i18n_zh_hant.h` | 891 条（与简体键完全对齐） |
| 语言枚举 | `runtime/settings.h` | `UI_LANG_*` 共 10 个值（含 zh-Hans / zh-Hant） |
| 查表实现 | `runtime/ui/i18n.mm` | `T()`，支持 `"显示文本##ImGui ID"`；`settings_ui_language()` 自动识别 `zh` + 地区（TW/HK/MO → 繁体） |
| 字形收集 | `runtime/ui/i18n.mm` `i18n_cjk_text()` | 把所有中文译文拼成一段，供字体图集按需光栅化 |
| 字体合并 | `runtime/ui/launcher.mm` ≈1691–1714；`menu.mm` ≈372–399 | `ImFontGlyphRangesBuilder` + `MergeMode` 合并 CJK 字体 |
| 手动切换 | `runtime/ui/difficulty.mm:74` | 「App language」选项，写入 `g_settings.ui_language` 并持久化 |
| CI 构建 | `/workspace/.github/workflows/android-apk.yml` | 新仓库没有 workflows，可复用 |

### 2.3 文本分布与体量

| 层 | 位置 | 体量 | 处理方式 |
|---|---|---|---|
| 原生启动器（主界面） | `runtime/ui/launcher.mm` | 85 处 `tr()`，166 条字面量 / 158 条去重 | 走 `tr()` 查表 |
| 原生设置/菜单 | `menu.mm`、`option_rows.mm`、`mods_tab.mm` 等 | 共约 30 处 `tr()` | 同上（属于应用 UI，不是游戏文本） |
| 未走 `tr()` 的硬编码 | 各 `.mm` 中 `ImGui::Text/Button/...("字面量")` | 47 处 | 需补包 `tr()` |
| Java 资源 | `android/lasrecomp/app/src/main/res/values/strings.xml` | 14 条 | 新增 `values-zh-rCN` / `values-zh-rTW` |
| Java 硬编码 | `LASRecompActivity.java` | 123 处字符串（含日志/路径/弹窗错误） | 面向用户的搬到资源文件 |
| 清单/快捷方式 | `AndroidManifest.xml`、`res/xml/shortcuts.xml` | 少量 | 资源化后翻译 |
| 游戏本体 | RomFS | —— | **不动** |

### 2.4 翻译缺口（以旧仓库中文表为基准）

对 `runtime/ui/*.mm` 做静态统计（正则扫描 `tr(...)` 参数）：

- `tr()` 字面量 166 条，去重 158 条
- 其中 **54 条**能在旧中文表中精确命中，**104 条**未命中
- 另有 **47 处** ImGui 调用直接使用未翻译字面量

未命中的一部分是扫描把三元表达式/字符串拼接拆开造成的误报，真实需要新翻的估计在 40–60 条量级（如 `Choose driver`、`Import save ZIP`、`Copying the mod...`、`Add another version`、`Base game` 等）。逐条核对后即可得到准确清单。

## 3. 实施步骤

### P0 工作区就位

- [ ] 将 `/workspace` 切换到新 fork（当前仍是旧仓库内容，`origin` 还指向已删除的 `LASRecomp-Android-Plus`）
- [ ] 建立分支，例如 `feat/zh-launcher`
- [ ] 从备份仓库保留旧实现作为参考：`git clone /workspace/backup/LASRecomp-Android-Plus.bundle old-ref`

### P1 移植 i18n 数据层

- [ ] 复制 `i18n_zh_hans.h`、`i18n_zh_hant.h` 到新仓库 `runtime/ui/`
- [ ] 在 `runtime/ui/i18n.mm` 增加 `kZhHans` / `kZhHant` 两张表与查表分支（沿用新仓库 `tr()` 接口）
- [ ] 处理 ImGui ID 稳定性：新仓库现在直接 `tr()` 结果当标签，切换语言时 ID 变化；建议引入旧版的 `"文本##id"` 约定（或统一 `"文本###id"`）
- [ ] `ui_upper()` 已是 UTF-8 安全，中文无需改动

### P2 语言选择与持久化

两种方案，二选一或组合：

- **方案 A（最小改动）**：只做自动识别。在 `settings_host_language()` / `settings_host_portuguese()` 旁增加 `settings_host_chinese()`，`zh` + TW/HK/MO → 繁体，其余 → 简体。系统语言即可生效。
- **方案 B（推荐）**：设置里加「界面语言 / App language」选项（参考旧仓库 `difficulty.mm:74`），写入新字段 `ui_language` 并持久化到设置文件；`Auto` 时回落到系统语言。这样英文系统的手机也能手动选中文。

### P3 字体与字形（不做会全是豆腐块）

- [ ] `launcher.mm` 字体加载处（约 1750–1765 行）：保持 Baloo2 作为主字体，追加 CJK 字体合并：
  - Android 候选路径：`/system/fonts/NotoSansCJK-Regular.ttc`、`/system/fonts/DroidSansFallback.ttf` 等，取第一个存在的
  - `ImFontConfig::MergeMode = true`，字形范围由 `i18n_cjk_text()` 生成（`ImFontGlyphRangesBuilder`），避免整段 CJK 撑爆图集
  - 三档字号（30 / 52 / 92）都要合并，标题档尤其注意图集尺寸
- [ ] `menu.mm`（约 380 行）同样处理，保证游戏内菜单不混排豆腐块
- [ ] 缺字体兜底：保留英文/葡语回落；可选内置一份 OFL 授权的中文子集字体

### P4 补翻缺口

- [ ] 用脚本产出「新仓库 `tr()` 键 vs 旧中文表」的差异清单（一次性脚本，不必入库）
- [ ] 旧表已覆盖的键直接复用；新增键按术语表翻译（见第 5 节）
- [ ] 给 47 处未包 `tr()` 的字面量补上 `tr()`
- [ ] Java 侧：新增 `res/values-zh-rCN/strings.xml`（14 条），并把弹窗/导入失败等面向用户的硬编码搬到资源；原生层直接显示 Java 传来的字符串，不会被 `tr()` 二次翻译，正好各管各的
- [ ] 快捷方式、通知渠道名（`Game service` 等）一并翻译
- [ ] App 名称 `LAS Recomp` 属品牌名，建议保留

### P5 构建与验证

- [ ] 复用旧仓库 `.github/workflows/android-apk.yml` 到新仓库（新仓库当前没有 workflows）
- [ ] 本机限制：沙箱无 `ANDROID_HOME` / NDK（`build_gamelib.sh` 需要 `ndk/30.0.16248370`），只能做静态检查；APK 需在 CI 或装有 SDK/NDK 的机器上打
- [ ] 验证项：
  - `LARECOMP_HOST_LANG=zh` / `zh-TW` 启动，检查启动器全中文
  - 无豆腐块、无溢出截断（52/92 号字下的长句、按钮、标题）
  - 设置页手动切换语言后立即生效并持久化
  - Java 弹窗（导入存档、驱动错误、存档导出）为中文
  - 游戏内文本仍由游戏 Language 选项控制，未被改动

## 4. 风险与注意事项

1. **代码分叉**：旧仓库 UI 与新版差异很大（`touch_controls.mm`、`tracker_overlay.mm` 等重写），只移植 i18n 相关 hunk，不要整文件覆盖。
2. **表键必须逐字匹配**：`tr()` 按英文原文精确查表，标点、空格、大小写不同都会回落英文；移植后要跑一遍覆盖统计。
3. **ImGui ID 冲突**：直接用译文当标签可能造成同页重复 ID（例如两处 `关闭`）；引入 `##id` 约定更稳。
4. **字体图集上限**：891 条译文约一两千个独立汉字，按文本建范围通常 2048×2048 内可放下；若标题档超限，可对标题档只合并常用字或降低字号。
5. **手动语言与 Java 资源不同步**：若用方案 B（应用内手动选语言），Java 侧资源仍跟随系统语言；可在 Android 13+ 用 `LocaleManager.setApplicationLocales` 对齐，或接受弹窗语言跟随系统。
6. **别动游戏文本**：RomFS、`g_language_codes`、游戏内 Language 选项均保持原样。

## 5. 术语约定（建议）

| English | 简体 | 繁体 | 备注 |
|---|---|---|---|
| Play / PLAY | 开始游戏 | 開始遊戲 | |
| Settings | 设置 | 設定 | |
| Video / Audio | 画面 / 声音 | 畫面 / 聲音 | |
| Save Editor | 存档编辑 | 存檔編輯 | |
| Warp | 传送 | 傳送 | |
| Cheats | 金手指 | 金手指 | |
| Randomizer | 随机模式 | 隨機模式 | |
| Game files | 游戏文件 | 遊戲檔案 | |
| Driver | 驱动 | 驅動 | |
| Import / Export | 导入 / 导出 | 匯入 / 匯出 | |

## 6. 待确认

1. 只做简体，还是简繁都上？（旧表两套都有，成本几乎为零，建议都上）
2. 范围是否包含游戏内 ImGui 菜单（`menu.mm` 等，属应用 UI，不是游戏文本）？（建议包含）
3. 界面语言用「自动识别」还是「自动 + 手动切换」？（建议后者）
