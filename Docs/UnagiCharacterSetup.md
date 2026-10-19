# 039-雅：资源对应与角色替换

2026-10-19 静态检查结果。当前 `Content/039-雅` 是源文件目录，没有 UE `.uasset`；需要通过 UE 内容浏览器导入，放入 Content 并不等于已完成资产导入。本次没有导入、修改或运行资产。

## 实际文件内容

| 文件 | 已读取到的内容 | 用途 |
| --- | --- | --- |
| `fbx/雅/1.fbx`～`11.fbx` | 每份都包含 8 个基础 Mesh、206 个 LimbNode、40 个表情 Shape 和 1 段动画 | 先选一份创建角色 Mesh/Skeleton，之后其他文件只导入动画 |
| `fbx/雅/TEX/*.tga` | 10 张 2048×2048 贴图 | 身体、脸、头发、武器材质 |
| `max/雅/MAX/Unagi.max` | 3ds Max 源场景，未打开场景内容 | 如需整理模型，可在 3ds Max 中打开后导出 FBX |
| `Unagi_Idle.max`、`Unagi_Walk.max`、`Unagi_Run.max` 及其他命名动作 | 3ds Max 动画源文件，未打开场景内容 | 需要对应动作时在 Max 导出带骨骼动画的 FBX；UE 不直接读取 `.max` |
| `max/雅/MAX/Unagi_Ghost.FBX` | 1 个 Mesh、2 个 LimbNode、1 段约 1 秒的动画 | 独立小网格，不能作为完整人物主体；具体外观用途尚未预览 |

编号 FBX 的骨骼名称列表一致，但这不代替 UE 导入时对完整骨骼层级和参考姿势的兼容检查。主体骨骼是 `Bip001 Pelvis`、`Bip001 L Thigh` 等，自带额外头发、服装、面部及武器相关节点，不能直接当作 UE4 Mannequin 的 Skeleton。

编号 FBX 的动画名称全部为 `Take 001`，不能按编号认定具体动作。读取到的导出时长如下，语义需要导入后预览：

| FBX | 时长（秒，约） |
| --- | --- |
| 1 | 3.333 |
| 2 | 0.650 |
| 3 | 3.317 |
| 4 | 3.233 |
| 5 | 0.750 |
| 6 | 0.883 |
| 7 | 0.450 |
| 8 | 0.767 |
| 9 | 3.883 |
| 10 | 1.883 |
| 11 | 0.850 |

## 先建立一个可替换的角色模型

1. 在内容浏览器新建 `/Game/Characters/Unagi`，建议按 Mesh、Materials、Textures、Animations 分目录。这里是建议的新目录，尚未创建。
2. 先导入 `Content/039-雅/fbx/雅/1.fbx`。选它是为了获得主体模型和骨架，不表示它的动画就是待机。
3. 导入为 **Skeletal Mesh**，**Skeleton 留空/None** 以建立新骨架。先关闭动画导入，避免将未知动作当作默认站姿；不勾选用第 0 帧替换参考姿势。导入成功后可命名为 `SK_Unagi`、`SKEL_Unagi`。
4. UE 5.8 Interchange 的 **Combine Skeletal Meshes** 可选 **Combine All Visible That Use Same Skeleton**。本机引擎源码确认存在此选项，可将同骨架的可见身体、脸、头发等合并为一个可赋给 Character Mesh 的资产。若使用其他导入界面，合并选项和最终生成资产数量应以该界面为准。
5. 若需要面部表情，开启 Import Morph Targets；源文件含 40 个 Shape，实际生成数量以导入结果为准。Physics Asset 可由导入器生成，再按人物尺寸调整。
6. 源文件含 `Unagi_Weapon01`～`Unagi_Weapon05`；至少 03、05 的源节点标记为隐藏。若导入后出现多套武器重叠，检查隐藏节点是否被导入。角色外观初期可仅保留 Body、Face、Hair；后续若武器需要独立装备，应在 DCC 中拆出武器再导出，而不是在 C++ 中混入资源路径。

**比例先确认：** 文件声明 UnitScaleFactor=1（厘米），但原始几何高度范围约 0～2，主要骨骼距离也在约 1 个单位以内，未见根节点 100 倍缩放。这提示模型可能小约 100 倍，尚未在 UE 中确认最终导入尺寸。先看导入预览/资产尺寸：若只有约 2 cm，统一以导入缩放 100 重新导入，使高度回到人物量级；若已是正常人物尺寸，则不要再乘 100。后续动画使用与模型相同的坐标、单位和导入变换，不分别随意缩放。

## 材质与贴图搭配

FBX 中的材质绑定和 Diffuse 引用已读取确认：

