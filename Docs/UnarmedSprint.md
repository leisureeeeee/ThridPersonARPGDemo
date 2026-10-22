# 奔跑：收刀与持刀（Shift）

> 本轮持刀扩展的 C++ 已编译、三段动画已生成；因编辑器重启后 MCP 未连接，六个动画默认引用及 Sequence 引脚尚未配置。以下持刀行为为本轮实现目标，当前还不能按完成版验收；待用户正常重新打开编辑器后继续。

## 已实现行为

- 在地面时，按住左/右 Shift + 移动键奔跑。松开 Shift 回步行；松开移动键接现有 Stop（停步）。只按 Shift 不播放奔跑。
- 静止起跑：Idle/Stop → SprintStart → Sprint；走路中加速：Start/Walk → Sprint。
- 反向输入沿用移动组件的制动和转身判定，进入 SprintTurn，再回 Sprint；允许松键或拔刀打断。
- 正在拔刀或正在收刀时暂时禁用奔跑。动作成功开始立即取消奔跑；持续按住 Shift 时，动作结束后恢复对应持刀/收刀奔跑。持刀本身不再禁用奔跑。
- PlayerController（玩家控制器）解除控制或结束时清除奔跑请求，避免旧角色残留请求。

## 职责和调用链

`Shift → MyPlayerController::HandleSprintInput → MyCharacter::SetSprintRequested → ARPGCharacterMovementComponent::SetSprintRequested/GetMaxSpeed → MyCharacterAnimInstance::NativeUpdateAnimation → BP_CharacterAnim1.locomotion`

控制器只接收输入，不直接改速度或动画。角色只裁决装备状态。移动组件负责实际速度上限及碰撞、制动、旋转。动画实例读取运动结果，向动画图输出是否奔跑、播放时间和循环倍率。动画实例配置持刀/收刀各三段动画引用，在移动时按 IsWeaponDrawn 选择 ActiveSprintStart/Loop/TurnAnimation，动画图通过 Sequence（序列）引脚读取。C++ 不硬编码资源路径；没有复制两套状态机。

保持原 MaxWalkSpeed，使用 GetMaxSpeed 的覆盖返回奔跑速度，释放时自然恢复原步行配置。当前是项目的本地移动流程，没有额外实现网络预测和自定义移动请求复制。

## 动画资源

当前角色是 SK_Fenrir，动画类为 BP_CharacterAnim1，不能把 UE4 骨架动画直接接给它。使用现有 IKRIg_Man_to_ironman，源 ARPG_Samurai SK_Mannequin、目标 SK_Fenrir，生成以下目标骨架动画：

目录 `/Game/BP/MyCharatcer/Animations/Sprint`：

- RT_Anim_ARPGSamurai_Sprint_Start_no_weapon_Anim
- RT_Anim_ARPGSamurai_Sprint_no_weapon_Anim
- RT_Anim_ARPGSamurai_Sprint_Turn180_no_weapon_Anim

收刀使用同组 no_weapon（无武器）循环。持刀新增同目录 RT_Anim_ARPGSamurai_Sprint_Start、RT_Anim_ARPGSamurai_Sprint、RT_Anim_ARPGSamurai_Sprint_Turn180，源为用户指定 Sprint 系列。保留所有原资产。六段目标序列均为 SK_Fenrir_Skeleton、Root Motion（根运动）关闭，位移由移动组件处理。

SprintStart/SprintTurn 用 Sequence Evaluator（序列求值器）接 C++ 明确时间；循环用 Sequence Player（序列播放器）接 SprintPlayRate。新过渡使用原有 Inertialization（惯性化）链路。

## 参数和当前限制

- BP_Character 移动组件：Ground Locomotion > Sprint > Sprint Speed（地面运动 > 奔跑 > 奔跑速度），当前 720 cm/s（原600，提高20%）。
- BP_CharacterAnim1 类默认值：Locomotion > Sprint > Sprint Authored Speed（运动 > 奔跑 > 动画匹配速度），初始 600 cm/s。它是首版调节基准，尚未通过实机脚步效果标定。
- Sprint Start Duration（奔跑起步时长）初始 0.85 秒。源文件长 2.1167 秒，但采样显示约 0.8167 秒后身体主要骨骼位置不再变化，因此只播放有效部分，避免长时间静止。
- 180°无武器动画同样长 2.1167 秒，主要运动在约 1.1 秒内。掉头显式时间为制动/转身已过时间的两倍，上限 1.1 秒；移动组件最短掉头时间为 0.55 秒，超时保护 1 秒。
- 没有发现本组独立 Sprint Stop（奔跑停步）资产，先混合到原步行停步；高速脚步、停止姿势与掉头落脚仍需要运行反馈，未宣称已解决脚滑或实现精确距离匹配。

## 实际验证与手动观察

C++ Development Editor 编译成功并正常重启加载；动画蓝图状态 BS_UP_TO_DATE，结构检查 7 状态、20 转换、无错误警告。读回确认三个状态的输出姿势、起步/掉头显式时间和循环播放倍率已连接，目标骨架一致，结束时没有脏内容包。没有主动启动 PIE（在编辑器中运行）、模拟输入或功能测试。

你运行后按顺序观察：静止 Shift+W 起跑；W 行走时加按 Shift；保持 W 松 Shift；奔跑松 W；奔跑时 W/S 反向；奔跑时左键拔刀，持刀按 Shift 应使用持刀奔跑，右键收刀后继续按 Shift 应恢复无武器奔跑。重点反馈起步是否拖沓、掉头是否转过头、循环是否滑脚、停步是否突兀。


## 持刀奔跑与提速更新（2026-10-22）

移动上限从600升至720，循环动画匹配基准仍600，因此满速循环倍率从1升至1.2，脚步随速度加快。起步倍率 SprintStartPlayRate 设为1.15，收刀有效起步0.85秒约在0.739秒播放完；持刀起步使用0.75秒片段，约0.652秒播完。持刀掉头原长0.55秒，按同一掉头进度映射到0..0.55秒，避免沿用无武器1.1秒时间范围。

新增动画设置位于 BP_CharacterAnim1 类默认值的 Locomotion > Sprint > Animations（运动 > 奔跑 > 动画），可分别替换 Sheathed/Drawn Sprint Start/Loop/Turn Animation（收刀/持刀的奔跑起步/循环/掉头动画）。控制器输入和7状态20转换拓扑保持不变。
