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

这里的操作入口在 **UE 编辑器的内容浏览器**，不是 Windows 资源管理器里右键 `1.fbx`。

1. 在 UE 按 `Ctrl+Space` 打开内容抽屉，进入用于存放新角色资产的目标目录。
2. 点击内容浏览器工具栏的 **Import / 导入**，在弹出的文件选择器里选择 `G:/UEProject/ThirdPersonARPG/Content/039-雅/fbx/雅/1.fbx`，再点打开。
3. 接下来出现的才是导入设置窗口。旧版界面勾选 **Skeletal Mesh / 骨骼网格体**；新版 Interchange 界面在相应分类中启用 **Import Skeletal Meshes**。这是指定模型的导入类型，不是右键文件时选的菜单。
4. 在该窗口找到 **Skeleton / 骨架**资源选择框，保持 **None / 无**，不要选择原来的 `UE4_Mannequin_Skeleton`。这里留空表示不复用已有 Skeleton；UE 仍会读取 FBX 自带的骨骼，并生成新的 Skeleton 资产，不表示导入一个没有骨骼的模型。
5. 首次先关闭 **Import Animations / 导入动画**，再点击窗口的 Import / 导入按钮。下文继续说明合并模型、Morph Targets、比例和材质。

Skeletal Mesh 是人物表面网格及蒙皮，Skeleton 是骨骼名称和层级；两者通常会作为相关联但不同的 UE 资产生成。UE 5.8 的新旧导入窗口布局不同，上述选项可能分组显示；本次没有看到用户当前窗口，不假定某个选项一定在固定位置。

资源设置顺序：

1. 在内容浏览器新建 `/Game/Characters/Unagi`，建议按 Mesh、Materials、Textures、Animations 分目录。这里是建议的新目录，尚未创建。
2. 先导入 `Content/039-雅/fbx/雅/1.fbx`。选它是为了获得主体模型和骨架，不表示它的动画就是待机。
3. 导入为 **Skeletal Mesh**，**Skeleton 留空/None** 以建立新骨架。先关闭动画导入，避免将未知动作当作默认站姿；不勾选用第 0 帧替换参考姿势。导入成功后可命名为 `SK_Unagi`、`SKEL_Unagi`。
4. UE 5.8 Interchange 的 **Combine Skeletal Meshes** 可选 **Combine All Visible That Use Same Skeleton**。本机引擎源码确认存在此选项，可将同骨架的可见身体、脸、头发等合并为一个可赋给 Character Mesh 的资产。若使用其他导入界面，合并选项和最终生成资产数量应以该界面为准。
5. 若需要面部表情，开启 Import Morph Targets；源文件含 40 个 Shape，实际生成数量以导入结果为准。Physics Asset 可由导入器生成，再按人物尺寸调整。
6. 源文件含 `Unagi_Weapon01`～`Unagi_Weapon05`；至少 03、05 的源节点标记为隐藏。若导入后出现多套武器重叠，检查隐藏节点是否被导入。角色外观初期可仅保留 Body、Face、Hair；后续若武器需要独立装备，应在 DCC 中拆出武器再导出，而不是在 C++ 中混入资源路径。

**比例先确认：** 文件声明 UnitScaleFactor=1（厘米），但原始几何高度范围约 0～2，主要骨骼距离也在约 1 个单位以内，未见根节点 100 倍缩放。这提示模型可能小约 100 倍，尚未在 UE 中确认最终导入尺寸。先看导入预览/资产尺寸：若只有约 2 cm，统一以导入缩放 100 重新导入，使高度回到人物量级；若已是正常人物尺寸，则不要再乘 100。后续动画使用与模型相同的坐标、单位和导入变换，不分别随意缩放。

## 材质与贴图搭配