| 部位 | FBX 材质名 | 基础颜色 | 法线 |
| --- | --- | --- | --- |
| 头发 `Unagi_Hair` | `Material #25` | `Unagi_Hair_D.tga` | `Unagi_Hair_N.tga` |
| 身体 `Unagi_Body` | `Material #27` | `Unagi_Body_D.tga` | `Unagi_Body_N.tga` |
| 脸 `Unagi_Face` | `Material #26` | `Unagi_Face_D.tga` | 此目录没有单独的 Face_N |
| 武器 `Unagi_Weapon01`～`05` | `Material #28` | `Unagi_Weapon_D.tga` | `Unagi_Weapon_N.tga` |

- `_D` 在本批 FBX 中明确接到 DiffuseColor，可先接材质 Base Color。
- `_N` 按法线贴图导入，使用 Normalmap 压缩并关闭 sRGB，连接 Normal；如出现凹凸反向，再核对法线绿通道约定。
- `_M` 有 Body、Hair、Weapon 三张。源 FBX 没有它们的材质通道连接信息，因此不能仅凭后缀断言它们是金属度或 ORM。先不接，待检查各通道和源材质定义后再使用；若确认作为数据遮罩使用，再关闭 sRGB。
- 先用简单材质确认颜色和部位；原作卡通光照、脸部阴影、头发高光不会仅靠默认 PBR 材质自动还原。
- 贴图分别出现在 `fbx/雅/TEX`、`max/雅/TEX`、`max/雅/MAX`，同名文件 SHA256 相同，仅导入第一份即可。
- FBX 引用的是原作者 `W:/BaiduNetdiskDownload/...` 的绝对路径，本机可能无法自动找到。手动从 `fbx/雅/TEX` 导入后，按上表赋给对应材质槽；不要依赖材质槽的数字顺序。

## 替换当前受控角色

打开当前使用的 `BP_Character`，选中继承的 **Mesh（CharacterMesh0）**，把 **Skeletal Mesh Asset** 改为导入的 `SK_Unagi`，并检查材质槽。替换的是带蒙皮的 Skeletal Mesh，不是把 Skeleton 资产拖到关卡，也不是替换 Capsule。

- 模型配置留在蓝图中，`MyCharacter` 继续提供角色移动、胶囊、弹簧臂和相机；`MyPlayerController` 继续处理 WASD 和鼠标，无需为换外观修改这些 C++ 类。
- 当前父类为 Mannequin 设置了 Mesh Yaw=-90、Z=-胶囊半高。新模型的朝向、根骨位置和尺寸不同，不能保证沿用即可：在蓝图视口让人物正面沿胶囊前向箭头，脚底对齐胶囊底部，胶囊包住身体；避免把头发或刀尖作为胶囊高度依据。
- 如果关卡放置了自动控制的 `BP_Character`，修改这个蓝图即可。如果由 GameMode 出生，则其 Default Pawn Class 应指向这个角色蓝图；当前原生 ARPGGameMode 默认是 `AMyCharacter`，它本身不指定角色资源。
- 此时可以替换外观，但没有匹配的动画驱动时仍可能是参考姿势。Mesh、Skeleton、动画和 AnimBlueprint 是不同职责，换 Mesh 不会自动把旧动画转换成新骨架。

## 动画搭配的两条路线

**用雅自带动作：** 模型建立后，再从编号 FBX 导入动画，关闭 Import Mesh、选择 `SKEL_Unagi`、开启 Import Animations，避免重复生成 11 份模型和骨架。逐个预览后按实际动作重命名；如果编号文件没有待机/走/跑，就用 Max 打开对应 `Unagi_Idle.max`、`Unagi_Walk.max`、`Unagi_Run.max`，以相同骨骼层级导出 FBX、烘焙动画并使用一致单位。为 `SKEL_Unagi` 创建 `ABP_Unagi`，通过速度驱动 Idle/Walk/Run 的 BlendSpace/状态机，最后赋给角色 Mesh 的 Anim Class。没有 Max 时，需要可用的已导出 FBX 或走重定向路线。

**继续用现有武士动画：** 为原 Mannequin 和新 Unagi 分别建立 IK Rig，在 IK Retargeter 中对应骨盆、脊柱、手臂、腿等链，调整匹配姿势后导出面向 Unagi 的动画，再由匹配的新 AnimBlueprint 使用。不要直接强行选择旧 Mannequin Skeleton，也不要仅把旧 AnimBlueprint 赋给不兼容的新骨架。额外头发、衣服和面部骨骼的驱动需要单独处理，普通人体动画重定向不会自动复现这些效果。

## 官方依据

- [FBX 导入选项](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-import-options-reference-in-unreal-engine)：新 Skeleton、动画单独导入、Morph Targets、参考姿势等选项。
- [导入骨骼模型](https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-skeletal-meshes-using-fbx-in-unreal-engine)：DCC 导出与 UE 内容浏览器导入流程。
- [IK Rig 动画重定向](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)：在不同骨骼名称和结构间转移动画。

检查范围：只读取了文件目录、FBX 元数据/几何范围/连接、TGA 文件头与哈希及项目源码；没有在 3ds Max 或 UE 中打开/导入模型，没有验证外观、动作语义、材质遮罩通道或最终比例。本说明中的导入步骤由用户操作验证。
