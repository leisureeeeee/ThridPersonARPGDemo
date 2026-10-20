# 用 C++ 动画实例连接待机与行走

## 当前实现

已在用户创建的 `Source/ThirdPersonARPG/AnimInstance/MyCharacterAnimInstance.h` 和 `.cpp` 中实现 `UMyCharacterAnimInstance`。尚未编译或加载新反射属性，未修改动画蓝图、角色或动画资产，未运行测试。用户选择直接使用 UE4 人偶骨架，本步骤按该前提接线。

## 调用链与职责

```text
PlayerController：处理玩家输入
  → CharacterMovement：加速、碰撞、位移、制动和转向
  → MyCharacterAnimInstance：读取实际水平速度
  → 动画蓝图：根据 bShouldMove 在 Idle 和 Walk 间切换
```

`UAnimInstance` 是动画实例基类。引擎在动画更新阶段调用 `NativeUpdateAnimation()`，这里先调用父类，再通过 `TryGetPawnOwner()` 获取拥有当前网格体的 Pawn，读取 `GetVelocity().Size2D()`。速度单位为 cm/s，忽略竖直分量。

- `GroundSpeed`：真实水平速度，供蓝图观察和后续播放速率匹配使用。
- `bShouldMove`：`GroundSpeed > 3.0f` 时为 true，供状态机过渡使用；3 cm/s 是过滤接近停止时微小速度的起始阈值。
- 预览中没有有效 Pawn 时，两项数据恢复为零/false，避免空指针和沿用旧速度。
- 两项成员为 private，使用 `BlueprintReadOnly` 与 `AllowPrivateAccess` 允许蓝图读取；`Transient` 表示运行时数据不需要持久保存。

使用实际速度而不是键盘按下状态，角色松键减速时仍可行走，碰墙实际停止时可回到待机。此处仅采集数据，在游戏线程读取 Pawn；没有在工作线程直接访问角色或修改移动组件，也不需要角色依赖具体动画实例类。

本机引擎 `AnimInstance.h` 建议在 `NativeUpdateAnimation()` 收集数据，将较重的线程安全计算放到 `NativeThreadSafeUpdateAnimation()`。当前只有少量速度计算，无需加入代理或复杂线程结构。现有 Build.cs 已依赖 Engine 模块，无需新增动画图表模块依赖。

## 编译与编辑器接线

1. 本次新增了反射属性，用户先编译并让编辑器重新加载。若属性或类未刷新，保存工作后关闭编辑器，以 Development Editor（开发编辑器）目标构建，再重新打开。本次没有替用户执行编译。
2. 创建或打开 `ABP_ARPG_UE4`，在 Class Settings（类设置）确认 Parent Class（父类）为 `MyCharacterAnimInstance`，Skeleton（骨架）为匹配的 UE4 人偶骨架。
3. 在 AnimGraph（动画图表）中添加 State Machine（状态机），命名 `Locomotion` 并接到 Output Pose（输出姿势）。
4. 双击进入，在其中创建 Idle（待机）和 Walk（行走），入口连 Idle。Idle 内使用 `AS_Idle_Seq`，Walk 内使用 `AS_Walk_Loop_F_0_Seq`；两个 Sequence Player（序列播放器）均启用 Loop Animation（循环动画）。
5. Idle → Walk 的过渡条件连接 `bShouldMove`；Walk → Idle 连接它的逻辑非，即 `!bShouldMove`。在 My Blueprint（我的蓝图）中显示继承变量后拖入读取；布尔变量可能显示为 Should Move。过渡混合时间先试 0.15 秒。
6. 在当前角色蓝图中选 Mesh（骨骼网格体组件），Animation Mode（动画模式）设为 Use Animation Blueprint（使用动画蓝图），Anim Class（动画类）设为所创建的动画蓝图。

Event Graph（事件图表）不必重复计算速度。因为角色已朝移动方向转身，第一版只接前向行走循环。

## 行走动画自带位移的处理

