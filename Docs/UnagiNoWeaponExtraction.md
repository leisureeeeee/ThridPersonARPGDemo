# 雅角色：不带武器的人物副本

已从原 `Content/039-雅/fbx/雅/1.fbx` 导出人物副本，并导入 UE。五个 `Unagi_Weapon01`～`05` 的几何及其蒙皮簇已移除，人物保留身体、脸、头发。原 `SK_Unagi`、骨架、材质、动画和源 FBX 未覆盖。

## 输出

- 独立 FBX：`SourceAssets/Unagi/Character/Unagi_NoWeapon.fbx`。
- UE Skeletal Mesh（骨骼网格体）：`/Game/Asset/Unagi/SK_Unagi_NoWeapon`。
- 共用 Skeleton（骨架）：原 `/Game/Asset/Unagi/SKEL_Unagi`。
- 提取事实及哈希：`Docs/UnagiNoWeaponExtraction.json`。

## 保留了什么

| 源网格 | 顶点数 | 源多边形数（未统一三角化） |
| --- | ---: | ---: |
| `Unagi_Hair` | 2705 | 5302 |
| `Unagi_Body` | 8258 | 15788 |
| `Unagi_Face` | 4026 | 2956 |

人物的顶点、纹理坐标、法线、蒙皮权重、绑定矩阵及表情数据保留原始 FBX 属性字节，重新序列化后逐对象比较一致。保留全部 234 个 Model（模型变换节点），包括 206 个 LimbNode（骨骼节点）及原武器/剑鞘相关骨骼；五个原武器网格节点变为 Null（空变换节点），不再挂接任何几何。因此在骨架树中仍看到武器名称是预期现象，不代表武器网格还存在。

UE 导入结果保留 234 根骨骼，与原网格的骨骼名称、顺序和父子关系一致；保留 40 个 Morph Target（变形目标）。人物只使用头发、身体、脸三种材质槽，分别复用 `Material__25`、`Material__27`、`Material__26`，没有武器材质槽。

没有删除原动画，独立 FBX 也保留源文件中已有的动画对象，未把它们当成新的动作转换成果。UE 本次仅导入网格，关闭动画导入。蒙皮通过骨骼变换驱动人物顶点；保留原骨架和人物权重，使原人体动作具备继续复用的基础，但本次没有逐段播放验证。

## 比例、材质与物理

- FBX 保持原始单位和坐标，不额外烘焙缩放。在本项目导入它时采用与原人物相同的 Import Uniform Scale（导入统一缩放）100；已生成的 UE 网格直接使用组件缩放 `1/1/1`。不要把先前独立武器 FBX 的「导入缩放 1」规则套用到这份保持源数据的人物 FBX。
- 新人物导入包围盒约为 `114.0 × 47.8 × 172.2` 厘米。原模型包围盒包括悬空或伸出的武器，因此去除武器后包围盒缩小不代表缩放不一致。
- FBX 没有嵌入完整贴图包。UE 网格复用本机原材质和贴图；当前原骨架和材质是用户已有未跟踪文件，本次没有擅自提交这些依赖。其他工作区需具备同套依赖，或重新导入独立 FBX 并配置材质。
- 新网格暂不指定 Physics Asset（物理资产），避免沿用可能含武器碰撞体的旧资源。角色胶囊移动与此独立；需要布娃娃或骨骼物理时，应另行创建/配置适合人物主体的物理资产。

## 在编辑器中使用

1. 在 Content Browser（内容浏览器）进入 `Asset/Unagi`，找到并双击 `SK_Unagi_NoWeapon`，检查身体、脸和头发，预期没有剑、剑鞘及其他四份武器状态几何。
2. 若编辑器尚未识别外部进程新建的文件，可先刷新该目录；仍未出现时，保存自己的工作后重新打开项目。不要覆盖原角色资产。
3. 准备替换时，在角色蓝图选择 Mesh（网格体组件），将 Skeletal Mesh Asset（骨骼网格体资产）设置为 `SK_Unagi_NoWeapon`。本次没有替用户改动蓝图、关卡或默认角色。
4. 保持现有人物对齐方式及组件缩放，从材质槽重新核对覆盖值。新模型是三槽，角色组件若保留旧的四槽材料覆盖，需要按新槽位调整。
5. 在动画序列中选择新人物作为 Preview Mesh（预览网格体），逐段观察人体动作、头发和表情。后续独立武器仍需按原多骨蒙皮或合适的刚性部件方案处理，详见 [原武器绑定限制](UnagiWeaponExtraction.md)。

以上为用户操作步骤，本次未运行 PIE（编辑器内运行）、自动化或功能测试，也未编译 C++。

## 实际资源检查与限制

- 独立 FBX 重读后仅有身体、脸和头发三个基础网格；原五个武器几何及对应的 Skin/Cluster（蒙皮/权重簇）已删除，人体及其表情/蒙皮对象内容一致。
- UE 导入成功，只有一个新增 SkeletalMesh（骨骼网格体），没有新建骨架、动画或材质；原角色、骨架、四个材质、物理资产和源 FBX 的磁盘 SHA-256 前后相同。
- 另起只读 UE 进程从磁盘重新加载新网格，确认骨架引用、234 根骨骼及父子关系、40 个变形目标、三个原材质和独立 FBX 重导入路径均已保存；该进程退出 0，0 error(s)、0 warning(s)。
- 导入进程退出 0，0 error(s)、3 warning(s)。三条警告为源网格缺少平滑组；日志还提示部分切线计算出现零长度法线。没有借此改写源法线或强行平滑，外观受光及动画效果待编辑器预览确认。
- 日志、临时脚本和进程报告位于忽略的 `Saved/UnagiNoWeaponWork` 及 `Saved/UnagiNoWeapon*.log`。

## 可复用脚本

先在项目根目录运行：

```powershell
python Tools/Unagi/export_unagi_no_weapon.py
```

该命令与 UE 导入脚本都拒绝覆盖已存在的输出。`fbx_binary.py` 只负责 FBX 二进制节点的读写并保留属性原始字节；`export_unagi_no_weapon.py` 决定移除对象并检查数据完整性；`import_unagi_no_weapon.py` 在 UE 中建立新网格并复用材质/骨架，职责独立。需要重新制作时，先选择新的输出位置并协调 UE 目标名称，而不是直接覆盖当前成果。

UE 脚本调用方式：

```powershell
& 'G:/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
  'G:/UEProject/ThirdPersonARPG/ThirdPersonARPG.uproject' `
  -run=pythonscript `
  -script='G:/UEProject/ThirdPersonARPG/Tools/Unagi/import_unagi_no_weapon.py' `
  '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
  -unattended -nop4 -nosplash -NullRHI -nosound -NoLiveCoding
```
