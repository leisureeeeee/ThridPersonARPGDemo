# 雅重定向后衣物展开的排查

2026-10-20。用户确认雅已能播放人体动作，并提供源人偶与雅的重定向截图。截图中雅的青色大块衣物向外支起。这个静态表现与衣物专用骨骼缺少适配相符；不能仅凭一帧确认所有异常都由重力或某一根骨骼引起。

## 已确认的数据

只读加载当前已保存的 `/Game/Asset/Unagi/SK_Unagi_NoWeapon` 得到：

| 项目 | 结果 |
| --- | --- |
| Skeleton（骨架） | `/Game/Asset/Unagi/SKEL_Unagi_NoWeapon` |
| 骨骼数量 | 234 |
| Physics Asset（物理资产） | 无 |
| Mesh Clothing Assets（网格体布料资产） | 0 个 |
| Post Process Anim Blueprint（后处理动画蓝图） | 无 |

这说明当前网格没有配置网格体布料数据或后处理骨骼物理。不能把“模型带有衣服”和“已经实现受重力驱动的衣服”视为同一件事，也不能据此排除某个角色实例另有独立组件或预览覆盖配置。

从模型骨骼和源 `SourceAssets/Unagi/Character/Unagi_NoWeapon.fbx` 的 Skin/Cluster（蒙皮/权重簇）连接中进一步确认：

- `Unagi_Body` 确实有受 `Furisode*`、`MaJia*`、`Bn_SleeveUP`、`SleeveDown001`、`Bone015` 影响的顶点，不只是存在这些骨骼名称。
- 例如 `Furisode1_02` 影响 338 个正权重身体顶点，`Furisode3_02` 影响 324 个，`MaJia1_02` 影响 226 个；这些数量不能直接相加为独立顶点总数，因为同一顶点可以有多根骨骼影响。
- `Bn_SleeveUP` 的父骨骼是 `Bip001-R-UpperArm`，`Furisode1_01` 和 `Furisode2_01` 挂在它下面。
- `Furisode3_01` 的父骨骼是 `Bone015`，`Bone015` 又挂在 `Bip001` 下，并非直接挂在手臂。需要核对该支点在原动作中的驱动方式，不能直接认定应改父级或删除。
- `Furisode_B_01_ROOT` 和多条 `MaJia*` 链位于 `Bip001-Spine2` 下。

之前新建的两份 IK Rig（IK绑定）只有人体 9 条运动链，没有添加上述衣物骨骼链或衣物动态。骨骼未被单独驱动时仍可继承父级运动，但不会自动生成原素材的专门局部动作。源 UE4 人偶没有雅的这些衣物骨骼，仅在目标绑定中新建同名链，也不能凭空得到源衣物动作。

## 后续处理顺序

1. 先核对人体重定向姿势与衣物支点。观察手臂弯曲时衣袖根部是否跟随、衣摆是否与躯干保持合理关系；区分静止姿势不合适、缺失原动画控制/约束和骨骼动态尚未添加。不要直接改变既有骨架层级或蒙皮。
2. 为衣物建立合适的基础姿势和跟随。必要时使用 Control Rig（控制绑定）或骨骼控制处理衣袖根部与静止姿态，再处理骨骼动态。
3. 对已有多段骨骼链，可以在 Animation Blueprint（动画蓝图）或后处理动画蓝图中配置 AnimDynamics（动画动力学）等骨骼动态，逐链设置限制及碰撞需求；这不是仅在普通动画序列中勾选重力就能获得的功能。
4. 若希望宽大衣袖具有更细致的布面弯折与接触表现，可以采用 Chaos Cloth（混沌布料），需要配置布料区域、固定边界、Max Distance（最大位移距离）、碰撞和材质参数。启用物理前仍需保证基础姿势和约束位置合理。

骨骼动态或布料模拟不是本次已实现内容。参考姿势、衣物支点与约束尚未经实际编辑器操作确认；具体方案需要以用户后续要求和实际动态效果决定。本次未添加布料、创建物理资产、修改动画蓝图或更改骨架。

## 检查方式与限制

Windows `computer-use` 工具的实时窗口连接失败，错误为 native pipe 不存在；按技能要求重试并重置后仍失败，没有操作用户窗口。最终画面判断来自用户新发的截图，已按 see 技能运行并读取结果；本地 OCR 没有识别到文字，不能替代动态视觉验证。

通过独立 UE Python 进程只读加载网格元数据，退出 0，0 errors / 0 warnings（零错误/零警告）。网格、新骨架和重定向器三份已有文件的 SHA-256 前后相同。另以本地 FBX 解析器只读核对蒙皮连接。过程报告位于被忽略的 `Saved/UnagiClothingInspection`，没有编译或执行播放、自动化、PIE（在编辑器中运行）、功能测试。

参考：[Epic AnimDynamics（动画动力学）](https://dev.epicgames.com/documentation/unreal-engine/animation-blueprint-animdynamics-in-unreal-engine?lang=en-US)、[Epic 布料工具属性](https://dev.epicgames.com/documentation/en-us/unreal-engine/clothing-tool-in-unreal-engine---properties-reference)。
