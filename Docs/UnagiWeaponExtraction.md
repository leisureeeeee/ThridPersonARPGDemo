# 雅角色独立武器模型

已从 `Content/039-雅/fbx/雅/1.fbx` 提取 `Unagi_Weapon01`～`05` 五个网格对象，并生成可单独使用的 Static Mesh（静态网格体）和 FBX 文件。没有修改原人物、骨架、材质或源 FBX，也没有自动挂接武器。

## 输出位置

- UE 内容浏览器：`Asset/Unagi/Weapons/SM_Unagi_Weapon01`～`SM_Unagi_Weapon05`。
- 独立模型文件：`SourceAssets/Unagi/Weapons/Unagi_Weapon01.fbx`～`Unagi_Weapon05.fbx`。
- 数值、原始偏移和文件哈希：`Docs/UnagiWeaponExtraction.json`。

| 对象编号 | 顶点数 | 三角形数 | 最大尺寸（厘米，约） |
| --- | ---: | ---: | ---: |
| 01 | 350 | 205 | 111.0 |
| 02 | 2403 | 1296 | 84.7 |
| 03 | 654 | 330 | 95.9 |
| 04 | 665 | 544 | 34.3 |
| 05 | 138 | 72 | 27.2 |

这里按源对象分别提取，没有将五个对象合并，也未根据隐藏状态丢弃对象。它们可能是武器部件或不同状态的模型，尚未通过视觉预览确认装配关系，不能直接认定为五种完整武器。

## 实现方式与轴心

`Tools/Unagi/extract_unagi_weapons.py` 由 UE 独立 Python 命令行进程运行。Interchange（交换导入）先把源场景的网格按对象分开读取为静态网格，应用场景旋转和缩放；人物、脸、头发等临时网格只存在于该进程内存中，没有保存进项目。

五个武器采用与当前人物一致的 100 倍导入比例。脚本读取各网格的 Mesh Description（网格描述），只平移顶点到各自包围盒中心，保留形状、UV（纹理坐标）、法线和材质槽，再用 UE 原生 FBX 导出器生成独立文件。

源武器的中心在人物坐标系中约高 176～192 厘米；直接沿用人物原点会使手部挂接出现很大偏移。输出模型的中心现在均位于 `(0, 0, 0)`。这是**几何中心轴心，不是已校准的握柄轴心**，手持时仍需调整插槽或组件偏移。

平移不会改变模型尺寸、顶点间相对位置或 UV。旧中心偏移记录在 JSON 的 `center_removed_cm` 中：若需在同一父坐标系下恢复原场景相对位置，可以据此恢复平移；实际角色骨骼姿态与附着变换仍需单独处理。

输出 FBX 已按厘米导出，重新导入这些新文件时从 Scale（缩放）1 开始，不要再次套用原始人物源文件的 100 倍导入缩放。

## 材质与独立使用

UE 网格复用当前角色的武器材质 `/Game/Asset/Unagi/Material__28`。它继续使用现有贴图，没有更改材质参数或另外复制贴图。导出的 FBX 保留材质槽和 UV，**不是包含全部贴图的自包含资源包**。

现有 `Material__28` 和原角色贴图是用户之前的未跟踪资源，本次没有顺带提交。其他工作区使用这些 UE 网格时，需要同套材质和贴图，或自行替换材质；几何 FBX 可单独导入。新武器无需 Skeleton（骨架）资产，不包含人物蒙皮或骨骼动画；作为刚性武器挂接到角色骨骼插槽后，由父组件带动运动。

## 与原版动画配合：蒙皮复核后的修正

2026-10-20 进一步读取原始 `1.fbx` 的 Skin/Cluster（蒙皮/骨骼权重簇）连接，发现这些源武器并非全部为单骨刚性挂件。绑定明细见 `Docs/UnagiWeaponBindings.json`。

| 源对象 | 有权重的骨骼 | 对原版动画的意义 |
| --- | --- | --- |
| `Unagi_Weapon01` | `Bn_katana_hilt`、`Bn_katana_hilt2` | 两组顶点分别受不同骨骼驱动 |
| `Unagi_Weapon02` | `Bn_Weapon`、六根 `Bn_scabbardA/B_*` | 七根骨骼参与，部分顶点有混合权重 |
| `Unagi_Weapon03` | `Bn_katana_burst`、`Bn_katana_burst_eye` | 两根骨骼参与，部分顶点有混合权重 |
| `Unagi_Weapon04` | `Bn_katana_hilt` | 全部 665 个顶点权重为 1，可考虑按该骨骼作刚性挂接 |
| `Unagi_Weapon05` | `Bn_katana_burst01`～`04` | 四根骨骼参与，部分顶点有混合权重 |

