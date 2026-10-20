# 当前基础走路接法

2026-10-21 更新：原先 Idle（待机）↔ Walk（行走）的双状态接法已扩展为四状态。完整改动、六条过渡及测试步骤见 [基础走路起停说明](BasicLocomotion-StartStop.md)。

## 现在的数据含义

- `GroundSpeed`：角色实际水平速度，单位 cm/s。
- `bShouldMove`：角色移动组件是否收到非零水平移动意图。现在不再用速度大于 3 判断这一项；松键后意图会清除，即使角色还在制动。
- `UMyCharacterAnimInstance::NativeUpdateAnimation()` 采集这两项数据；角色移动组件继续处理位移、碰撞、转身与制动，控制器继续处理输入。

## 当前状态机

动画蓝图：`/Game/BP/MyCharatcer/BP_CharacterAnim`，父类为 `MyCharacterAnimInstance`，状态机名为 `locomotion`。

```text
Idle → Start → Walk → Stop → Idle
       └─松键─→ Stop ─重新输入─→ Start
```

| 状态 | 动画 | 循环 |
|---|---|---|
| Idle | `AS_Idle_Seq` | 是 |
| Start | `AS_Walk_Start_F_0_Seq` | 否 |
| Walk | `AS_Walk_Loop_F_0_Seq` | 是 |
| Stop | `AS_Walk_Stop_F_0_Seq` | 否 |

Start/Stop 播放一次，进入时重置；Start → Walk 与 Stop → Idle 按剩余播放时间自动过渡。输入打断规则优先级高于自动结束规则。旧的 Idle ↔ Walk 直连已移除。

## 位移基准

源码默认 `MaxWalkSpeed = 167.0f`，依据行走循环原始轨迹约 178.42 cm / 1.0667 s ≈ 167.27 cm/s 取整。四段动画播放器使用 Play Rate（播放速率）1。行走、起步、停步序列开启 Force Root Lock（强制根锁），保持 Enable Root Motion（启用根运动）关闭，避免动画位移叠加到胶囊体移动之外。

用户已修复 Start/Stop 的骨架引用；现与 Loop 使用同一份 UE4 人偶骨架。本轮未改骨架结构、源关键帧或攻击动画。

## 用户接下来要做的事

1. 编译并重新加载更新后的 C++；需要完整加载时保存退出编辑器，以 Development Editor（开发编辑器）目标构建再重新打开。
2. 编译保存 `BP_CharacterAnim`，角色 Mesh（骨骼网格体组件）的 Anim Class（动画类）继续选择它。
3. 确认角色最大行走速度为 167；若蓝图或场景实例显式覆盖为 600，只重置这一项。
4. 测试待机循环、长按走路、松键停步、短按松键、停步中重新按键、W+A/D 改向。

## 本版限制

这版完成基础起停状态流转与输入打断，未加入距离匹配、步幅适配、左右脚停步选择、急转、跑步或脚部 IK。起停动作按动画时间播放，尚未让动画进度与实际位移逐帧匹配；脚底滑动与自然程度以用户效果测试为准。

本轮仅核对源码、资产属性及状态/连线/规则，未执行 C++ 构建或运行效果测试。
