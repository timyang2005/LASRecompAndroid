# LAS Recomp for Android

**简体中文** | [English](README.md) | [Português](README.pt-BR.md)

这是 peiikai 的 **[LAS Recomp](https://github.com/peiikai/LASRecomp)** 的 Android 移植版：《塞尔达传说 织梦岛》
（Nintendo Switch，2019）的静态重编译。与原项目一样，这**不是模拟器**：Switch 的代码会被编译成原生 ARM64 代码，
游戏的图形（NVN）被翻译成 Vulkan。PC 版（Windows、macOS、Linux）的功能与随机模式见
[原仓库](https://github.com/peiikai/LASRecomp)。

> [!NOTE]
> **本仓库是这个项目的汉化分支**：应用自身的启动界面与设置菜单已翻译为简体 / 繁体中文
> （**游戏内文本不受影响**，那仍由游戏自己的 Language 选项控制）。APK 从
> [本仓库的 Releases](https://github.com/timyang2005/LASRecompAndroid/releases) 下载；版本号规则为
> `<上游版本>-zh.<第几版>`（当前 `0.5.11-zh.1`，versionCode `2701`），新版本可直接覆盖安装，存档保留。
> 汉化细节见 [docs/ZH-LOCALIZATION.md](docs/ZH-LOCALIZATION.md)，构建与发版规范见
> [docs/CI-RELEASE.md](docs/CI-RELEASE.md)。
>
> 下面「0.5.x 更新」是**上游原版**的更新记录，其中的下载链接指向上游发布页；本分支的汉化版请从上面的 Releases 下载。

> [!IMPORTANT]
> 和原项目一样，这个移植版几乎完全由 AI 完成：移植代码由 Claude（Anthropic 的 Opus 5.5，通过 Claude Code）编写，
> 后续开发由 **GPT Astra** 与 **GPT Sol 6.1**（OpenAI，通过 Codex）协助，由人工指挥、测试与审查。
> 是否运行这些代码，请自行判断。

> [!WARNING]
> **实验性项目。** APK 从 [Releases](https://github.com/timyang2005/LASRecompAndroid/releases) 页面获取：
> 其余一切（你的游戏文件、游戏代码、驱动）都在手机上通过应用内引导设置。完整方案、各阶段的状态与实测数据见
> [docs/ANDROID-PORT.md](docs/ANDROID-PORT.md)（葡语）。

> [!NOTE]
> **不含任何游戏文件**，应用里也没有游戏内容。你需要自己合法获得的、已提取的《织梦岛》游戏文件
> **v1.0.0、v1.0.1 或 v1.1.0**。如何获取这些文件，恕不提供。

## 支持这个项目

如果你想支持这个移植版的持续开发，可以在 GitHub、Ko-fi、Buy Me a Coffee 或通过 **Pix** 赞助。
赞助完全自愿：每个版本对所有人都一样、都免费，没有任何赞助者专属内容。

当前目标：买一台 **POCO X7 Pro（455 美元）**——一台搭载联发科天玑 SoC 与 Mali GPU 的测试机，用来改进测试、
优化我们所有移植项目（BOTWRecompAndroid、TOTKDroid、LASRecompAndroid 与 Swap2-Native）的性能。
下方的徽章显示各平台收到的捐款总额，由我手工更新。

[![POCO X7 Pro goal](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Fmamcjr%2FBOTWRecompAndroid%2Fmain%2F.github%2Fdonation-goal.json&query=%24.raised&prefix=%24&color=2EA043&logo=xiaomi&logoColor=white&label=POCO%20X7%20Pro%20goal&suffix=%20%2F%20%24455)](https://github.com/mamcjr/BOTWRecompAndroid/blob/main/.github/donation-goal.json)

[![Sponsor on GitHub](https://img.shields.io/badge/Sponsor-on%20GitHub-ea4aaa?logo=githubsponsors&logoColor=white)](https://github.com/sponsors/mamcjr)
[![Ko-fi](https://img.shields.io/badge/Ko--fi-mamcjr-FF5E5B?logo=kofi&logoColor=white)](https://ko-fi.com/mamcjr)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-mamcjr-FFDD00?logo=buymeacoffee&logoColor=black)](https://buymeacoffee.com/mamcjr)
[![Pix](https://img.shields.io/badge/Pix-Random%20Key-32BCAD?logo=pix&logoColor=white)](#支持这个项目)

> **Pix（仅巴西——随机密钥）：** `6c326b56-6caf-4c3a-bcf7-1fa8ad2abd16`

## 0.5.11 更新

修复 Mali GPU 上的问题（光照、纹理），以及关闭应用时的崩溃。

[下载 v0.5.11 预览版 APK](https://github.com/mamcjr/LASRecompAndroid/releases/tag/v0.5.11)。

- **Mali 上房屋与迷宫里的光照闪烁**（#2）：光照读取的内存范围列表只留了 16 个位置，而一个迷宫需要 20–33 个，
  它们把彼此挤了出去。现在留 48 个，且再次漏掉的范围会被移到最前面。
- **Mali 上会写入游戏内存的绘制**：把两段内存范围合并会让绘制过程在绘制中途结束。现在在过程开始前就合并。
- **Mali 无法平滑过滤的纹理**：改为不过滤采样（否则行为未定义：在 BOTWRecompAndroid 的一台 Mali-G68 上草地变黑）。
  也可能是花朵黑边的成因（#14）。
- **关闭应用时的崩溃**（#2、#14）：关闭时 GPU 驱动自行清理，而应用还在绘制（
  “FORTIFY: pthread_mutex_lock called on a destroyed mutex”，或崩在 Mali 驱动里）。现在应用结束时不走这条清理。

验证：在 POCO F7 Ultra 上，从最近任务关闭应用在 0.5.10 上 3 次里崩 2 次，现在 6 次里 0 次；
模拟 Mali 路径时，光照路线在相同画面与帧率下无报错跑通。此处未在真 Mali 上测试。

## 0.5.10 更新

修复 Mali 上的一个崩溃：游戏会自己关闭，最常出现在切换区域时。

[下载 v0.5.10 预览版 APK](https://github.com/mamcjr/LASRecompAndroid/releases/tag/v0.5.10)。

- **游戏中的崩溃**：销毁纹理时（常见于切换区域），渲染线程可能同时在用 GPU 驱动，导致 Mali-G57 的驱动崩溃（#2）。
  现在渲染线程会等到纹理销毁完成。

验证：在 POCO F7 Ultra 上，开启原生游戏代码、模拟 Mali 路径跑 issue #2 的路线，四次区域切换无崩溃。此处未在真 Mali 上测试。

## 0.5.9 更新

修复 PowerVR 过曝的画面与中文语言；Mali 加载区域后的 CPU 复制更少。

[下载 v0.5.9 预览版 APK](https://github.com/mamcjr/LASRecompAndroid/releases/tag/v0.5.9)。

- **PowerVR**：画面不再一片惨白——驱动读不了的一种顶点格式（模型的法线）改由着色器解包（#8）。
- **中文**：简体与繁体中文会显示游戏的中文文本，而不是日文（#12）。
- **Mali、Xclipse 与高通自带驱动**：加载区域后，绘制不再退回到整块 CPU 复制，空闲的上传内存会被归还
  （来自 BOTWRecompAndroid）；Mali 驱动可压缩的渲染目标。
- **修复**：从纹理读出的 8 位数值改为四舍五入而非截断；从后台返回时丢失的窗口表面会重建；打不开的影片不再让游戏崩溃。

验证：在 POCO F7 Ultra 上复现并修复了 PowerVR 的问题、检查了中文文本、模拟 Mali 路径跑了 issue #2 的路线。
此处未在真 PowerVR 或 Mali 上测试。

## 0.5.8 更新

游戏可以既不编译自己的代码、也不复制自己的数据就运行；另修复 issue #2、#7、#10。

[下载 v0.5.8 预览版 APK](https://github.com/mamcjr/LASRecompAndroid/releases/tag/v0.5.8)。

- **原生游戏代码（实验性）**：游戏文件文件夹上方的 **Game code: Native** 让游戏自己的 ARM64 代码直接在手机 CPU 上运行，
  初始化过程什么也不用编译。已安装的游戏可在 Video > Native game code 切换。默认关闭。来自 BOTWRecompAndroid。
- **就地使用游戏文件**：**Game files: Use them in place** 直接从你的文件夹（其中的 `romfs/` 或 `romfs.bin`）读取游戏数据，
  不再复制约 6 GB。Android 会请求「访问所有文件」权限；文件夹必须留在原处。
- **现成的 romfs.bin**：`exefs/` 旁边有 `romfs.bin` 也能被初始化接受：一个文件代替约 3,800 个。
- **修复**：有声音播放时返回启动界面会在 Turnip 上崩溃；0.5.7 的写跟踪崩溃与 Mali 内存一致性（#2）；
  最多 16 张交换链图像（#10）；更轻量的游戏代码编译（#7）；只有约 7 GB 以上内存才把游戏内存放在 GPU 上。

验证：在 POCO F7 Ultra 上，原生代码在村庄跑 30 与 60 fps、重启回启动界面、模拟 Mali 路径；
游戏文件分别从 `romfs/` 文件夹和 `romfs.bin` 就地使用。此处未在真 Mali 上测试。

## 0.5.7 更新

在驱动无法把游戏内存映射到 GPU 的手机上（Mali、三星 Xclipse、高通自带驱动）减少 CPU 工作量。

[下载 v0.5.7 预览版 APK](https://github.com/mamcjr/LASRecompAndroid/releases/tag/v0.5.7)。

- **顶点数据只复制改动的部分**：在这些驱动上，每一帧都会复制全部顶点与索引缓冲（Mali-G57 上每 2 秒 250–490 MB）。
  现在应用会检测游戏写入了游戏内存的哪些页，只复制这些页。来自 BOTWRecompAndroid。
- **应用版本号**显示在启动界面右下角。

验证：在 POCO F7 Ultra 上模拟 Mali 风格的驱动路径时，野外每 2 秒的 CPU 复制从 769 MB 降到 123 MB，画面不变。
此处未在真 Mali 上测试。

从 v0.5.6 升级不会重新编译游戏代码，也不改变存档。

## 0.5.6 更新

来自原版 LAS Recomp 的修复、两处引擎修复，以及受损游戏文件的检查。

- **带重音的名字**：像 "Sebastiã" 这样的名字会在字母中间被截断，导致文件选择界面崩溃；现在按完整字母截断
  （8 个字符，与主机一致）。
- **受损或被改动的游戏文件**：初始化会按名称与大小检查每个游戏文件；缺失、改名或被改动时，启动界面会提示
  （游戏可能因此崩溃），日志里会列出这些文件。
- **存档编辑器**：Give 会给你选中的那个道具（以前在 Mermaid's Scale 上选它会给出另一个）。
- **手机上编译游戏代码更快**（1.0.1 在 POCO F7 Ultra 上：284 秒，此前 416 秒），使用本地内存的着色器在 GPU 上更轻。
  升级会为每个版本重新编译一次游戏代码。

验证：在 POCO F7 Ultra 上，新建名为 "Sebastiã" 的存档、用存档编辑器给予 100 卢比、启动界面的受损文件警告，
以及 1.0.1 游戏代码 284 秒编译完成。此处未测试导入真正受损的副本（检查本身是用真实的 1.0.0 文件验证的）。

## 0.5.5 更新

修复 0.5.4 上反馈的问题：钓鱼小游戏、进入区域时 Auto 掉到 30 fps、动态分辨率与自定义字体。

- **钓鱼小游戏**：水下视角会显示另一个房间、开始前的池塘，或者什么都没有；鱼回来了，30 与 60 fps、任意宽高比都正常。
- **Auto 帧率**：为新区域准备着色器花掉的帧，不再让它掉到 30 fps 几秒钟。
- **动态分辨率**改为参照 GPU 自身耗时，着色器工作不再拉低分辨率。
- **自定义字体**：应用会打开自己的文件选择器，可以在其中选择字体文件。
- **Turnip LRZ** 在 Adreno 7xx 上也默认开启；模糊的侧边栏更轻（纹理读取量降到三分之一）；日志会显示每帧的 GPU 耗时。

验证：在 POCO F7 Ultra 上，钓鱼小游戏（0.5.4 水下全黑，0.5.5 鱼回来了）、一局里只掉一次 30 而不是两次、
动态分辨率跨新区域保持稳定、导入了一款字体。此处未测试 Adreno 7xx 上的 LRZ 与 Mali 上的侧边栏。

## 0.5.4 更新

合并了 Nycolazs 的 [LASRecomp-Android-Plus](https://github.com/Nycolazs/LASRecomp-Android-Plus) 的更多内容
（经作者同意），并修复启动时的黑屏。

- **启动黑屏**（0.5.3，X 上有反馈）：窗口打开失败会让应用干等最多 10 分钟且不说原因；现在 5 秒后就报错。
- **驱动错误**：Vulkan 驱动启动失败时，弹窗提供「安装另一个驱动」或「回到手机自带驱动」。
- **触屏操作**：可选用游戏自己的道具图标做按键（设置 > 控制 > 带道具图标的按键）；固定摇杆；
  只显示游戏此刻用得到的按键（默认开启）。
- **手柄**：正在使用的那只手柄就是游戏读取的那只；按字母识别面键，适用于 Xbox 风格手柄与掌机。
- **视频**：1.25x 与 1.5x 内部分辨率；较窄画面两侧的边栏显示其边缘的模糊扩展（或纯黑：视频 > 侧边栏）。
- **性能**：Turnip 上压缩渲染目标（UBWC）、更少的颜色复制、小光晕按自身尺寸渲染、手机编译游戏代码快约 6%。
- **西班牙语、法语、德语、意大利语、荷兰语菜单**，手机设为对应语言时生效。
- 你在主屏幕时游戏保持打开，Android 结束应用时设置不再丢失。

验证：在 POCO F7 Ultra 上，Turnip 下村庄 60 fps、高通驱动 30 fps，手机上编译 1.0.1 游戏代码，
按键显示道具图标、驱动弹窗。此处未测试手柄、新语言与其他 GPU。

## 0.5.3 更新

支持更多低于 Vulkan 1.3 的驱动，例如许多三星手机上的原厂 Mali 驱动。

- **又有五个扩展改为可选**（[issue #2](https://github.com/mamcjr/LASRecompAndroid/issues/2)，Galaxy A15 / Mali-G57）：
  没有 `synchronization2` 时用 Vulkan 1.0 的屏障；没有扩展动态状态时，绘制状态被固化进各自的管线
  （管线更多：那里的首次卡顿更多）；低于 Vulkan 1.3 时不需要 demote / terminate。

验证：在 POCO F7 Ultra 上强制开启每种回退，Turnip（60 fps）与作为 Vulkan 1.1 的高通驱动（30 fps）下村庄都绘制正确。
尚未在真 Mali 上测试。

## 0.5.2 更新

支持没有动态渲染的驱动，例如许多三星手机上的原厂 Mali 驱动。

- **没有 `VK_KHR_dynamic_rendering` 的驱动**（[issue #2](https://github.com/mamcjr/LASRecompAndroid/issues/2)，
  Galaxy A15 / Mali-G57）：应用不再卡在启动；改用经典渲染通道绘制（来自 BOTWRecompAndroid，它与本项目共用这个渲染器）。
- 启动信息会列出每一个缺失的扩展，而不只是第一个。

验证：在 POCO F7 Ultra 上强制开启渲染通道，Turnip（60 fps）与作为 Vulkan 1.1 的高通驱动（30 fps）下
村庄、帧生成、SMAA 与菜单都绘制正确。尚未在真 Mali 上测试。

## 0.5.1 更新

Adreno 610 等不支持 16 位存储的 GPU 上的 Turnip。

- **不支持 16 位存储的 GPU**（[issue #4](https://github.com/mamcjr/LASRecompAndroid/issues/4)，Adreno 610 + Turnip）：
  应用不再因为没有任何着色器用到的 `storageBuffer16BitAccess` 卡在启动。
- **在 Adreno 750 上确认**（[issue #3](https://github.com/mamcjr/LASRecompAndroid/issues/3)）：开启 Turnip 与
  “Game memory on the GPU (Turnip)” 后，文字与颜色绘制正确。

验证：在 POCO F7 Ultra 上强制关闭 16 位存储，Turnip（60 fps）与高通驱动（30 fps）下村庄绘制正确。
尚未在真 Adreno 610 上测试。

## 0.5.0 更新

游戏版本 1.0.1 与 1.1.0、多个版本并存、着色器预编译，以及上游 LAS Recomp 下个 beta 的修复。

- **游戏版本 1.0.1 与 1.1.0**，从游戏自己的文件识别。
- **多个版本并存**：设置 > 存档与文件 > **Add another version**；启动界面的 **PLAY ‹ 版本 ›** 在它们之间切换。
  存档共享；每个版本各自编译一次游戏代码。安装其他版本时基础版本 1.0.0 保留。
- **着色器预编译**（来自上游 beta 5）在启动界面进行：某区域第一次出现时的卡顿更少。
- **来自上游 beta 5 的修复**：全有或全无的存档、Dampe 的 amiibo 选项、物理崩溃与卡死、加宽视野下的室内光照接缝、
  更宽的宽高比、钓鱼小游戏的池塘。
- **Adreno 7xx 上的 Turnip**（128 MB 着色器内存范围，[issue #3](https://github.com/mamcjr/LASRecompAndroid/issues/3)）：
  开启 “Game memory on the GPU (Turnip)” 后，着色器通过指针访问该范围之上的游戏内存，文字与颜色绘制正确。
  自动模式在那里仍用复制；想试就把它设为 On。
- Auto 的 30 fps 是稳定的 30；Vulkan 1.1 / 1.2 驱动能正确读取扩展特性；驱动包从 `meta.json` 加载自己的库；
  锁屏状态下打开会等待而不是停下。

更新后，游戏代码会重新编译一次。

验证：在 POCO F7 Ultra（Adreno 830，Turnip）上，1.0.0、1.0.1、1.1.0 同时安装，用箭头切换，三个都进到村庄；
存档未变。尚未在真 Vulkan 1.1 手机或非 8xx 的 Adreno 上测试。

## 0.4.1 更新

针对 Turnip 下画面异常的 GPU 提供驱动兼容选项，以及用于随 bug 报告提交的日志文件。

- **驱动兼容**（启动界面：设置 > 存档与文件），两项默认都是 **Automatic**：在 Adreno 8xx 上用最快的路径
  （它们就是在这里开发和测试的），其他 GPU 上用更稳妥的路径。每项也可以强制开或关。
  - **Game memory on the GPU (Turnip)：** 把游戏内存直接映射进 GPU 的内存，或每帧复制
    （花 CPU 时间，但在快路径不行的设备上绘制正确）。
  - **Turnip low-resolution Z (LRZ)：** 关闭时在驱动加载前设置 `TU_DEBUG=nolrz`，与
    [LASRecomp-Android-Plus](https://github.com/Nycolazs/LASRecomp-Android-Plus) 的做法一致。重启后生效。
- **保存日志文件**：用 Android 文件夹选择器选定文件夹后写入 `lasrecomp-log.txt`
  （文件管理器进不了应用自己的目录）；崩溃后，上一次运行会保留为 `lasrecomp-log-previous.txt`。
  日志现在还会列出 GPU 的存储缓冲范围与内存类型。

验证：在 POCO F7 Ultra（Adreno 830，Turnip）上，Automatic 保留两条快路径（村庄 60 fps，与 0.4.0 相同）；
两项都关闭时，村庄通过复制绘制正确（稳定 30 fps；60 fps 下 48–60）。在手机上选定、写入并轮转了日志文件夹。
尚未在非 8xx 的 Adreno 上测试——那正是 Automatic 的目标场景
（[issue #3](https://github.com/mamcjr/LASRecompAndroid/issues/3)，一台 Adreno 750）。

## 0.4.0 更新

帧生成（30 → 60 与 60 → 120）、游戏 Mod、葡语菜单与新的放大算法。

- **帧生成**（视频 > 帧生成，默认关闭）：从画面、场景深度与相机运动生成中间帧，改编自
  [LASRecomp-Android-Plus](https://github.com/Nycolazs/LASRecomp-Android-Plus)。游戏的 30 fps 变成 60，
  开启 120 Hz 选项后它的 60 变成 120（游戏本身仍按自己的 60 Hz 时钟运行）。HUD 保持不动，林克不会抖；
  GPU 跟不上时它会自动暂停，稍后再试。在 POCO F7 Ultra + Turnip 上，村庄 60 → 120 时每个真实帧约多花 0.4 ms GPU，
  且全程保持开启。使用高通原厂驱动时请用 30 fps。
- **120 Hz 选项**（在帧生成之下，默认关闭）：开启时请求显示器最快模式，游戏的 60 fps 变成 120；关闭时请求 60 Hz。
  若手机系统设置把屏幕固定在 120 Hz，中间帧跟随屏幕。
- **物体自身的运动**（实验性）：自己移动的角色在中间帧里也会移动。
- **Mod**（设置 > Mod）：内含 `romfs` 文件夹的目录，与 Eden、Ryujinx 的 Mod 相同，可逐个开关；
  可用 Android 文件夹选择器导入 Mod 目录。应用不自带任何 Mod。
- **葡语菜单**：手机设为葡萄牙语时，设置、启动界面、存档编辑器与提示均为葡语。
- **Snapdragon Game Super Resolution 1 放大**（高通）：单趟处理，比 FSR 1 更轻。
- **性能叠加层**：显示含帧生成的帧率（FPS+FG）、紧凑的单行样式（FPS、FPS+FG、内存、电池温度），可拖到屏幕任意位置。
- **手柄操作菜单**：第一次按键不再丢失，左右改变数值，A 或 L / R 进入页面，B 关闭。
- **Vulkan 1.1 / 1.2 驱动**：扩展集与 1.3 匹配的现在也被接受（例如原厂 Mali 驱动）；缺哪个扩展，提示里会指明。
- **32 位着色器数学**（视频）：用于光照、草地或文字绘制异常的驱动。
- **初始化占用更少空间**：`romfs.bin` 直接从选定文件夹构建。所需可用空间从约 14 GiB 降到约 8 GiB，
  已安装的用户会自动回收约 5.7 GiB。
- **初始化的编译步骤**：游玩配置之外的游戏函数（LASRecomp-Android-Plus 的列表）不做优化编译。
- **固定 30 fps 下的动态分辨率**按 30 的预算而不是 60 的预算调整。
- 菜单与触屏操作采用《织梦岛》主题。Android 上没有随机模式。

验证：在 POCO F7 Ultra 上，Turnip 与高通驱动下帧生成 30 → 60、60 → 120、30 → 120；Mod（应用、关闭、用选择器导入）；
菜单翻译；模拟手柄按键操作菜单；拖动叠加层；SGSR 1；从选定文件夹全新初始化游戏
（约 7 分 40 秒：`romfs.bin` 约 45 秒，然后是编译），之后开着 Mod 进入村庄。尚未在手机上测试：
Vulkan 1.1 驱动（Mali）与 Adreno 750。

## 0.3.9 更新

重点是兼容更多手机与驱动。

- **没有 `shaderInt64` 的原厂驱动**（例如 Galaxy S23、Adreno 740）：驱动没有 64 位整数时，着色器的地址与进位运算
  改用 32 位字。S23 上用高通自带驱动时，光照、草丛与道具栏文字恢复正常。具备该特性的驱动（如 Turnip）仍走原生路径。
  [诊断记录](docs/ANDROID-S23-STOCK-RENDERING.md)。
- **更稳健的 Vulkan 启动**：交换链只请求表面支持的用途与合成方式；管线缓存使用前先校验；可选特性先查询再启用。
  出错时错误弹窗会在屏幕上给出原因，不需要 adb。
- **导入**：复制前检查可用空间、能识别不完整的复制、拒绝应用自己已导入的目录。
- **固定 30 fps 在 Turnip 上真正保持 30**。游戏根据每帧实际消耗多少个垂直同步来选模式，而有了渲染线程，
  Turnip 的帧落在一个之内，于是它退回 60。该选项现在开启游戏自己的 30 fps 模式，且呈现等待两个垂直同步，与 Switch 一致。
  在 POCO F7 Ultra 上：Turnip 与高通驱动下都是稳定 30.0 fps，切回 Auto 即恢复正常。Auto 模式不变。
- **全新安装**默认固定 30 fps 与 50% 内部分辨率。已有的设置仍然生效。
- **Bug 报告表单**会询问失败发生在哪个阶段；错误弹窗的截图已经很有帮助。
- **测量工具**（村庄基准路线、设置矩阵、GPU 计时与纹理追踪），全部默认关闭。

验证：在 Galaxy S23 + 原厂驱动上，两次加载中目视检查了光照、草地与文字，并用上一版 APK 作对照。
在 PC 上跑了 32 位着色器翻译、着色器缓存、常量表与 Vulkan 启动的测试。Mali GPU 仍需真机测试。
[兼容性说明](docs/ANDROID-COMPATIBILITY.md)。

## 0.3.8 更新

0.3.8 APK 更新：启动界面只显示 **PLAY**，没有 Vanilla/种子选择器。新背景沿用游戏那种玩具模型村庄的风格，
带一个 Android 小冒险者。

![Android adventurer icon](docs/icon/android-hero.png)

![Android island launcher background](docs/artwork/android-island.png)

- **触屏设置**：更大的侧栏分类、行内说明、更大的控件与下拉选项、滑动滚动与常驻的返回按钮。
  视频、音频、控制可直接进入；存档与文件在启动前就能用，更多选项里是其余页面。
- **渲染工作**：管线配置复用、可复用的纹理转换缓冲、稳定动态纹理的精确比较、仅加载用的 arena 复制复用、
  计算着色器翻译排队。着色器编译线程数可配置。
- **稳定的 ADPF 默认值**：保留原有的墙钟时间性能信号，并同步会话管理。用户 50% 分辨率的 Turnip 对比中，
  恢复该信号后采集区间从约 52–58 FPS 提升到 58.9–59.9 FPS。偶尔的长帧仍会出现；这不是受控基准，也不保证其他设备。
- **更快的更新**：内置游戏代码构建器只在其内容指纹变化时才解包。
- **新美术**：一个 Android 冒险者图标与原创岛屿主题启动背景，用 OpenAI 图像生成。它们是同人插画，不是游戏资源。

验证：PC 上的 release 构建、22 项上传缓存检查、12 项模拟触摸手势检查与 28 项存档迁移检查；
用户在连接的 Turnip 设备上测了帧时间。高通/Mali 的复制路径行为与所有菜单流程仍需更广泛的真机测试。
细节：[优化](docs/ANDROID-OPTIMIZATIONS.md)、[触屏设置](docs/ANDROID-TOUCH-SETTINGS.md)。

<!-- stationary-matrix-s23:start -->
## 实测性能 —— Galaxy S23

2026-10-21 · SM-S911B，Adreno 740，Android 16 · **Auto 帧率**。静态玛贝村场景，同一位置/存档，
一次 Turnip 游戏运行跑完 40 种组合。每种配置静置 10 秒、采集 30 秒。数值为 Android 测得的平均呈现 FPS。
Auto 让游戏自己选 30/60 FPS，不会解锁 120 FPS。

**测试条件：** 本地诊断版 APK，Turnip v26.0.0 R8，ADPF 未改动，并给 Turnip 设置 `LARECOMP_VK_NO_PLACED=1`。
没有这个临时内存变通，Android 会因内存不足杀掉 Turnip（这台 S23 上，关闭其他应用后也一样）。
这些结果不代表 0.3.8 版本里已经发布的修复。

<details>
<summary>Turnip FPS —— 40 个样本</summary>

| 分辨率 | 宽高比 | 滤镜 | AA | Turnip R8 |
| --- | --- | --- | --- | ---: |
| 50% | 16:9 | 双线性 | 关 | 31.28 |
| 50% | 16:9 | 双线性 | SMAA | 30.37 |
| 50% | 16:9 | FSR1 | 关 | 31.14 |
| 50% | 16:9 | FSR1 | SMAA | 32.28 |
| 50% | 解锁 | 双线性 | 关 | 29.92 |
| 50% | 解锁 | 双线性 | SMAA | 29.81 |
| 50% | 解锁 | FSR1 | 关 | 29.30 |
| 50% | 解锁 | FSR1 | SMAA | 27.24 |
| 75% | 16:9 | 双线性 | 关 | 29.97 |
| 75% | 16:9 | 双线性 | SMAA | 30.00 |
| 75% | 16:9 | FSR1 | 关 | 29.91 |
| 75% | 16:9 | FSR1 | SMAA | 29.68 |
| 75% | 解锁 | 双线性 | 关 | 26.32 |
| 75% | 解锁 | 双线性 | SMAA | 24.26 |
| 75% | 解锁 | FSR1 | 关 | 23.27 |
| 75% | 解锁 | FSR1 | SMAA | 22.61 |
| 1x | 16:9 | 双线性 | 关 | 25.21 |
| 1x | 16:9 | 双线性 | SMAA | 23.75 |
| 1x | 16:9 | FSR1 | 关 | 25.31 |
| 1x | 16:9 | FSR1 | SMAA | 23.33 |
| 1x | 解锁 | 双线性 | 关 | 20.59 |
| 1x | 解锁 | 双线性 | SMAA | 20.05 |
| 1x | 解锁 | FSR1 | 关 | 20.16 |
| 1x | 解锁 | FSR1 | SMAA | 21.50 |
| 动态 | 16:9 | 双线性 | 关 | 29.93 |
| 动态 | 16:9 | 双线性 | SMAA | 29.97 |
| 动态 | 16:9 | FSR1 | 关 | 30.00 |
| 动态 | 16:9 | FSR1 | SMAA | 29.97 |
| 动态 | 解锁 | 双线性 | 关 | 29.96 |
| 动态 | 解锁 | 双线性 | SMAA | 29.96 |
| 动态 | 解锁 | FSR1 | 关 | 29.55 |
| 动态 | 解锁 | FSR1 | SMAA | 28.30 |
| 匹配屏幕 | 16:9 | 双线性 | 关 | 24.96 |
| 匹配屏幕 | 16:9 | 双线性 | SMAA | 23.74 |
| 匹配屏幕 | 16:9 | FSR1 | 关 | 26.95 |
| 匹配屏幕 | 16:9 | FSR1 | SMAA | 24.77 |
| 匹配屏幕 | 解锁 | 双线性 | 关 | 21.05 |
| 匹配屏幕 | 解锁 | 双线性 | SMAA | 20.68 |
| 匹配屏幕 | 解锁 | FSR1 | 关 | 20.46 |
| 匹配屏幕 | 解锁 | FSR1 | SMAA | 20.25 |

</details>

未启用 FSR1 即双线性；AA 开启即 SMAA。「动态」会在采样期间改变分辨率。这是每种组合一个样本的配置扫描，
不能说明移动场景下的稳定性，也不能说明某个设置带来了因果关系上的改善。顺序、温度与缓存都会影响结果。
[协议与遥测](docs/ANDROID-STATIONARY-MATRIX.md) · [FPS 数据](docs/benchmarks/galaxy-s23-20261021.json)。

原厂驱动：对比因渲染不完整而中止（反馈有光照、草地与文字问题）。其部分 FPS 结果不列入此表。
原因（缺少 `shaderInt64`）已在 0.3.9 修复；原厂驱动的测量尚未重做。POCO F7 Ultra 的测试在排查兼容性期间暂缓。
<!-- stationary-matrix-s23:end -->

## 导入与导出存档

在启动界面打开 **设置 → 存档与文件 → 存档 - Eden / Yuzu / Ryujinx**：

- **导入存档文件夹**：选择包含 `savedata0.bin`（或其他 `savedataN.bin`）以及游戏其他 `.bin` 文件的
  《织梦岛》存档文件夹。
- **导入存档 ZIP**：选择该文件夹的 ZIP。外层多包一层文件夹也能识别；包含多个存档配置的压缩包会被拒绝，以免进度混淆。
- **导出存档**：选择 ZIP 的保存位置。在模拟器中关闭游戏后，把它的内容解压到 Eden、Yuzu 或 Ryujinx 打开的
  《织梦岛》存档文件夹。
- **导出上一次的存档**：导出上次导入之前的备份。导入那个 ZIP 即可恢复。

导入会替换所选存档集里的所有槽位，并保留一份原存档的副本。游戏启动前也可以迁移。
如果 Android 不允许选择其他应用的文件夹，先把存档从模拟器导出到可访问的目录（如「文档」），再在这里选择那份副本或 ZIP。
原始 `.bin` 文件不经转换直接传输；随机模式的附属文件不会放进 ZIP。

## 截图

![启动界面](docs/android/start_screen.jpg)

| ![大地图，58 fps](docs/android/overworld.jpg) | ![托隆波海岸沙滩，60 fps](docs/android/beach.jpg) |
|:-:|:-:|
| ![找到剑](docs/android/sword.jpg) | ![托隆波海岸](docs/android/shore.jpg) |

POCO F7 Ultra + Turnip：屏幕上是触屏按键，左上角是性能叠加层。

![玛贝村 75%（1080p），全屏，60 fps](docs/android/1080p_fullscreen.jpg)

75% 内部分辨率（1080p）全屏：55–60 fps，多数时间 60。

![玛贝村 75%（1080p），16:9，60 fps](docs/android/1080p_16x9.jpg)

75%（1080p）、16:9、Switch 的形状（两侧有边栏）：稳定 60 fps。

![玛贝村原生分辨率，全屏，30 fps](docs/android/native_fullscreen.jpg)

原生分辨率（3200×1440，100%）全屏：稳定 30 fps，走游戏自己的 30 fps 模式（满速）。

## 当前状态

在 **POCO F7 Ultra**（骁龙 8 Elite / Adreno 830，Android 16）上配 **Turnip Gen8 V37** 驱动测试：

- 游戏能启动，影片有声播放，菜单、存档与游玩都正常；
- **稳定 60 fps**（包括大地图），默认 50% 内部分辨率（720p）：我们的野外测量里 98% 的时间是 60，
  没有超过 18 ms 的帧。75%（1080p）时在 16:9 下保持 60，全屏在我们的测试里是 55–60 fps；
  原生分辨率（3200×1440）全屏是稳定 30。游戏仍有自己的 30 fps 模式，与 Switch 一样，供某帧赶不上时使用；
- **触屏操作**：浮动摇杆，A / B / X / Y，L / R / ZL / ZR，Minus / Plus 与一个菜单键，在没有连接手柄时显示
  （一旦接上手柄就隐藏）。布局可在游戏内菜单的「控制」页编辑（拖到喜欢的位置、调整大小与不透明度）；
  Android 返回键打开菜单；
- 蓝牙手柄（用 GameSir X4 Aileron 测试过），可在「控制」页重新映射；
- 帧率设置（「视频」页）：Auto（游戏自己选 30/60）、30 fps 或 60 fps；
- 性能叠加层（fps、帧时间、CPU 簇、ADPF 会话状态与各线程耗时/核心、GPU、内存、温度、电池），
  并且应用从后台返回后没有黑屏。游戏内菜单不含桌面版的选项（全屏、4K / 8K、随机模式）。

### 为什么它这么快

游戏的着色器在运行时从 Switch GPU 的代码翻译成 GLSL。在 Adreno 上，翻译后的着色器曾是瓶颈，因此 Android 上的翻译器现在：

- 在翻译时解析控制流栈，而不是在每个着色器里把它们保留为数组；
- 把分支、循环与跳转表（从着色器自己的常量里读出）变成结构化的 `if` / `for` / `switch` 代码，
  而不是 `switch (pc)` 状态机——后者在移动端编译器手里表现很差（野外的光照着色器从 24 个寄存器降到 16–19 个，
  指令数从 2884 降到 1717）；
- 在片段与计算着色器里用 GPU 自己的指令做半精度转换。

CPU 侧：

- **渲染线程**：游戏的命令列表在独立线程上翻译成 Vulkan，同时游戏继续准备下一帧，并按块交给 GPU，
  好让 GPU 在整帧翻译完之前就开始。游戏线程每帧的工作量从约 11.4 ms 降到约 7.4 ms；
- **模拟 Switch 的 0 号核心**：游戏把一些作业线程和主线程放在同一个核心上，在那里轮流执行；
  若真正并行，更快的主线程会暴露它作业系统里的竞态（崩溃）。现在这些线程在这里也轮流执行，
  游戏的其他核心保持并行；
- **帧节奏**：区分游戏自己的 30 fps 帧与迟到的 60 fps 帧；Android 的性能提示（ADPF）让两个线程共用
  一个以 60 fps 为目标的统一会话；并修复模拟内核里三处线程同步竞态；
- 运行时以优化方式构建，游戏的 GPU 栅栏不再等待合成器，每个绘制的准备工作有缓存。
- 在使用复制路径的驱动上，顶点与索引数据只从游戏改动过的页复制（v0.5.7）。这降低了模拟 Mali 路径上的 CPU 复制量；
  真机 Mali、Xclipse 或高通设备上的 FPS 提升尚未测得。

## Turnip 还是手机自带驱动

游戏在 Mesa 的 **Turnip** 驱动和手机自带的**高通**驱动上都能运行，**包括光照**。
**60 fps** 仍然要用 Turnip：

| 驱动 | 光照 | 大地图（720p） |
|---|---|---|
| Turnip | 有 | 60 fps（98% 的时间） |
| 高通自带 | 有 | 30 fps |

![玛贝村跑在手机自带的高通驱动上，光照正常，30 fps](docs/android/qualcomm_driver.jpg)

在手机自带的高通驱动上，没有 Turnip（叠加层的 GPU 一行显示 "Qualcomm"）：光照在。

游戏的着色器直接读写游戏内存，用的地址与 CPU 相同：例如每帧的光照列表由一个计算着色器生成，
再被像素着色器读取；还有些着色器通过指针访问数据。在 PC 上，游戏内存通过 `VK_EXT_external_memory_host`
导入 GPU，而这两个驱动都没有这个扩展。

- **Turnip** 有 `VK_EXT_map_memory_placed`：GPU 内存在游戏堆区的确切地址上分配，CPU 与 GPU 共享同一份内存，无需复制。
- **其他驱动**（高通的，以及原则上 Mali、Xclipse 等）走一个 **GPU arena**：着色器写入的内存区域放在 GPU 上，
  因此读取它们的绘制在同一帧里就看到结果；CPU 的改动会被发现并由 GPU 复制进去，GPU 的结果完成后写回游戏内存。
  着色器通过指针访问的区域在运行时发现：着色器记录未命中的访问，这些区域随后被加入。代价比 Turnip 的共享内存高，
  所以目前是 30 fps。

## 目前限制

- 在骁龙（Adreno）上测试过：Turnip 下 60 fps，手机自带驱动下 30 fps（两者都有光照）。应用用
  [libadrenotools](https://github.com/bylaws/libadrenotools) 加载 Turnip；驱动不随包提供，需要你自己准备。
  **Mali**：游戏在一台 Mali-G615 上以 20–30 fps 运行；自 0.3.3 起纹理在 CPU 上解码
  （Mali 不支持 BC 纹理，也就是 Switch 的格式），解决了那里的彩色色块——仍需更多 Mali 手机确认。
  Xclipse 等预计走同一条路径：欢迎反馈。
- 你需要自己提取的 v1.0.0、v1.0.1 或 v1.1.0 游戏文件。初始化会检查所需额外可用空间
  （测试用 dump 约 **8 GiB**；留 12 GB 更宽裕）：它复制 ExeFS，并从选定文件夹直接构建 RomFS 数据镜像。
  初始化成功后，如果你有备份，可以删掉手机上原始的 dump。
- 首次初始化会在手机上编译游戏代码。v0.4.0 时在 POCO F7 Ultra 上完整初始化约 7 分 40 秒；
  v0.5.6 只在同一台手机上编译 v1.0.1 的游戏代码用了 284 秒。耗时会因手机与游戏版本而异。
- 游戏运行中改变分辨率可能导致应用关闭。请在标题画面修改。
- 手机上还没有随机模式。

## 安装

你需要一台 Android 11 或更新版本的 arm64 手机（在骁龙上测试过）、自己提取的游戏文件
（v1.0.0、v1.0.1 或 v1.1.0：`exefs` 与 `romfs` 两个目录）复制到手机上的某个位置，
以及在 Adreno GPU 上要跑 60 fps 的话，一份 Turnip 驱动包（adrenotools 的 `.zip`，其他模拟器也用它）。

1. 从 [Releases](https://github.com/timyang2005/LASRecompAndroid/releases) 页面安装 APK。
2. 打开应用，点 **游戏文件文件夹**，选择包含 `exefs` 与 `romfs` 的目录（或它们的上一级）。
   Android 会请求访问权限：应用会复制 ExeFS，并在自己的目录里构建 RomFS 数据镜像。
3. 等待初始化：复制、游戏数据镜像与游戏代码（保持屏幕常亮）。首次运行更久；
   之后的启动会复用已编译的代码，除非某次更新要求重编。
4. 可选，Adreno 上为了 60 fps：**设置 > 启动前 > Vulkan 驱动 > 选择驱动**，选择 Turnip 的 `.zip`，
   然后 **立即重启**。不用它的话，游戏跑在手机自带驱动上，光照正常，30 fps。
   如果某个驱动让屏幕花屏或应用打不开，长按应用图标选 **系统驱动**：应用会回到手机自带驱动。
   在 Adreno 830（骁龙 8 Elite）上测试过：

   | 驱动 | 结果 |
   | --- | --- |
   | Turnip Gen8 V37 | 可用，60 fps |
   | [whitebelyash](https://github.com/whitebelyash/freedreno_turnip-CI/releases) Stable Turnip V2、Mainline Turnip V32、A8XX Turnip V29 | 可用，60 fps |
   | Turnip v26.3.0-R6 | 屏幕花屏，随后在游戏中崩溃 |
   | [K11MCH1](https://github.com/K11MCH1/AdrenoToolsDrivers/releases) Turnip A8XX Draft（`turnip_a8xx.zip`） | GPU 失去响应（设备丢失） |
   | K11MCH1 Turnip v26.0.0 R8 | 为 A7xx 编译：这里没有可用 GPU，应用改用手机自带驱动 |
5. **开始游戏**，用触屏操作或手柄。屏幕上的菜单键、返回键，或手柄上的 Minus + Plus 可打开游戏内菜单。

应用把一切放在 `Android/data/com.lasrecomp.android/files/`：存档、设置、管线缓存与游戏文件。
卸载应用会删掉这个目录，存档也在内：请先把 `save` 目录复制出来。

## 从源码构建

Windows + Git Bash、带 **NDK 30.0.16248370** 与 **CMake 3.30.4** 的 Android SDK、一台开启 USB 调试的手机，
以及 Python 3.12。Linux 上用 bash 应该也能走同样的步骤，但那边没有测试过。

1. 应用的依赖（SDL3、shaderc、Vulkan-Headers、VMA、volk 与 DejaVu 字体）：
   `bash android/deps/build_deps.sh`。
2. 可选：FFmpeg，用于影片与启动界面背景。它的 `configure` 需要 Unix shell，所以在 Windows 上用 WSL 跑：
   `wsl -- bash android/deps/build_ffmpeg.sh`。
3. 应用内置的游戏代码构建器（供 Android 用的 Python 3.14、capstone、lz4、zig）：
   `bash android/deps/build_gen_deps.sh`；unicorn 也需要 Unix shell：
   `wsl -- bash android/deps/build_unicorn.sh`。
4. 应用本身：`cd android/lasrecomp && ./gradlew assembleRelease`
   （若 `~/.lasrecomp/keystore.properties` 存在则用它里面的密钥签名，否则用 debug key），然后
   `adb install -r app/build/outputs/apk/release/app-release.apk`，并按[安装](#安装)一节设置。

开发时，游戏代码仍可以在 PC 上编译再推送：`bash android/build_gamelib.sh <dump>/exefs`
（配合 `python -m venv tools/venv` 与 `pip install lz4 capstone unicorn`），然后
`bash android/push_game.sh <含 exefs/ 与 romfs/ 的目录>`。

本分支的签名配置模板见
[`android/lasrecomp/ci/keystore.properties.example`](android/lasrecomp/ci/keystore.properties.example)，
版本号规则与发版流程见 [docs/CI-RELEASE.md](docs/CI-RELEASE.md)。

## 致谢

- **[LAS Recomp](https://github.com/peiikai/LASRecomp)**（peiikai）：本移植版建立在其重编译器、运行时、Vulkan 渲染器
  与着色器翻译器之上。它的 README 致谢了背后的所有人，包括 LAS-Randomizer 与它的选项重新实现的社区 Mod。
- **[LASRecomp-Android-Plus](https://github.com/Nycolazs/LASRecomp-Android-Plus)**（Nycolazs，MIT）：
  面向低于 1.3 的驱动的 Vulkan 1.1 / 1.2 扩展路径（`runtime/nvn/vulkan/device.c`）沿用了它的思路；
  它的游玩配置热点函数列表（`tools/hot_functions.txt`）让初始化能更快编译其余部分；
  基于重投影的帧生成（`runtime/nvn/vulkan/framegen.c`）改编自它。经作者同意，更多内容已合入本项目：
  驱动错误弹窗、手柄选择、安全的设置写入、保活服务、固定摇杆与上下文感知的触屏按键、按字母识别面键、
  带道具图标的按键、1.25x / 1.5x、模糊侧边栏、Turnip 的压缩渲染目标（UBWC）、小光晕按自身尺寸、
  惰性颜色复制、手机上不导入堆区、构建的文件顺序，以及西班牙语、法语、德语、意大利语、荷兰语菜单文案
  （来自 [deedywu/LASRecomp](https://github.com/deedywu/LASRecomp)，MIT）。
- [SDL 3](https://libsdl.org)（zlib）：窗口、输入、音频与 Android activity。
- [libadrenotools](https://github.com/bylaws/libadrenotools)（bylaws，BSD 2-Clause）：加载自定义 Vulkan 驱动。
- [Mesa Turnip](https://docs.mesa3d.org/drivers/freedreno.html)（MIT）：它面向的 Vulkan 驱动（不随包提供）。
- [shaderc](https://github.com/google/shaderc)（Apache 2.0）、[Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)
  （Apache 2.0 / MIT）、[volk](https://github.com/zeux/volk)（MIT）与
  [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)（MIT）。
- [FFmpeg](https://ffmpeg.org)（LGPL 2.1+，可选，动态链接）：影片播放。
- 在手机上运行的游戏代码构建器：[Python](https://www.python.org) 3.14（PSF 许可）、
  [Zig](https://ziglang.org) 0.17（MIT）、[Capstone](https://github.com/capstone-engine/capstone) 5.0.9（BSD 3-Clause）、
  [Unicorn](https://github.com/unicorn-engine/unicorn) 2.1.4（GPL 2.0；其未经修改的源码：
  [2.1.4](https://github.com/unicorn-engine/unicorn/archive/refs/tags/2.1.4.tar.gz)，由
  `android/deps/build_unicorn.sh` 构建）与 [LZ4](https://github.com/lz4/lz4) 1.10.0（BSD 2-Clause）。
- [Dear ImGui](https://github.com/ocornut/imgui)（MIT）：叠加菜单与启动界面。
- [SMAA](https://github.com/iryoku/smaa) 与 [AMD FidelityFX Super Resolution 1](https://github.com/GPUOpen-Effects/FidelityFX-FSR)
  （MIT）：抗锯齿与放大选项，内置在渲染器里。
- [Snapdragon Game Super Resolution 1](https://github.com/SnapdragonStudios/snapdragon-gsr)（高通，BSD 3-Clause，
  [`third_party/sgsr/LICENSE`](third_party/sgsr/LICENSE)）：Snapdragon GSR 1 放大选项
  （`runtime/nvn/vulkan/shaders.c`）。
- 游戏 Mod（设置 > Mod，`runtime/mods.c`）：内含 `romfs` 文件夹的目录，与 Eden、Ryujinx 的相同，
  可在菜单里逐个开关；不包含任何 Mod。用「设置 > Mod > 导入 Mod 文件夹」导入 Mod 目录
  （或复制到 `Android/data/com.lasrecomp.android/files/mods/`）。
- 启动界面上的 Android 机器人图标，复制或修改自 Google 创作并共享的作品，依据
  [Creative Commons 3.0 Attribution License](https://creativecommons.org/licenses/by/3.0/) 使用。
- [DejaVu fonts](https://dejavu-fonts.github.io)（Bitstream Vera 许可）：Switch 系统字体的替身。

## 法律声明

这是一个同人项目，与任天堂没有任何隶属、授权或赞助关系。《塞尔达传说》与《织梦岛》是任天堂的商标。
本仓库与任何构建都不含游戏文件、游戏代码、资源或字体。游玩必须由你提供自己合法获得的游戏文件。

## 参与贡献

Android 的 bug 与建议请报到上游仓库的 [Issues](https://github.com/mamcjr/LASRecompAndroid/issues)。
葡语与英语都可以。设备信息该写什么、Android 开发、验证与 PR 规范见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 许可证

代码以 [MIT 许可证](LICENSE) 开源。原版 LAS Recomp 对 Peiikai 的版权声明予以保留；
另有声明注明 mamcjr 与 LASRecompAndroid 贡献者的贡献。第三方组件各自保留其许可证
（见上，以及[原版 README](https://github.com/peiikai/LASRecomp#license)）。
该许可证只覆盖本项目的代码：它不授予任天堂游戏的任何权利，游戏需你自己提供。
