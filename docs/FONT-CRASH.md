# 启动即闪退：ROM 字体在 release 构建下会 SIGSEGV

> 事后记录。当时用来二分的临时工作流和诊断包（cjk0 / cjk2、对照包）已经删除，结论与修法保留。

对应提交 `5f8c876`（修复）与 `a8aa6f2`（打包路径）。现象：上游官方 APK 正常，本 fork 的中文化包一打开就退，
日志停在 Vulkan 初始化那几行，没有任何 `signal 11` 记录。

## 怎么缩小到字体

两次对照实验，每次只改一个变量：

| 包 | 中文表 | 字体合并 | 结果 |
|---|---|---|---|
| 对照包（上游 main，零汉化） | 无 | 无 | 能跑 |
| zh4 | 有 | 三档中的两档，80 px 上限 | 崩 |
| cjk0 | 有 | **完全关闭** | **能跑**（中文显示为 `?`） |
| cjk2 | 有 | 只合并一个小字号（40 px） | **崩** |

cjk0 能跑直接排除了 `tr()`、语言探测和整张翻译表 —— 剩下的变量只有"字体"这一个。
同时也排除了图集大小：cjk2 的 atlas 只有 8 MB，比 zh4 的 32 MB 还小，照样崩。

## 真因：ImGui 的断言在 release 里是空的

`ImFontAtlasBuildWithStbTruetype` 对字体数据有三道 `IM_ASSERT`：

```cpp
IM_ASSERT(font_data_size > 100 && "Incorrect value for font_data_size!");
IM_ASSERT(font_offset >= 0 && "FontData is incorrect, or FontNo cannot be found.");
IM_ASSERT(0 && "stbtt_InitFont(): failed to parse FontData. ...");
```

`IM_ASSERT` 就是 `assert`，`-DNDEBUG` 下全部编译掉。于是字体解析失败不会停在断言上，而是继续用
`font_offset = -1` 去索引缓冲区。用本仓库的 ImGui 1.91.9 复现（`-O2 -DNDEBUG`）：

| 输入 | 结果 |
|---|---|
| 正常 Noto Sans CJK / Baloo 2 | 通过 |
| 裁剪过的字体（截断） | 图集建不出来，后续画字时崩 |
| `ttcf` 头 + 越界 offset 表 | **exit 139（SIGSEGV），无任何输出** |
| CFF2 可变字体（`OTTO`，无 `glyf`、无 `CFF `） | 同上 |

最后两种正是 ROM 字体的常见形态：HyperOS 的 MiSans 有可变字体版本（CFF2，stb_truetype 不支持），
有些 ROM 的 `/system/fonts/NotoSansCJK-Regular.ttc` 是被替换或裁剪过的集合，第一个 face 的 offset 越界。
这解释了三件事：为什么本地用同名字体复现不出来、为什么不打印任何日志、为什么崩在 Vulkan 初始化之后
（启动屏第一次建字体图集的那一刻）。

## 修法

1. **不再依赖 ROM 字体。** APK 自带一份子集：`assets/res/fonts/ui-cjk.otf`，Noto Sans CJK SC
   （OFL 1.1，见同目录 `OFL.txt`），292 KB，只含界面用到的码点（当前 1323 个）+ ASCII。
   Java 侧把 `assets/res` 复制到 `files/res` 并 `setenv("LARECOMP_RES_DIR", ...)`，native 从这里读。
2. **系统字体只作兜底，且必须先过 `sfnt_ok()`。** 检查 tag、表目录是否落在缓冲区内、有没有 `cmap` 和
   可用的轮廓表（`glyf` 或 `CFF `），明确拒绝 `CFF2`。不合格的字体直接跳过 —— 中文退化成方块，不会崩。
3. **版本号由 `android/lasrecomp/version.properties` 统一推导**（`0.5.11-zh.1` / `versionCode 2701`），
   见 [`docs/CI-RELEASE.md`](CI-RELEASE.md)；启动屏右下角能看到。

顺带修掉的几个问题：

- `UiCjkFont::path` 原来指向构造函数里的局部 `char[512]`，构造返回即悬垂；
- 字形范围只收 `>= U+2000`，导致表里的省略号、引号、破折号（U+2013/U+2018/U+2026）根本没有字形，
  现在放宽到 `> U+00FF`；
- 表里的 `✕`（U+2715）Noto Sans CJK SC 没有，换成 `×`（U+00D7）；
- `sourceSets.assets.srcDirs()` 是替换不是追加，`src/main/assets` 里的字体原本进不了 APK，
  现在两个目录都给，CI 里 `unzip -l` 校验它确实在包里；
- `copyAssets` 是空目录非递归的，遇到 `res/fonts/` 子目录会抛 `IOException`，把整个 `onCreate`
  的 env 设置一起带掉，改成递归。

## 防止再犯

`tools/font_coverage.py` 进 `i18n` 工作流：表里出现的字符如果既不在打包子集、也不在 Baloo 2 里，
构建直接失败。加字之后必须重建子集，命令写在脚本头注释里。

## 图集尺寸（同一份 ImGui 实测）

| 场景 | 图集 | RGBA |
|---|---|---|
| 上游（无中文） | 1024×2048 | 8 MB |
| 启动屏，合并两档（52.5、96.2，80 px 上限） | 2048×4096 | 32 MB |
| 游戏内菜单，合并四档（34/39/56/72） | 2048×4096 | 32 MB |
| 早期版本（合并 211.6 px 标题档） | 4096×32768 | 537 MB（超任何手机 GPU 上限） |

标题档不合并：它只画 `LAS Recompiled`，英文。