**有两个操作位置：** 在骨骼网格体编辑器里指定“每个部位用哪个材质”，在材质编辑器里配置“材质使用哪些贴图和参数”。在内容浏览器双击导入后的 Skeletal Mesh（例如 SK_Unagi），在 Asset Details / 资产详情中找到 Material Slots / 材质槽，把身体、头发、脸、武器对应的 Material 或 Material Instance 赋给槽位；不要把 Skeleton 资产当成编辑材质的入口，也不要将 TGA 贴图直接作为材质槽的材质。

如果对应材质尚未配置，双击该材质打开 Material Editor，配置颜色、法线等贴图连接；如果它是 Material Instance，则打开实例调整已暴露的贴图参数。完成后保存材质和模型。角色蓝图 BP_Character → Mesh → Materials 也可为该组件覆盖材质；如果模型预览正确但角色仍显示旧材质，要检查这里是否残留旧覆盖。模型资产的材质槽定义默认外观，蓝图组件的设置作用于该角色组件。

### 以身体材质为例连接贴图

1. 先通过内容浏览器把 `fbx/雅/TEX` 中的 TGA 导入为 UE 纹理资产。然后双击身体槽对应的材质，打开 Material Editor（材质编辑器）。若导入后尚未生成身体材质，可在内容浏览器空白处右键，新建 Material（材质），例如命名 `M_Unagi_Body`。
2. 将内容浏览器中的 `Unagi_Body_D` 和 `Unagi_Body_N` 拖入材质编辑器中间的节点画布，会各生成一个 Texture Sample（纹理采样）节点。拖入的是已经导入的 UE 纹理，不是直接把磁盘里的 TGA 拖到输入引脚上。
3. 找到右侧名字与材质相同的主输出节点。把 `Unagi_Body_D` 节点的 RGB（红绿蓝颜色）输出圆点，拖线接到主节点 Base Color（基础颜色）输入圆点。
4. 把 `Unagi_Body_N` 节点的 RGB（红绿蓝向量）输出接到 Normal（法线）。不要用 A（透明度通道）或单独的 R/G/B（红/绿/蓝通道）代替完整法线向量。
5. 双击内容浏览器中的 `Unagi_Body_N` 打开纹理编辑器，检查 Compression Settings（压缩设置）为 Normalmap（法线贴图），sRGB（颜色空间选项）关闭；颜色贴图 `Unagi_Body_D` 的 sRGB（颜色空间选项）保持开启。若法线采样节点报类型不匹配，检查 Sampler Type（采样器类型）是否为 Normal（法线）。
6. 暂时不连接 `Unagi_Body_M`，也先不调整复杂卡通着色。点击 Apply（应用）和保存，再回到 Skeletal Mesh（骨骼网格体）的 Material Slots（材质槽），将身体槽指定为这个身体材质。可先确认身体出现颜色和表面细节，再处理头发、脸与武器。

最小连线关系为：`Unagi_Body_D.RGB → Base Color（基础颜色）`；`Unagi_Body_N.RGB → Normal（法线）`。颜色图提供表面颜色，法线图改变受光方向来表现细节，不改变实际骨骼或网格形状。头发和武器替换为对应前缀的 D/N 纹理，脸目前只有 D 纹理，法线输入可暂时不接。

**主节点输入不同的情况：** 本项目启用了 Substrate（新版材质系统），但 UE 5.8 仍允许传统输入形式，不能仅凭项目设置断定当前材质没有颜色和法线输入，也不需要主动转换已有材质。若当前根节点只有 Front Material（正面材质）这类表面入口，则先找到已连接的 Substrate Slab BSDF（单层表面材质节点），把颜色接它的 Diffuse Albedo（漫反射反照率）、法线接它的 Normal（法线）；该表面节点的输出连接根节点 Front Material（正面材质）。若尚无表面节点，可在画布右键搜索其英文名称添加。当前具体材质节点图尚未查看，这里按实际看到的输入区分两种接法，不要求用户修改全项目材质设置。

如果打开的是参数列表而没有连线画布，则可能是 Material Instance（材质实例）；此时在已有的纹理参数中指定 Body_D / Body_N，或者打开 Parent（父材质）查看其参数定义。只有已暴露的参数才能在实例中更改。

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
