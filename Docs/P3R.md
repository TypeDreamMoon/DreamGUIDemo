# P3R 风格练习 — 女神异闻录 3 Reload 菜单界面

用 DreamGUI（`.dui` 文本界面）、DreamShader（材质）、DreamFX（Niagara 粒子）复刻 P3R 暂停菜单这一族界面，并串成一条可以操作的流程。**风格练习、非官方**：角色、名字、图片都是原创（GPT 图像生成），界面的版式和动效照游戏做。

代码在 `Source/P3RUIDemo`（运行时模块）和 `Source/DreamGUIDemoEditor`（`P3RSetup` 命令行工具），界面 `DUI/P3R`，材质 `DShader/P3R`，粒子 `DFX/P3R`，美术源文件和处理脚本 `Art/P3R`，管线脚本 `Tools/P3R`。

## 运行

UE 5.8 打开 `DreamGUIDemo.uproject`，打开地图 `/Game/P3R/Maps/P3R_Demo`，点 Play 即可。

| 场景 | 键盘 | 手柄 |
|---|---|---|
| 野外 → 打开菜单 | Tab / Esc / Enter | Start / Y / A |
| 选择 | ↑↓ 或 W/S | 十字键 / 左摇杆 |
| 决定 | Enter / Space | A |
| 返回 / 关闭菜单 | Esc / Backspace | B |
| 菜单里切换设计稿（最终版 → 早期稿 A → 早期稿 B） | Tab / F1 | View |
| STATUS 页 L1 / R1 | Q / E | LB / RB |

鼠标：悬停选中、点击决定。

流程：野外（教室）→ 菜单（画面沉进「心之海」）→ STATUS / COMMU / SYSTEM 三页；SYSTEM 里 RETURN to TITLE 回到野外；其余菜单项会抖一下表示没收录。

## 对应的参考图

| 界面 | `.dui` | C++ |
|---|---|---|
| 暂停菜单（最终版） | `DUI/P3R/P3R_MainMenu.dui` | `UP3RMainMenu` + `FP3RWordMenu` |
| SYSTEM | `DUI/P3R/P3R_System.dui` | `UP3RSystemMenu` |
| COMMU 列表 | `DUI/P3R/P3R_Commu.dui` + `Components/P3R_CommuRow.dui` | `UP3RCommuScreen` / `UP3RCommuRow` |
| STATUS 队伍 | `DUI/P3R/P3R_Status.dui` + `Components/P3R_PartyRow.dui` | `UP3RStatusScreen` / `UP3RPartyRow` |
| 早期稿 A（玻璃板） | `DUI/P3R/P3R_ProtoGlass.dui` | `UP3RProtoGlass` |
| 早期稿 B（拼贴） | `DUI/P3R/P3R_ProtoMosaic.dui` | `UP3RProtoMosaic` |
| 野外 HUD / 底层水面 | `P3R_Field.dui` / `P3R_Backdrop.dui` | `UP3RFieldHud` / `UP3RBackdrop` |

流程由 `AP3RDirector` 管；材质在 `DShader/P3R/Materials`（`M_P3R_Sea` 把暂停画面色调映射成水下蓝、加水纹/焦散/光柱；`M_P3R_InkGlass` 是早期稿 A 的玻璃板；`M_P3R_Figure` 是菜单里的主角：外套和最暗的阴影半透出水底、细线保留、轻微水波，`Silhouette=1` 时是转场用的深蓝剪影），粒子在 `DFX/P3R`（气泡、碎片）。

照实机录像（YouTube「Pause Menu - Persona 3 Reload」逐帧）对过的动效：

- **光标即遮罩**：白色楔形压到的字会变色——选中词（黑字）压到的部分变红，其他词压到的部分变白；光标移动、每秒的脉动、鼠标悬停拖动时，扫过的词都会跟着闪。实现是每帧把 `CursorWhite` 的轮廓换算到每个词自己的坐标系，交给裁在字形上的 `<词>Red` 形状（`FP3RWordMenu::UpdateInk`）。
- **进子页**：主角特写化成深蓝剪影从左下扫过菜单，新页面从选中的那个词的位置以墨团形状张开，开始时略放大并蒙一层蓝（`UP3RScreen::PlayBlotIn`、页面里的 `Closeup` 节点）。
- **回主菜单**：页面从右上角开出一个圆洞，主菜单在洞里以同一个圆显现，并亮一下（`PlayBlotOut` + 主菜单的 `Reveal`）。
- **STATUS**：主角手遮脸，指缝间的斜玻璃片显示所选队员的眼睛；换人时玻璃片白闪，新眼睛从右边滑进来。
- **COMMU**：主角举着的牌是 UI 画的，换羁绊时翻面。两种列表的选中样式都是从左边擦入（行里 `Chosen` 节点的 MaskTransition）。

