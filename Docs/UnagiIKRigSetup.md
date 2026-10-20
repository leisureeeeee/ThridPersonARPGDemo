# 雅角色 IK 绑定配置

2026-10-20 创建。新资源位于内容浏览器 `/Game/Asset/Unagi/Rig`，没有修改人物、原骨架或原动画。

## 已创建的资源

| 资源 | 职责 | 使用的模型 |
| --- | --- | --- |
| `IK_UE4Mannequin` | 源 IK Rig（IK绑定），描述人偶的骨盆和身体运动链 | `/Game/ARPG_Samurai/Demo/Mannequin/Character/Mesh/SK_Mannequin` |
| `IK_Unagi` | 目标 IK Rig（IK绑定），描述雅的骨盆和身体运动链 | `/Game/Asset/Unagi/SK_Unagi_NoWeapon` |
| `RTG_UE4_To_Unagi` | IK Retargeter（IK重定向器），连接源与目标并保存运动链映射 | 上述两份绑定 |

复用项目已有 UE4 人偶，未另外复制源骨架。源骨盆为 `pelvis`，目标骨盆为 `Bip001-Pelvis`。Root Motion（根运动）骨骼分别使用两份模型真实骨骼层级的第一个节点：`root` 和 `Avatar_Female_Size02_Unagi_Model`；骨盆与根运动节点承担不同职责，没有将两者混为一谈。

## 运动链

| 链名称 | 源起始 → 结束 | 雅起始 → 结束 |
| --- | --- | --- |
| `Spine`（脊柱） | `spine_01` → `spine_03` | `Bip001-Spine` → `Bip001-Spine2` |
| `LeftClavicle`（左锁骨） | `clavicle_l` → `clavicle_l` | `Bip001-L-Clavicle` → `Bip001-L-Clavicle` |
| `RightClavicle`（右锁骨） | `clavicle_r` → `clavicle_r` | `Bip001-R-Clavicle` → `Bip001-R-Clavicle` |
| `LeftArm`（左臂） | `upperarm_l` → `hand_l` | `Bip001-L-UpperArm` → `Bip001-L-Hand` |
| `RightArm`（右臂） | `upperarm_r` → `hand_r` | `Bip001-R-UpperArm` → `Bip001-R-Hand` |
| `LeftLeg`（左腿） | `thigh_l` → `foot_l` | `Bip001-L-Thigh` → `Bip001-L-Foot` |
| `RightLeg`（右腿） | `thigh_r` → `foot_r` | `Bip001-R-Thigh` → `Bip001-R-Foot` |
| `Neck`（颈部） | `neck_01` → `neck_01` | `Bip001-Neck` → `Bip001-Neck` |
| `Head`（头部） | `head` → `head` | `Bip001-Head` → `Bip001-Head` |

链名称用于说明人体部位。源和目标骨骼可以保留不同名称，重定向器通过同名链对应它们，而不是强行更换动画的骨架引用。创建前读取两份模型的真实骨骼，并检查每条链的结束骨骼确实是起始骨骼本身或其后代。

重定向器已配置骨盆运动、FK Chains（正向运动学链）、根运动和曲线映射操作。基础运动链转换采用骨盆与 FK，无须先添加求解器；由于当前没有 IK Goal（IK目标）及求解器，Run IK Rig（运行IK绑定）操作暂时关闭。没有配置手指、脚趾、衣服、头发或武器的专用链。

## 当前范围与后续操作

本次建立的是重定向配置，并未修复原太刀动画缺少的依赖：

`/Game/Katana_Animations/Demo/Mannequin_UE4/Character/Mesh/UE4_Mannequin_Skeleton`

现有 UE4 人偶的骨架位于另一个资源路径；建立源绑定不会自动修复上述引用。原太刀动画仍应从完整原包恢复骨架及预览模型，或另取未错误替换的副本，先确认它在正确源骨架上有效。之后再将可用动画转换给雅，避免在原动画上直接替换为 `SKEL_Unagi`。

用户可在编辑器中按以下步骤检查：

1. 在内容浏览器进入 `Asset/Unagi/Rig`，打开 `IK_Unagi`。预期显示无武器的雅，Hierarchy（层级）中的骨盆为 `Bip001-Pelvis`，IK Retargeting（IK重定向）中有上表 9 条链。
2. 打开 `IK_UE4Mannequin`，预期显示 UE4 人偶，骨盆为 `pelvis`，链名称与雅一致。
3. 打开 `RTG_UE4_To_Unagi`。在 Asset Settings（资产设置）中核对 Source IKRig Asset（源IK绑定资产）与 Target IKRig Asset（目标IK绑定资产），检查 FK Chains（正向运动学链）操作中的 9 条链映射。
4. 后续用可用的源动画，通过 Run Retarget（运行重定向）检查动作。当前使用默认参考姿势；两边若为不同的 A/T 形姿势，需先调整目标 Retarget Pose（重定向姿势），再核对肩臂、脚底及握刀姿态。
5. 满意后使用 Export Selected Animations（导出所选动画）另存目标动画；本次未执行此步骤。

本次没有启动播放、PIE（在编辑器中运行）、自动化或其他功能测试，也没有编译 C++。默认参考姿势、脚底接触、根运动表现和手指握刀效果尚未经视觉确认，不能将资产配置检查等同于动画表现验收。

## 创建方式与完整性核对

`Tools/Unagi/create_unagi_ik_rigs.py` 通过 UE 编辑器 Python 的资产工厂和控制器创建资产，只显式保存上述三个新包。脚本拒绝覆盖同路径既有资产；后续在编辑器中手动修改配置不会被脚本自动覆盖。没有将输入、角色行为或动画资源路径写入 `MyCharacter.h`。

创建后另起只读 UE 命令行进程，从磁盘重新加载三个资产，核对模型、骨盆、根运动骨骼、运动链、源目标引用和映射。两个成功进程均退出 0，日志均为 0 errors / 0 warnings（零错误/零警告）。17 个既有受保护文件（含人物与材质、源人偶包、四个待机动画、地图和编辑器配置）创建前后 SHA-256 相同。创建与只读重载得到的新资产哈希相同。

资源配置摘要及哈希见 `Docs/UnagiIKRigSetup.json`。临时过程日志在被忽略的 `Saved/UnagiIKRigWork`，不纳入 Git。

原 `SKEL_Unagi` 与人物材质依然是用户工作区中尚未跟踪的依赖，本次没有顺带暂存；在其他工作区使用新绑定及无武器人物时，也需要这些原依赖。

参考：[Epic IK绑定 Python 工作流](https://dev.epicgames.com/documentation/unreal-engine/using-python-to-create-and-edit-ik-rigs-in-unreal-engine)、[Epic IK重定向说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)。
