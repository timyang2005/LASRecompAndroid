# 构建与发版规范（LASRecompAndroid 汉化分支）

本仓库是 [`mamcjr/LASRecompAndroid`](https://github.com/mamcjr/LASRecompAndroid) 的 fork：

| 分支 | 内容 |
|---|---|
| `main` | 上游纯镜像，只做 fast-forward，不放本 fork 的任何改动 |
| `feat/zh-launcher` | 汉化与 CI 改动，发布也从这里出 |

## 1. 版本号

版本号只有一个来源：**[`android/lasrecomp/version.properties`](../android/lasrecomp/version.properties)**。

```properties
baseName=0.5.11   # 上游基线版本（上游 build.gradle.kts 的 versionName）
baseCode=27       # 上游基线 versionCode
forkRev=1         # 本 fork 在同一基线上的第几次发布
```

推导规则（写死在 `app/build.gradle.kts` 里）：

| 项 | 规则 | 例 |
|---|---|---|
| `versionName` | `<baseName>-zh.<forkRev>` | `0.5.11-zh.1` |
| `versionCode` | `<baseCode>*100 + forkRev` | `27*100+1 = 2701` |

规则的作用：

- **单调递增**：`0.5.11-zh.1`=2701 → `0.5.11-zh.2`=2702 → `0.5.12-zh.1`=2801。
  Android 要求升级包的 `versionCode` 大于已装版本，这样每个新版本都能直接覆盖安装，存档不丢。
- **可回溯**：看到 APK 的版本号就知道它基于哪个上游版本、是这个基线上的第几版。
- **一个来源**：Gradle、启动屏右下角的版本号、APK 文件名、Release 标题、tag 全部由它推导，
  工作流会断言 APK 里的 `versionName/versionCode` 与它一致，不一致直接失败。

`forkRev` 到 99 就进位到 100 会与下一个 base 冲突，实际不可能（一个上游版本不会有 99 次发布）。

## 2. 发版流程

```bash
# 1) 改版本号：同一基线上再发一版就 forkRev + 1
#    （上游基线变了则先做第 4 节的同步，baseName/baseCode 改成新的，forkRev 回到 1）
$EDITOR android/lasrecomp/version.properties

# 2) 提交
git commit -am "release: 0.5.11-zh.2"

# 3) 打 tag 并推送（tag 必须与推导出的 versionName 完全一致，前缀 v）
git tag v0.5.11-zh.2
git push origin feat/zh-launcher v0.5.11-zh.2
```

CI 随后自动：

1. 跑中文表与字体校验（`checks` job）；
2. 编译、签名 APK，校验版本号 / 字体 / 证书指纹；
3. 用 tag 创建 GitHub Release，附 `LASRecomp-<versionName>.apk`、`.sha256`、`signature.txt`。

重跑同一个 tag 会覆盖同名附件（`--clobber`），不会产生第二个 Release。

**不打 tag 的 push** 只构建、产物进 Actions Artifacts，用来验证分支能不能编过，不会产生 Release。
`workflow_dispatch` 同理。

## 3. 工作流

| 文件 | 职责 |
|---|---|
| `.github/workflows/i18n-check.yml` | 改到中文表 / 字体 / 校验脚本时跑静态检查（几秒） |
| `.github/workflows/android-apk.yml` | 构建 + 签名 + 发布；内部先跑同一套 `checks`，发布前必有校验 |

构建链按 README 的 *Building from source*：

| 步骤 | 脚本 | 必需 | 失败时 |
|---|---|---|---|
| 1 | `android/deps/build_deps.sh`（SDL3 / shaderc / Vulkan 头 / VMA / volk / DejaVu） | ✅ | 重试 3 次，仍失败则 job 失败 |
| 2 | `android/deps/build_ffmpeg.sh`（影片、启动器背景） | 可选 | 清掉半成品，CMake 走 `LARECOMP_NO_FFMPEG` |
| 3 | `android/deps/build_gen_deps.sh`（手机端编译游戏代码用） | 可选 | 删掉 `out/gen`，APK 不含游戏代码构建器 |
| 4 | `./gradlew assembleRelease` | ✅ | — |

Runner 上装 `platforms;android-36`、`build-tools;36.0.0`、`cmake;3.30.4`、`ndk;30.0.16248370`，JDK 17。

## 4. 同步上游

```bash
git fetch upstream            # upstream = https://github.com/mamcjr/LASRecompAndroid
git checkout main && git merge --ff-only upstream/main && git push origin main
git checkout feat/zh-launcher && git merge main
# 冲突一般在 runtime/ui/launcher.mm、menu.mm、settings.c（汉化改过的文件）
```

合并完成后：

1. 把上游新的 `versionName` / `versionCode` 抄进 `version.properties` 的 `baseName` / `baseCode`；
2. `forkRev` 回到 `1`；
3. `python tools/i18n_check.py` —— 上游新增的界面文案如果没翻译，这里会直接列出；
4. 表里出现新字时，重建打包字体子集（命令在 `tools/font_coverage.py` 头部）。

## 5. 签名

发布包用固定密钥签名，证书 SHA-256：

```
f00f7ebe8bb0d960a74399db2d4a8fba5fe13d5448dd2a662c945a896a2f4cff
```

指纹固定意味着**每次发布的 APK 签名相同**，老用户可以直接覆盖安装、存档保留。
密钥本身不进仓库，放在仓库 Secrets：

| Secret | 说明 |
|---|---|
| `SIGNING_KEYSTORE_B64` | keystore 的 base64（`base64 -w0 release.jks`） |
| `SIGNING_STORE_PASSWORD` | store 密码 |
| `SIGNING_KEY_ALIAS` | 别名 |
| `SIGNING_KEY_PASSWORD` | key 密码 |

缺任何一个，工作流在第一步就失败，不会交付一个装不上的包。

> **这把密钥在早期提交里出现过**（当时随仓库分发），所以它只能证明「包来自这条发布流水线、能覆盖安装」，
> 不具备防篡改的身份鉴别作用：拿到旧提交的人也能签出同证书的包。要从根上解决只能换密钥，
> 代价是所有已装用户必须卸载重装（存档会丢），因此这里选择保留并如实说明。

> **不要更换密钥**：换密钥等于换签名，已装用户必须先卸载（`Android/data/com.lasrecomp.android/files/` 里的存档会一起删掉）。
> 只有密钥泄露并且愿意承担「所有用户重装」的代价时才轮换，轮换时同步改
> `android-apk.yml` 里的 `EXPECTED_CERT_SHA256` 和本文档。

工作流用 `apksigner` 校验 v2/v3 签名并提取指纹，与上面的常量比对；指纹不符直接失败。
本地复核：

```bash
$ANDROID_HOME/build-tools/36.0.0/apksigner verify --verbose --print-certs LASRecomp-0.5.11-zh.1.apk
```

## 6. 产物命名

```
LASRecomp-<versionName>.apk          例 LASRecomp-0.5.11-zh.1.apk
LASRecomp-<versionName>.apk.sha256   APK 的 sha256
signature.txt                        apksigner 完整输出 + 证书指纹 + 构建链接
```

文件名里没有构建号、没有 `latest`：一个版本一个文件，历史 Release 每个 tag 一份，不再维护「滚动 Release」。