角色是原创形象，只照游戏的渲染方式和姿势画。主菜单、STATUS、COMMU 三张（`Art/P3R/Gen/V3`）是拿对应的实机截图当姿势和构图参考、拿自家旧图当身份参考，多轮生成出来的：主菜单倒着的头在左下、白衬衫和手臂斜向右上；STATUS 仰头、指尖勾在发际线、手掌横在眉骨；COMMU 侧身、下巴顶着画面上沿、手举白牌。这几张本身就是平涂，只把红领带改成黑芯红边（`toon.ribbon_only`）。STATUS 的手由 `Art/P3R/hand_layer.py` 抠成单独一层（`T_P3R_HeroHandFront`），叠在队员眼睛的玻璃片上面，所以眼睛是从指尖和手掌之间露出来的；队员的眼睛也按这张的仰头角度重新画过（平涂、往左上看），在玻璃片里只比主角的眼睛略大。COMMU 白墙上的投影是同一张图的纯平灰剪影（`M_P3R_Figure` 的 `SilhouetteShade=0`）。

SYSTEM 和拼贴里还在用的第二轮旧图（`Art/P3R/Gen/V2`）是动画插画的画法（发丝多、渐变多、大块红领带），`Art/P3R/toon.py` 把它们重绘成菜单里的赛璐璐配色：皮肤和衬衫纯白加淡紫灰阴影、头发三档平涂蓝、外套和水同色并半透、细深蓝描边、领带变成黑红两色。SYSTEM 里那张外套保持实心靛蓝（贴在白纸上）。

水下背景（`M_P3R_Sea`）是参考图量出来的渐变：顶上水面的青色到底部的纯蓝，教室只作为浮雕——亮处取渐变里更靠上的颜色，暗处取更靠下的。

COMMU 手里的牌是自己设计的一副塔罗（`Art/P3R/extras.py` 从一张生成图切出 `T_P3R_Tarot_0..5` 和牌背）：进页面时先亮牌背再翻到所选羁绊，换选择时绕竖轴做 3D 翻转，落定时闪一下并有一道斜光扫过；握牌的手指同样抠成前景层（`T_P3R_HeroCardFront`），压在牌上。

早期稿 A 背后是暂停时的街景（`T_P3R_NightStreet`），清晰不糊；上面两块不是玻璃，而是平面多边形的高斯模糊：`BackgroundBlur` 铺满全屏，但强度取自多边形遮罩贴图（里面 1、外面 0，`T_P3R_BoardMask`/`SideMask`），白雾是同形状的 `Shape`，墨稿只留黑线、放在带贴图遮罩的组里。不能直接给容器加贴图遮罩：那样子树会画到离屏图上，背景模糊读不到街景，只剩一片灰。入场同理只滑入、不淡入。早期稿 B 的小图是 `Art/P3R/mosaic.py` 从街景和主角图裁出来的浅景深特写（光斑、虚掉的立面、地砖、镜头前的灰色、暖色近景、墨稿侧脸、青光里的人物），蓝色三角区保持干净。

## 无头资产管线（不用开编辑器）

```powershell
python Art/P3R/prepare_art.py                 # 生成图 → Art/P3R/Final（抠图、裁 16:9、主角卡通化、拼贴小图、气泡/碎片贴图）
python Art/P3R/Fonts/make_fonts.py            # 从引擎自带 Roboto 做窄体字重（P3R-*.ttf）
Tools/run-commandlet.ps1 -Run P3RSetup        # 导入贴图、建字体资产、建地图
Tools/P3R/shaders.ps1                         # 编 DreamShader 材质
# DreamFX：pwsh Plugins/DreamFX/.skill/dfx.ps1 build DFX/P3R/NS_P3R_Bubbles.dfs -Force（Shards 同理）
Tools/run-commandlet.ps1 -Run DreamUICompileWidgets -Args '-Manifest=Tools/P3R/widgets.json'   # 编全部 .dui
Tools/run-commandlet.ps1 -Run ScrubImportPaths -Args '-Path=/Game/P3R'   # 提交前去掉资产里的本机绝对路径
Tools/P3R/record.ps1 -Name tour -Seconds 28 -Keys '1.5:Tab,4.0:Down,5.6:Enter' -Stills '3.5,7'  # 离屏录制
python Tools/P3R/compare.py tour               # 参考图与录像静帧并排 → Saved/Takes/compare.png（参考图在 Plan/P3R/ref，不入库）
```

`Tools/P3R/rebuild.ps1` 把上面整条跑一遍（出图 → 编 C++ → 导入 → 编界面 → 去路径 → 录 → 对照）。

按键脚本也能驱动鼠标：`时间:Move@X;Y` 用 0.15 秒把指针移到设计坐标（1920×1080，左上为原点），`时间:Click@X;Y` 移过去并点击，例如 `2.6:Move@968;404,5.0:Click@984;634`。

## 字体

- 拉丁字母：`Art/P3R/Fonts/P3R-*.ttf` 是引擎自带 Roboto（Apache 2.0）改窄的版本，宽度对齐当初排版用的窄体，许可证见同目录 `LICENSE-Roboto.txt`。
- 中文：Source Han Sans（SIL OFL 1.1，原样），见 `LICENSE-SourceHanSans.txt`。
- 字体资产只记字体文件的相对路径，距离场在运行时生成，所以这些字体文件要跟工程一起。

参考截图（`Plan/P3R/ref`）不入库。
