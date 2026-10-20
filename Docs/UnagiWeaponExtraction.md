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

## 在中文编辑器中查看和挂接

1. 在 Content Browser（内容浏览器）打开 `Asset/Unagi/Weapons`，双击一个 `SM_Unagi_WeaponXX`。检查外观、UV 与材质是否符合预期；当前任务未执行视觉预览。
2. 可先将它单独拖入关卡观察，Transform（变换）的 Scale（缩放）从 `1, 1, 1` 开始。模型约为上表尺寸，轴心应在模型中心附近。
3. 需要手持时，在角色蓝图添加 Static Mesh Component（静态网格体组件），选择该武器，让它成为人物 Mesh（网格体组件）的子组件。
4. 在 Parent Socket（父级插槽）选择实际创建的右手插槽，例如 `WeaponSocket`。调整组件的相对位置、旋转，使握柄对准手掌；不要把居中轴心理解为能零偏移握持。
5. 挂接时关闭 Simulate Physics（模拟物理）；按游戏设计配置碰撞。这次仅完成模型提取，没有生成战斗碰撞、装备切换或伤害逻辑。

**原人物中的武器仍然保留。** 如果人物模型本身已含武器，再添加独立武器会同时显示两份。后续要独立装备，需要另行处理人物中原有武器的显示或拆分；本次没有覆盖当前 `SK_Unagi`。

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
