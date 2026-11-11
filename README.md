# DreamGUIDemo

Demo project for the Dream toolchain plugins — [DreamGUI](https://github.com/TypeDreamMoon/DreamGUI) (text-defined `.dui` UI), [DreamShader](https://github.com/TypeDreamMoon/DreamShader) (text materials) and [DreamFX](https://github.com/TypeDreamMoon/DreamFX) (text Niagara effects) — on Unreal Engine 5.8. It holds the scenes the DreamGUI demo video was recorded from.

DreamGUI 系列插件的演示工程：DreamGUI 演示视频里的画面都出自这里。

| 内容 | 是什么 | 地图 |
|---|---|---|
| **NOCTURNE//OS** | 原创的一整套科幻系统界面：3D 星图主菜单、设置页、档案（拖放）、道具轮盘、音乐盒、日志、出发、游戏内 HUD、世界空间面板、图层效果、文字与性能展示；带一个按镜头名回放的导演 | `/Game/Nocturne/Maps/Nocturne_Demo`（默认地图） |
| **P3R 风格练习** | 女神异闻录 3 Reload 暂停菜单族：水下主菜单、SYSTEM、COMMU、STATUS 和两版早期稿，串成可操作的流程；细节见 [Docs/P3R.md](Docs/P3R.md) | `/Game/P3R/Maps/P3R_Demo` |
| **NieR / 底特律 风格练习** | NieR:Automata 标题/菜单/设置；底特律：变人 读取画面、HUD、线索标签、目标、分析模式 | `/Game/UITests/NieR/Maps/NieR_TestMap`、`/Game/UITests/Detroit/Maps/Detroit_TestMap` |

> **风格练习，非官方。** NieR、底特律、P3R 三套只学原作界面的版式和动效，与原作及其权利方无关；角色、名字和美术都是原创（程序生成或 AI 绘图），不含任何原作素材、字体或截图。

## 准备

- Windows，Unreal Engine 5.8，Visual Studio 2022（C++ 游戏开发）。
- 三个插件是 git 子模块，克隆后拉一层即可（DreamGUI 自己的 msdfgen 子模块不用拉，照样能编）：

  ```bash
  git clone https://github.com/TypeDreamMoon/DreamGUIDemo.git
  cd DreamGUIDemo
  git submodule update --init
  ```
- 右键 `DreamGUIDemo.uproject` → Generate Visual Studio project files，编 `DreamGUIDemoEditor`（Development Editor），或直接双击 uproject 让引擎编译。
- `UnrealBridge` 插件是可选的（只有 `Tools/Nocturne` 里经编辑器重编控件的脚本用到它），没有它工程照常打开。

## 操作

**NOCTURNE//OS**：打开工程点 Play。鼠标、键盘、手柄都能用；任意键或点击唤醒标题。编辑器里 Esc 会停 PIE，返回用 Backspace 或点「ESC 返回」。命令行加 `-DemoShot=<镜头名>` 回放视频里的镜头（Tour、Title、Menu、Settings、Archive、Wheel、Music、Log、Depart、Hud、Full、World、LayerFx、Text、Perf、CardOpen、CardEnd、Test…），`-DemoMeasure` 实时测帧率后退出。

**P3R**：见 [Docs/P3R.md](Docs/P3R.md#运行)（Tab/Esc 开菜单，↑↓ 选，Enter 决定，Esc 返回，Tab 切换三版设计稿）。

**NieR**：方向键选、Enter 决定、Backspace 返回；菜单页签用手柄肩键切换。

**底特律**：WASD 移动、Shift 跑、鼠标或方向键看；Q 分析模式、E 分析线索、Tab 目标、1–5 传送到线索。

无人值守录制用按键脚本：NieR / 底特律加 `-DemoKeys=时间:键,...`（例如 `-DemoKeys=1.4:Enter,2.3:Down`，`W/2.5` 表示按住 2.5 秒），P3R 用 `-P3RKeys=`（还能 `Move@X;Y`、`Click@X;Y` 驱动鼠标），NOCTURNE//OS 的镜头由导演自己驱动。

## 目录

| 路径 | 内容 |
|---|---|
| `Source/DreamGUIDemo` | NOCTURNE//OS 和镜头导演、按键脚本 |
| `Source/P3RUIDemo` | P3R 风格练习 |
| `Source/DreamGUIDemoEditor` | 编辑器命令行工具：`P3RSetup`（导入 P3R 美术、建字体和地图）、`ScrubImportPaths`（去掉资产里的本机绝对路径） |
| `Plugins/DevUITests` | NieR / 底特律 风格练习（C++） |
| `Plugins/DreamGUI` `DreamShader` `DreamFX` | 子模块 |
| `DUI/` | 所有界面的 `.dui` 源文件（`DUI/DreamUI.code-workspace` 可直接用 VS Code 打开，装 DreamUI Language Support 扩展） |
| `DShader/` | DreamShader 材质源文件 |
| `DFX/` | DreamFX 粒子源文件 |
| `Art/` | 贴图、字体、PSD 的源文件和处理脚本 |
| `Tools/` | 无头管线脚本（PowerShell 7 + Python 3） |
| `Docs/` | 各套界面的说明 |

## 工具

脚本自己找引擎：先看环境变量 `UE_ENGINE_ROOT`，再找 Epic 启动器装的 UE 5.8；都没有就在 `Tools/Local.ps1`（不入库）里写 `$env:UE_ENGINE_ROOT = '<引擎目录>'`。录制要 `ffmpeg` 在 PATH 上。

```powershell
Tools/check-load.ps1                                   # 无头检查：编全部蓝图 + 四张地图各跑 25 秒
Tools/run-commandlet.ps1 -Run ScrubImportPaths         # 提交新导入/重新生成的资产前跑一次
Tools/Nocturne/record.ps1 -Map "/Engine/Maps/Entry?game=/Script/DreamGUIDemo.NocturneGameMode" -Seconds 75 -Name tour -Extra '-DemoShot=Tour'
Tools/P3R/rebuild.ps1                                  # P3R 整条管线
```

离屏录制（`-RenderOffScreen -dumpmovie`）按固定步长逐帧出图，录出来的片子每次一样，也不占前台。

## 许可

字体：NieR 用 M PLUS 1p（OFL），底特律用引擎自带 Roboto（Apache 2.0），P3R 用改窄的 Roboto（Apache 2.0）和 Source Han Sans（OFL），NOCTURNE//OS 的 emoji 用 Noto Color Emoji（OFL）；许可证文本在各字体旁边。