MCP 检查确认，当前 Walk 使用 `AS_Walk_Loop_F_0_Seq`，时长约 1.0667 秒。直接采样骨骼姿势，root 从第一帧 (0,0,0) 移动到末帧 (0,178.419815,0) cm；Idle 的 root 起止均为零。根位移诊断当时，行走序列的 `Enable Root Motion`（启用根运动）和 `Force Root Lock`（强制根锁）均关闭；后续 MCP 已确认强制根锁开启、启用根运动关闭。动画实例为 `Root Motion from Montages Only`（仅从蒙太奇提取根运动）。

这会让动画在胶囊体移动之外，再把可见人偶推到胶囊体前方。转向时该偏移随角色旋转，循环重置或混合回待机时偏移归零，与用户反馈的转弯突进、停止后退高度吻合。读取根运动提取接口返回单位变换不能证明原始 root 没有位移；本次以骨骼姿势采样得到的 178.42 cm 为证据。

本阶段位移由 CharacterMovement 驱动，修复步骤是打开当前行走序列，在 Asset Details（资产详细信息）搜索 Root Motion（根运动），开启 Force Root Lock（强制根锁）；Enable Root Motion 保持关闭。当前 Root Motion Root Lock（根运动根锁）为 Ref Pose（参考姿势），源 root 第一帧为零，本次无需另改锁定基准。保存这一个序列后由用户重新运行验证。不要批量锁定攻击动画，它们后续可能需要根运动。

此设置强制锁定根骨骼在输出姿势中的位置，不改源骨骼关键帧，不额外驱动胶囊体。根位移诊断阶段只完成只读检查与说明；用户已反馈根锁解决了突进/回退。后续 MCP 确认行走播放器循环已开启。步速匹配按下面的独立步骤实施，效果测试由用户负责。

## 步速匹配：C++ 默认速度

已在 `AMyCharacter` 构造函数中设置 `GetCharacterMovement()->MaxWalkSpeed = 167.0f`，使用现有角色移动组件的速度配置，不新增一套重复的 WalkSpeed 属性。该值来自当前行走序列原始根位移约 178.42 cm / 1.0667 秒 ≈ 167.27 cm/s，先取 167 cm/s，配合行走播放器 Play Rate（播放速率）1 使用。

旧的编辑器角色速度为 600 cm/s，约为动画均速的 3.6 倍；根锁之后仍用 600 会出现脚步慢而角色前移快的滑步。调整最大速度使胶囊体的匀速位移接近动作的设计步速，源 root 仍由强制根锁去除。本步骤不添加跑步、动态播放速率、起停或 IK。

本次修改了构造函数，需要用户编译并重新加载原生类；建议保存工作后退出编辑器，构建 Development Editor（开发编辑器）目标，再打开项目。未代为编译或重启编辑器。

重新打开后，在角色蓝图及场景角色的 Character Movement（角色移动组件）中检查 Max Walk Speed（最大行走速度）是否为 167。蓝图或摆放实例若显式保存了 600 的覆盖值，C++ 默认不会强行覆盖用户配置：在该属性旁重置为继承默认值，或明确设为 167；不要重置整张蓝图。

由用户验证：按住 W 至少 5 秒，观察持续行走时落地脚是否明显滑动；W+A / W+D 时动作持续播放，随后松键回到待机。当前 Idle 播放器循环仍为 false（本次 MCP 只读确认），需要在 Idle 状态序列节点开启循环并编译保存动画蓝图，以避免待机 10 秒后停帧。本次没有修改或保存该蓝图，保留用户现有资产修改。

## 用户验证步骤与限制

- 在 PIE（编辑器中运行）后，在动画蓝图的调试对象中选择实际角色，查看 GroundSpeed 与 Should Move：静止为 0/false，移动超过 3 cm/s 后为非零/true，停止后回到 0/false。
- 验证站立播放待机、移动播放行走、只转镜头不会触发行走；松键减速后能回到待机。
- 本次没有接起步、停步、跳跃、急转或脚部 IK。快速反向仍可能滑步，后续再完善。
- 新的 C++ 默认速度为 167 cm/s，按当前动画约 178.42 cm / 1.0667 s 的根轨迹均速取整；这是匀速直行的基准，加减速和快速转向的脚步修正留后续。
- 本次实际验证仅为源码/API 静态核对与 Git 差异检查，没有编译、动画播放、PIE 或自动化测试，不宣称运行通过。
