# LAS Recomp for Android · 汉化版

**简体中文** | [English](README.md) | [Português](README.pt-BR.md)

这是 [`mamcjr/LASRecompAndroid`](https://github.com/mamcjr/LASRecompAndroid) 的汉化分支：把**应用自身的启动界面与设置菜单**
翻译成简体 / 繁体中文，跟随手机语言自动切换，也可以在菜单里手动选择。

游戏本体的文本**不受影响**：那是游戏自己的语言设置（RomFS 自带中文），与界面语言是两套独立选项。

## 下载与安装

到 [Releases](../../releases) 下载对应版本：

```
LASRecomp-0.5.11-zh.1.apk          安装包
LASRecomp-0.5.11-zh.1.apk.sha256   校验值
signature.txt                      签名证书与构建信息
```

```bash
adb install -r LASRecomp-0.5.11-zh.1.apk   # 覆盖安装，存档保留
```

也可以在手机上直接点开 APK（需允许「安装未知应用」）。要求 **Android 11+ / arm64-v8a / Vulkan 1.3**。
首次运行仍需自备游戏 dump，按启动器引导设置（见 [docs/ANDROID-PORT.md](docs/ANDROID-PORT.md)）。

## 版本号规则

| 项 | 规则 | 例 |
|---|---|---|
| 版本名 | `<上游版本>-zh.<第几版>` | `0.5.11-zh.1` |
| versionCode | `<上游 versionCode>*100 + 第几版>` | `2701` |

所以版本号只会变大，每个新版本都能直接覆盖旧版本，不需要卸载。
每次发布都用同一把密钥签名（证书 SHA-256 `f00f7ebe8bb0d960a74399db2d4a8fba5fe13d5448dd2a662c945a896a2f4cff`），
签名不一致时 Android 会拒绝升级；签名只能说明包出自同一把密钥，不具备身份鉴别作用（见 [docs/CI-RELEASE.md](docs/CI-RELEASE.md)）。

## 汉化范围

- 启动界面、设置菜单、游戏内菜单（简体 / 繁体，各 878 条文案，覆盖率 100%）；
- Java 侧弹窗、快捷方式、通知渠道（跟随系统语言）；
- 打包 Noto Sans CJK SC 子集字体，不依赖 ROM 字体，避免启动闪退。

细节与维护方式见 [docs/ZH-LOCALIZATION.md](docs/ZH-LOCALIZATION.md)，
构建 / 发版规范见 [docs/CI-RELEASE.md](docs/CI-RELEASE.md)。

## 自行构建

与上游相同（NDK `30.0.16248370`、CMake `3.30.4`、JDK 17），步骤见
[README 的 *Building from source*](README.md#building-from-source)。

```bash
bash android/deps/build_deps.sh
cd android/lasrecomp && ./gradlew assembleRelease
```

release 签名默认读 `~/.lasrecomp/keystore.properties`，模板见
[`android/lasrecomp/ci/keystore.properties.example`](../android/lasrecomp/ci/keystore.properties.example)。
没有该文件时退回 debug key（仅适合自测，不能覆盖安装）。

## 上游与许可

上游项目：[`mamcjr/LASRecompAndroid`](https://github.com/mamcjr/LASRecompAndroid)；原始项目：
[`peiikai/LASRecomp`](https://github.com/peiikai/LASRecomp)。本仓库 `main` 是上游纯镜像，
汉化改动全部在 `feat/zh-launcher` 分支。许可证与上游一致，见 [LICENSE](LICENSE)。

**不含任何游戏文件**：需要自行合法获得 *塞尔达传说 织梦岛* v1.0.0 / v1.0.1 / v1.1.0 的提取文件。