此前将五个网格都提取为 Static Mesh（静态网格体），保留了外形、材质和纹理坐标，**没有保留这些蒙皮权重**。因此它们适用于刚性道具用途，不能保证完整复现原版武器动画。此前将所有武器统一建议挂到右手插槽，适用范围过宽；即使对单骨对象，也应优先检查原驱动骨骼和绑定偏移，而非默认手骨。

如果目标是保留原版动作，采用以下方案。第 1 项已于后续任务完成，第 2～4 项尚未实施：

1. 已制作只去除武器几何的人物副本 `SK_Unagi_NoWeapon`，保留原骨架、参考姿势、人物蒙皮，以及武器动作所需骨骼，详见 [无武器人物导出结果](UnagiNoWeaponExtraction.md)。
2. 从原始 FBX 另行导出保留原蒙皮和骨骼绑定的 Skeletal Mesh（骨骼网格体）武器，保留共同坐标系，不沿用这次居中静态网格的无蒙皮数据。
3. 让人物播放原动画，并按兼容的骨骼层级配置 Leader Pose（主姿势）共享骨骼变换；需要独立评估时再考虑 Copy Pose From Mesh（从网格复制姿势）。保留完整骨架名目还不够，必须确保人物组件实际评估武器需要的骨骼。
4. 逐段预览人物和武器：比较握持位置、刀鞘相关骨骼和局部变形。当前只确认绑定关系，没有播放确认这些骨骼在各段动作里具体如何运动，也没有确认武器变体的显隐切换逻辑。

人物去除武器几何本身不会删除动画数据，但武器若改成静态物体并挂到另一根骨骼，驱动方式就发生变化。保留原蒙皮和原骨骼驱动，是继续配合原版动画的关键。

参考：[Epic：模块化角色及姿势共享的骨骼层级限制](https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-modular-characters-in-unreal-engine)。

## 在中文编辑器中查看和挂接

以下是刚性静态道具的查看与挂接方法，不代表完整还原原版多骨骼武器动画；要保留原动画，采用上一节的骨骼网格体方案。

1. 在 Content Browser（内容浏览器）打开 `Asset/Unagi/Weapons`，双击一个 `SM_Unagi_WeaponXX`。检查外观、UV 与材质是否符合预期；当前任务未执行视觉预览。
2. 可先将它单独拖入关卡观察，Transform（变换）的 Scale（缩放）从 `1, 1, 1` 开始。模型约为上表尺寸，轴心应在模型中心附近。
3. 需要手持时，在角色蓝图添加 Static Mesh Component（静态网格体组件），选择该武器，让它成为人物 Mesh（网格体组件）的子组件。
4. 在 Parent Socket（父级插槽）选择用途对应的骨骼插槽；复现源单骨对象时，先检查其原驱动骨骼。调整组件的相对位置、旋转，使目标部件对齐；不要把居中轴心理解为能零偏移握持。
5. 挂接时关闭 Simulate Physics（模拟物理）；按游戏设计配置碰撞。这次仅完成模型提取，没有生成战斗碰撞、装备切换或伤害逻辑。

**原人物 `SK_Unagi` 中的武器仍然保留。** 如果该人物本身已含武器，再添加独立武器会同时显示两份。后续任务已另外创建不带武器几何的 `SK_Unagi_NoWeapon`，可在准备独立装备时使用；原 `SK_Unagi` 没有覆盖，也没有自动替换当前受控角色。

## 实际核对与限制

- 五份导出 FBX 均只有一个 Mesh（网格）模型、一个几何对象，没有骨骼及 Deformer（变形器）；都有 UV、法线、切线和材质层。
- 导出几何的包围盒中心均为零，尺寸与提取结果一致。
- 从磁盘重新加载五个 UE 资产，均为 StaticMesh，顶点数、三角形数和居中位置正确；材质和独立 FBX 的重新导入路径已保存。
- 原 `1.fbx`、`SK_Unagi`、`SKEL_Unagi`、`Material__28` 的磁盘 SHA-256 在处理前后保持一致。
- 未执行编译、自动化测试、PIE（编辑器内运行）、视觉预览或手部挂接验证。

## 脚本入口

以下命令使用当前机器的引擎路径。脚本针对本项目既有源目录与材质；若输出已经存在，会停止而不是覆盖。

```powershell
& 'G:/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
  'G:/UEProject/ThirdPersonARPG/ThirdPersonARPG.uproject' `
  -run=pythonscript `
  -script='G:/UEProject/ThirdPersonARPG/Tools/Unagi/extract_unagi_weapons.py' `
  '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
  -unattended -nop4 -nosplash -NullRHI -nosound -NoLiveCoding
```

完整运行记录位于 `Saved/UnagiWeaponWork`，临时数据和引擎缓存不纳入 Git。
