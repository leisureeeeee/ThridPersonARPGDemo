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

## 在 UE 5.8 中先修正静态衣物姿势

以下是操作指导，本次没有替用户执行或验证最终效果。

1. 打开 `Asset/Unagi/Rig/RTG_UE4_To_Unagi`，选择 Target（目标），确认当前编辑的是雅。具体位置在重定向器的 Hierarchy（层级）面板顶部：Source（源）/Target（目标）切换按钮下方就是 Current Retarget Pose（当前重定向姿势）下拉框；选择目标后，这一栏表示雅的目标姿势。它不在导出动画序列或 IK Rig 资产的详情中。
2. 在 Retarget Pose（重定向姿势）工具中选择 Create（创建）或 Duplicate Current（复制当前姿势），命名为 `Unagi_ClothesFix`；将它设为 Current Retarget Pose（当前重定向姿势）。保留已有姿势用于对照。
3. 在重定向器顶部工具栏靠左找到模式切换按钮：播放时显示 Running Retarget（正在运行重定向），点击一次后变成 Editing Retarget Pose（正在编辑重定向姿势）；如果已显示后者，就已在编辑模式。UE 5.8 这里是同一个按钮随状态改变文字，不必寻找始终叫 Edit Retarget Pose 的独立按钮。在 Hierarchy（层级）的过滤菜单中取消 Hide Bones Not in a Chain（隐藏不在链中的骨骼）和 Hide Bones Not Retargeted（隐藏未重定向的骨骼），让衣物专用骨骼可见。
4. 先查找并选择 `Bn_SleeveUP`，将鼠标放回人物视口并按默认快捷键 E，出现旋转圆环后拖动一个红/绿/蓝圆弧，小幅调整一个局部轴，观察它实际影响哪块衣物。一次约 5 度只是观察用的起点，不是确认修复值；方向不对就用 Ctrl+Z 撤销并换轴。若青色突出衣物没有变化，先撤销本次尝试，再定位 `Bone015` / `Furisode3_01`，不要累计修改无关骨骼。逐段核对 `Furisode1_01`、`Furisode2_01` 及其子骨骼。若界面存在姿势混合滑块，确认值为 1 以完整显示该姿势。不要直接照抄固定的 90 度角，当前尚未测定这些骨骼的局部轴与合适偏移。
5. 如果突出的部分没有变化，再逐个定位 `Bone015` / `Furisode3_01`、`Furisode_B_01_ROOT` / `Furisode_B_01` 和 `MaJia*` 链。通过选中骨骼和小幅旋转确认影响区域，只调整确认相关的衣物骨骼，不以改变人体脊柱或肩膀来掩盖衣物问题。
6. 退出姿势编辑，切回 Run Retarget（运行重定向），由用户观察待机、抬臂和走路动作。先确认衣袖根部跟随且衣物不大面积横穿身体，再导出新的动画副本；不要覆盖原动画来尝试修复。修改重定向器的姿势不会自动更新之前已导出的动画。

本机实现确认：旋转目标姿势选中骨骼不要求该骨骼已属于重定向链；控制器将旋转偏移记录在独立 Retarget Pose 中，重定向处理以目标重定向姿势作为输出初始姿势。因此这条路径可用于检查未映射衣物骨骼的基础姿态，但本次没有确认任何具体旋转角度或逐动作结果。

如果衣物在静止姿势正常、抬臂时又明显脱离手臂，应该转入 Control Rig（控制绑定）或其他骨骼控制，为相关支点建立保持适当偏移的跟随关系。例如 `Bone015` 当前挂在 `Bip001` 下，不能仅靠固定姿势期待它获得手臂的所有运动；也不能不经确认就改它的骨架父级。

基础姿势和支点正常后，才为 `Furisode*`、`MaJia*` 等适用链添加 AnimDynamics（动画动力学）及限制；它会用自己的简化刚体求解，并不依赖角色的 Physics Asset（物理资产），但也不会自动提供完整的衣物碰撞。若采用 RigidBody（刚体）或需要身体碰撞的布料方案，则另外配置相应物理/碰撞资产。宽袖细致布面效果可再采用 Chaos Cloth（混沌布料），不要把骨骼轻量动态等同于完整布料模拟。

参考：[重定向姿势与层级过滤](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)、[骨骼控制](https://dev.epicgames.com/documentation/unreal-engine/animation-blueprint-skeletal-controls-in-unreal-engine)。

## 检查方式与限制

Windows `computer-use` 工具的实时窗口连接失败，错误为 native pipe 不存在；按技能要求重试并重置后仍失败，没有操作用户窗口。最终画面判断来自用户新发的截图，已按 see 技能运行并读取结果；本地 OCR 没有识别到文字，不能替代动态视觉验证。

通过独立 UE Python 进程只读加载网格元数据，退出 0，0 errors / 0 warnings（零错误/零警告）。网格、新骨架和重定向器三份已有文件的 SHA-256 前后相同。另以本地 FBX 解析器只读核对蒙皮连接。过程报告位于被忽略的 `Saved/UnagiClothingInspection`，没有编译或执行播放、自动化、PIE（在编辑器中运行）、功能测试。

参考：[Epic AnimDynamics（动画动力学）](https://dev.epicgames.com/documentation/unreal-engine/animation-blueprint-animdynamics-in-unreal-engine?lang=en-US)、[Epic 布料工具属性](https://dev.epicgames.com/documentation/en-us/unreal-engine/clothing-tool-in-unreal-engine---properties-reference)。
