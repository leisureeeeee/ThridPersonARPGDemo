# 雅的衣发动态与重定向：MCP 实时配置审计

2026-10-20，北京时间。通过运行中的 ThirdPersonARPG 编辑器读取，不覆盖用户资产，不启动动画播放、PIE、模拟、编译或功能测试。

## 当前场景角色

- NewMap 中 BP_Character 的 CharacterMesh0 使用 `/Game/Asset/Unagi/SK_Unagi_NoWeapon`，骨架为 `SKEL_Unagi_NoWeapon`，234 根骨骼。
- Animation Mode（动画模式）为 Animation Blueprint（动画蓝图），但 Anim Class（动画类）为空，没有当前动画实例或单节点播放动画。此结论针对检查时场景中的组件，不排除用户另行在动画资产编辑器预览，也不证明游戏代码以后不会设置动画。
- 网格的 Physics Asset（物理资产）为空，Clothing Assets（布料资产）为空，Post Process Anim Blueprint（后处理动画蓝图）为空。项目中单独存在 `1_PhysicsAsset`，但它没有分配到当前网格；本次未成功读取其内部物理体和约束，不能判断其制作质量或适配对象。
- 头发包含 HairB1/2/3、HairF1/2/3/4、HairS1/2 多段骨骼链，链根挂在 Bip001-Head。另有 Furisode、MaJia、Skirt 衣袖/衣摆骨骼。本次仅确认骨骼与层级；历史文档中的蒙皮证据仍可参考，但不把名称当作全部权重或视觉验证。

## 两套重定向的实际状态

| 项目 | RTG_UE4_To_Unagi | IK_To2 |
| --- | --- | --- |
| 源 IK Rig（绑定） | IK_UE4Mannequin | IK_man |
| 目标 IK Rig（绑定） | IK_Unagi | IK_Unagi111 |
| 目标人体链 | 9 条，全部有同名源映射 | 0 条 |
| 目标重定向骨盆 | Bip001-Pelvis | 未设置 |
| 目标 IK Solver（求解器） | 0 个 | 0 个 |
| 当前目标姿势 | Default Pose | Default Pose |
| 操作栈 | 有骨盆、FK、根运动等操作；Run IK Rig 已禁用且其 chains 为空 | 0 个操作 |

原 RTG 的 9 条链为 Spine、左右 Clavicle、左右 Arm、左右 Leg、Neck、Head；没有手指、脚趾链，没有手脚 IK Goal（IK 目标）。它可以传递基本人体 FK（正向运动学）动作，但当前没有完整的手脚接触修正流程。IK Toon/空绑定的存在不证明用户正在用它播放；应先确认实际预览或导出的动画来自哪个重定向器。

目标姿势列表中存在名为 `为Unagi_CLothesFix` 的姿势，但当前激活的是 Default Pose。当前抽查的人体和衣袖骨骼的重定向姿势旋转偏移都是单位四元数。修改姿势也不会自动重导出已烘焙的动画。

通过 IK Rig 参考姿势的骨骼位置计算，源/目标左上臂方向夹角约 2.20°，长度分别约 30.34 cm 和 22.36 cm。这不是完整姿势或动作质量评估，不能因此断言所有关节都匹配；同样不能未经检查就断言全部问题来自明显的 A/T 姿势错位。

## 后续修复顺序

1. 保留现有资产，使用重定向器/绑定副本处理：确定实际动画来源，补足所需手指、脚趾链，检查骨盆和根运动，配置手脚 IK 目标及求解；单独处理必要的前臂/上臂扭转与辅助骨驱动。根据实际握刀、抬臂、走路的失败点调整，而非批量把所有骨骼加进链。
2. 确认角色实际使用的 Animation Blueprint（动画蓝图），使基本人体动作正常输出，再检查衣袖支点。Bone015 等特殊支点不能直接靠骨盆运动替代手臂跟随，也不应未经验证修改父级或蒙皮。
3. 在人体动作和支点正常后，为头发、衣摆逐链配置 AnimDynamics（动画动力学）等次级运动；调整根部固定、阻尼和运动限制。它使用自己的简化物理体，不要求先给全身勾选模拟物理。宽衣袖需要更细布面效果时，再评估 Chaos Cloth（混沌布料）及碰撞。

当前完成的是问题定位，没有执行上述修复，也没有逐动作播放或判断最终动态效果。所有结果为检查时状态。

参考：https://dev.epicgames.com/documentation/unreal-engine/retargeting-operation-stack-in-unreal-engine-5-8

参考：https://dev.epicgames.com/documentation/unreal-engine/animation-blueprint-animdynamics-in-unreal-engine
