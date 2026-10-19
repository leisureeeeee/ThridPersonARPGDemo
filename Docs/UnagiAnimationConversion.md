# 雅角色动画导入结果与 MAX 转换

## 本次实际结果

已把资源包现有 `Content/039-雅/fbx/雅/1.fbx`～`11.fbx` 导入为 11 个 Animation Sequence（动画序列）。

- 内容浏览器目录：`/Game/Asset/Unagi/Animations/SourceFBX`。
- 名称：`AN_Unagi_01`～`AN_Unagi_11`，按源编号命名，尚未播放确认动作含义。
- 使用当前人物 `SK_Unagi` 关联的 `SKEL_Unagi`，沿用模型的 Interchange（交换导入）偏移变换，统一缩放为 100。
- 每段有 234 条骨骼轨道。时长、帧区间数和文件哈希见 `UnagiAnimationImport.json`。
- 原模型和骨架导入前后的磁盘 SHA-256 相同；未修改角色蓝图、地图、材质、输入或相机代码。
- 没有执行编译、自动化测试、PIE（编辑器内运行）或动画播放验证。成功导入不代表已确认动作语义、循环衔接或武器握持效果。

这些动画来自已有 FBX，**不代表 149 份 MAX 场景已经转换完成**。本机安装记录、常见目录及 D/E/G 盘搜索未找到 `3dsmax.exe` / `3dsmaxbatch.exe`；无法执行 Max 控制器求值和烘焙。没有安装商业软件、申请许可或把源场景上传到在线转换网站。

## 在编辑器查看

1. 打开 Content Browser（内容浏览器）的 `Asset/Unagi/Animations/SourceFBX`。
2. 双击 `AN_Unagi_01`，检查预览人物是否为 `SK_Unagi`，单击播放。预期人物能随时间轴运动；这一步尚未由本次任务执行。
3. 逐段观察并按实际动作重命名，留意比例、朝向、脚底、武器是否正确跟随。编号本身不能确定待机、跑步或攻击。
4. 动画序列是动作数据；角色要按移动速度自动切换动作，还需后续配置 Animation Blueprint（动画蓝图）和 Blend Space（混合空间），本次没有接入这部分行为。

若正在打开的编辑器尚未显示磁盘新资产，稍等内容目录扫描，或在保存个人工作后重新打开项目。本次没有关闭编辑器，也没有保存或覆盖其中未保存的资产。

## 导入原理与脚本

`Tools/Unagi/Import-UnagiAnimations.ps1` 启动独立的 `UnrealEditor-Cmd.exe`，仅为该进程启用 Python 与编辑器脚本插件，使用 Python 导入流程。不会改写项目插件配置，也不会触发项目编译。

`import_unagi_animations.py` 读取当前人物网格的骨架及保存的导入变换，构建仅导入动画的 Interchange（交换导入）流程。它关闭网格、材质、贴图、通用自定义属性曲线及骨架曲线元数据写入，只保存新动画，不保存现有模型或骨架。这里处理的是骨骼动作，未配置额外的表情曲线导入。

初次按默认 30 帧采样时，01、04、08 三段成功；其他八段的结束时间落在半帧上，UE 拒绝导入。脚本已改为 60 帧采样，八段随后成功，保留源时间范围，无需四舍五入截断。前述三段保留已成功的 30 帧版本；各动画独立保存采样率，可以共用同一骨架。

从项目根目录再次导入现有 FBX：

```powershell
./Tools/Unagi/Import-UnagiAnimations.ps1 -EngineRoot G:/UE_5.8
```

已存在的目标资产会跳过，绝不替换。每次生成独立的 `Saved/UnagiAnimationWork/<运行编号>/import-report.json` 和 `import.log`。只读资源、导入资产与读取导入元数据属于本次处理流程，未执行游戏功能测试。

## 待安装可用 Max 后继续

已准备 `Export-UnagiAnimations.ps1` 和 Max 原生 Python 脚本 `export_unagi_animations.py`。**Max 端尚未运行验证**，当前只生成了导出计划和明确的缺失依赖报告。

先导出一份待机动作，再在编辑器确认骨架、单位及武器效果：

```powershell
./Tools/Unagi/Export-UnagiAnimations.ps1 `
  -MaxBatch 'C:/Program Files/Autodesk/3ds Max 2026/3dsmaxbatch.exe' `
  -Scene Unagi_Idle -ImportIntoUE -EngineRoot G:/UE_5.8
```

上面的 Max 版本路径只是示例，需要替换为实际已安装、已授权且支持源场景的版本。脚本默认只处理一份待机，不会自动安装软件。确认单份导出和导入后，批量处理主目录动作：

```powershell
./Tools/Unagi/Export-UnagiAnimations.ps1 `
  -MaxBatch 'C:/Program Files/Autodesk/3ds Max 2026/3dsmaxbatch.exe' `
  -AllScenes -ImportIntoUE -EngineRoot G:/UE_5.8
```

- 默认选 `max/雅/MAX` 内 70 份主角色动作；排除人物基础场景以及属于其他骨架的 `Unagi_Ghost*`。
- 外层另有 70 份同名但内容不同的场景。加 `-IncludeAlternateScenes` 才纳入，共 140 份；输出分别位于 `Primary` 和 `Alternate`，不会相互覆盖。
- 导出 FBX 默认放在 `Saved/UnagiMaxExport`；UE 导入目标为 `Asset/Unagi/Animations/FromMax/Primary` 或 `Alternate`。
- 每个场景按自身时间轴范围和帧率逐帧烘焙。烘焙把 Biped（两足角色系统）等 Max 控制器的求值结果转换为 FBX 骨骼变换曲线，让 UE 能读取。
- 使用完整场景中的骨架与蒙皮网格导出，UE 阶段只取动画；不重命名骨骼、不拆武器、不覆盖原 MAX、不导入灯光或摄像机。缺失插件或外部参照会中止该场景导出，并记录失败。
- 不同于已有编号 FBX，MAX 导出的单位、骨架层级兼容性仍需确认；如果 UE 报骨架不兼容，查看报告后处理源骨架，不强行更新现有骨架参考姿势。

## 版本管理范围

本次仅新增动画资产、转换脚本、清单和说明。当前人物模型、骨架、材质及源 FBX 原先属于用户未跟踪文件，本次保留其状态，没有顺带提交。

因此，这 11 份动画在当前工作区依赖已有 `/Game/Asset/Unagi/SKEL_Unagi` 和预览用 `SK_Unagi`。仅拉取本次提交的其他工作区仍需要具备这套人物资产；本次提交不是完整角色资源包。脚本也要求当前人物模型、骨架和源文件存在。

## 官方资料

- [Epic：使用 FBX 导入动画](https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-animations-using-fbx-in-unreal-engine)
- [Autodesk：3ds Max Batch 的脚本入口和日志](https://help.autodesk.com/cloudhelp/2018/ENU/3DSMax-Batch/files/GUID-48A78515-C24B-4E46-AC5F-884FBCF40D59.htm)
- [Autodesk：FBX 导出脚本参数](https://help.autodesk.com/cloudhelp/2024/ENU/3DSMax-Interoperability/files/GUID-3242579C-E37D-4A7C-BFA5-F4A2F22653E7.htm)
