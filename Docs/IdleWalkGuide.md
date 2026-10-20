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

MCP 检查确认，当前 Walk 使用 `AS_Walk_Loop_F_0_Seq`，时长约 1.0667 秒。直接采样骨骼姿势，root 从第一帧 (0,0,0) 移动到末帧 (0,178.419815,0) cm；Idle 的 root 起止均为零。行走序列的 `Enable Root Motion`（启用根运动）和 `Force Root Lock`（强制根锁）当前均关闭；动画实例为 `Root Motion from Montages Only`（仅从蒙太奇提取根运动）。

这会让动画在胶囊体移动之外，再把可见人偶推到胶囊体前方。转向时该偏移随角色旋转，循环重置或混合回待机时偏移归零，与用户反馈的转弯突进、停止后退高度吻合。读取根运动提取接口返回单位变换不能证明原始 root 没有位移；本次以骨骼姿势采样得到的 178.42 cm 为证据。

本阶段位移由 CharacterMovement 驱动，修复步骤是打开当前行走序列，在 Asset Details（资产详细信息）搜索 Root Motion（根运动），开启 Force Root Lock（强制根锁）；Enable Root Motion 保持关闭。当前 Root Motion Root Lock（根运动根锁）为 Ref Pose（参考姿势），源 root 第一帧为零，本次无需另改锁定基准。保存这一个序列后由用户重新运行验证。不要批量锁定攻击动画，它们后续可能需要根运动。

此设置强制锁定根骨骼在输出姿势中的位置，不改源骨骼关键帧，不额外驱动胶囊体。此轮只完成只读诊断和说明，没有代为修改/保存动画资源，未运行 PIE 或播放测试；完整运行结果待用户确认。根位移修好后，600 cm/s 的角色移速与约 167.27 cm/s 的该动作根轨迹平均速度仍有差异，脚步速度匹配是后续独立步骤。

## 用户验证步骤与限制

- 在 PIE（编辑器中运行）后，在动画蓝图的调试对象中选择实际角色，查看 GroundSpeed 与 Should Move：静止为 0/false，移动超过 3 cm/s 后为非零/true，停止后回到 0/false。
- 验证站立播放待机、移动播放行走、只转镜头不会触发行走；松键减速后能回到待机。
- 本次没有接起步、停步、跳跃、急转或脚部 IK。快速反向仍可能滑步，后续再完善。
- 当前移动速度上限为 600 cm/s，尚未测量该步行动画的原始速度；这一阶段验证数据与状态切换，不保证脚步和位移完全匹配。
- 本次实际验证仅为源码/API 静态核对与 Git 差异检查，没有编译、动画播放、PIE 或自动化测试，不宣称运行通过。
