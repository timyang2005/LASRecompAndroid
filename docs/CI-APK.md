# 固定签名 APK 的 CI 构建

`.github/workflows/apk.yml` 在 GitHub Actions 上编译 `feat/zh-launcher` 的 release APK，
用**每次都相同的密钥**签名，产出的 APK 可以直接安装、后续版本可以直接覆盖升级。

## 为什么需要固定签名

`app/build.gradle.kts` 的 release 签名逻辑：

```kotlin
val releaseKey = File(System.getProperty("user.home"), ".lasrecomp/keystore.properties")
signingConfigs { if (releaseKey.exists()) create("release") { /* 读 4 个属性 */ } }
buildTypes { getByName("release") {
    signingConfig = signingConfigs.findByName("release") ?: signingConfigs.getByName("debug")
} }
```

没有 `~/.lasrecomp/keystore.properties` 时，release 会用 AGP 自动生成的 **debug key**。
CI runner 每次都是新环境，debug key 每次都不同 —— 两次构建的 APK 签名不一致，
`adb install -r` 会报 `INSTALL_FAILED_UPDATE_INCOMPATIBLE`，只能先卸载（存档会丢）。

所以工作流把 `~/.lasrecomp/keystore.properties` 指向仓库内的
`android/lasrecomp/ci/lasrecomp-release.jks`，并在构建后用 `apksigner` 断言
证书指纹等于 `EXPECTED_CERT_SHA256`。指纹不符直接失败，不会交付一个「装不上」的包。

| 项 | 值 |
|---|---|
| Keystore | `android/lasrecomp/ci/lasrecomp-release.jks`（JKS，RSA 2048，有效期至 2054） |
| 别名 / 密码 | `lasrecomp` / `lasrecomp`（公开，见下方安全说明） |
| 证书 SHA-256 | `f00f7ebe8bb0d960a74399db2d4a8fba5fe13d5448dd2a662c945a896a2f4cff` |
| 包名 | `com.lasrecomp.android` |
| ABI / 系统 | `arm64-v8a`，Android 11+（minSdk 30），需 Vulkan 1.3 |

> ⚠️ **密钥随仓库公开**：任何人都能用它签出同签名的 APK。它只用于让 CI 产物
> 能覆盖安装，不具备任何身份鉴别作用。不要用于应用商店或任何需要防篡改的场景。

## 构建链

按 `README.md` 的 *Building from source*，工作流执行：

| 步骤 | 脚本 | 必需 | 失败时 |
|---|---|---|---|
| 1 | `android/deps/build_deps.sh`（SDL3 3.4.16 / shaderc 2026.3 / Vulkan 头 / VMA / volk / DejaVu） | ✅ | 重试 3 次，仍失败则 job 失败 |
| 2 | `android/deps/build_ffmpeg.sh`（影片、启动器背景） | 可选 | 清掉半成品，CMake 走 `LARECOMP_NO_FFMPEG` |
| 3 | `android/deps/build_gen_deps.sh` + `build_unicorn.sh`（Python 3.14 / zig / capstone / lz4，手机端编译游戏代码用） | 可选 | 删掉 `out/gen`，APK 不含游戏代码构建器 |
| 4 | `bash ./gradlew --no-daemon assembleRelease` | ✅ | — |

第 2、3 步失败**不阻断 APK 产出**：清掉半成品后 Gradle 会自动降级。
这样即使上游某个下载源失效，也至少能拿到一个可安装、能进启动器的包。

环境由 runner 自带 SDK 安装：`platforms;android-36`、`build-tools;36.0.0`、
`cmake;3.30.4`、`ndk;30.0.16248370`，JDK 17（Temurin），并复用 SDK NDK
作为 `ANDROID_NDK_LINUX`（`build_ffmpeg.sh` / `build_unicorn.sh` 原本会另下载一份 Linux NDK）。
`actions/cache` 缓存下载的源码 tarball、编译产物和 Gradle 依赖，第二次构建显著缩短。

## 如何触发

1. **自动**：向 `feat/zh-launcher` 推送 `.github/workflows/apk.yml` 或
   `android/lasrecomp/ci/**` 的改动即触发一次。
2. **手动（UI）**：工作流需在默认分支才会显示 *Run workflow* 按钮。本 fork 的默认分支
   是 `main`（上游纯镜像），因此用 REST API 指定分支触发：
   ```bash
   curl -X POST \
     -H "Authorization: Bearer <PAT，需 workflow scope>" \
     -H "Accept: application/vnd.github+json" \
     https://api.github.com/repos/timyang2005/LASRecompAndroid/actions/workflows/apk.yml/dispatches \
     -d '{"ref":"feat/zh-launcher","inputs":{"full":"true","publish":"true"}}'
   ```
   `full=false` 可跳过第 2、3 步，只出最小可安装 APK；`publish=false` 不更新滚动 Release。
3. **打 tag**：把工作流同步到 `main` 后也可加 `push: tags: ['v*']` 触发，本 fork 暂未启用。

> 若希望 UI 上直接有按钮，把 `apk.yml` 同步到 `main` 即可（它 checkout 的是当前 ref，
> 在 main 上跑会构建英文版；要构建汉化版需在 `actions/checkout` 里显式指定 `ref: feat/zh-launcher`）。

## 产物与下载

每次成功构建产出：

- **Artifacts**：`LASRecomp-zh-APK`，含带版本/运行号的 APK、固定名
  `LASRecomp-zh-latest.apk`、两个 `.sha256`、`signature.txt`（`apksigner` 完整输出）。
- **滚动 Release**：`apk-latest`（prerelease），同名文件用 `--clobber` 覆盖，
  提供免登录下载直链：
  <https://github.com/timyang2005/LASRecompAndroid/releases/download/apk-latest/LASRecomp-zh-latest.apk>

安装：

```bash
adb install -r LASRecomp-zh-latest.apk   # 覆盖安装不会丢存档
```

或在手机上直接点开 APK（需允许「安装未知应用」）。首次运行仍需自备游戏 dump，
按启动器引导设置；游戏本体的 RomFS 文本自带中文，与本次汉化无关。

## 换用自己的密钥

在仓库 *Settings → Secrets and variables → Actions* 添加（全部可选，但要么都配要么都不配）：

| Secret | 说明 |
|---|---|
| `SIGNING_KEYSTORE_B64` | keystore 文件的 base64（`base64 -w0 my.jks`） |
| `SIGNING_STORE_PASSWORD` | store 密码 |
| `SIGNING_KEY_ALIAS` | 别名，留空则 `lasrecomp` |
| `SIGNING_KEY_PASSWORD` | key 密码，留空则同 store 密码 |

配置后工作流改用这把密钥，并跳过内置指纹断言（只打印实际指纹）。
**注意换密钥等于换签名**：已装的用户必须先卸载旧版，存档请先备份
（存档在 `Android/data/com.lasrecomp.android/files/`，卸载会一并删除）。

## 校验清单

工作流末尾的「校验签名并整理产物」会：

1. `apksigner verify --verbose --print-certs`，要求 v2（或 v3）签名为 `true`；
2. 提取 `certificate SHA-256 digest`，去掉可能存在的冒号与空白后转小写，
   与 `EXPECTED_CERT_SHA256` 比对；
3. 生成 `sha256sum` 与 `signature.txt` 一起上传。

> **apksigner 输出格式不统一**：build-tools 36 的 verbose 输出是
> `V2 Signer: certificate SHA-256 digest: …`，旧版则是
> `Signer #1 certificate SHA-256 digest: …`。校验脚本只锚定
> `certificate SHA-256 digest` 字段本身，不依赖前缀。若按旧格式硬匹配，
> 指纹会提取为空并误判为签名失败——首次构建就踩过这个坑：APK 其实已正确签名
> （证书指纹、v2 都对），但 job 被判失败，Artifact 与 Release 都没有上传。

本地复核：

```bash
$ANDROID_HOME/build-tools/36.0.0/apksigner verify --verbose --print-certs LASRecomp-zh-latest.apk
keytool -printcert -jarfile LASRecomp-zh-latest.apk | grep SHA256
```
