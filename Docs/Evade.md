# 空格八方向躲避

## 操作与当前范围

按一次空格触发一次地面躲避，约0.9秒。当前有WASD移动输入时，沿与移动一致的世界方向躲避；无输入时，向角色当前朝向的后方躲避。相机方向影响WASD的世界方向，动画选择则相对角色当前朝向分成八个45度扇区，不是把W/S固定绑定到某个动画名。持续按住空格不会连发，躲避中输入不缓存。

持刀和收刀状态都允许躲避。攻击、拔收刀和躲避互斥；当前不支持通过躲避打断攻击。Alt仍切换拔收刀，左键仍为轻攻击，Shift仍为奔跑。动作结束后恢复原移动规则，相机输入保持可用。

这次只接动作与根运动位移，不包含无敌帧、伤害免疫、体力消耗、取消窗口、连段或联网预测。现有玩家流程为单机权威端，GA采用ServerOnly（仅服务器执行）；未新增客户端请求RPC和联机表现同步。

## 输入、事件与技能

MyPlayerController 创建 EvadeAction，映射 SpaceBar（空格），绑定 Started（开始）事件。触发时从 UEnhancedPlayerInput.GetActionValue(MoveAction) 读取当前帧输入，避免依赖回调顺序或上一帧缓存。移动和躲避共用 GetWorldMovementInput，使用当前角色的相机偏航角换算WASD世界方向；Character不引用具体键位或Enhanced Input类型。

Character.RequestEvade 将世界方向交给当前Avatar对应ASC。ASC先确认权威端及Avatar身份，把方向转换到角色局部坐标，通过 atan2/45度舍入得到八方向索引；无输入索引为4（后撤）。Event.Action.Evade 的 EventMagnitude 携带这个索引，HandleGameplayEvent 触发已经授予的 UEvadeAbility。

方向顺序在代码与配置中保持一致：前、右前、右、右后、后、左后、左、左前。方向在请求时捕获，动作开始后不随相机或方向键重新选取动画。

UEvadeAbility 为 InstancedPerActor（每个拥有者一个实例），不重入。Ability.Evade 标识技能，激活期间自动持有 State.Action.Evading，阻塞 Drawing/Sheathing/Attacking/Evading。它检查地面、动画实例、无其他蒙太奇及八项配置；激活时检查选中蒙太奇的骨架并提交技能，使用 PlayMontageAndWait（播放蒙太奇并等待）播放到完成。完成、中断、取消或播放失败均结束技能并清除临时状态；不在混出开始时提前释放。

玩家ASC在绑定Avatar后按句柄只授予一次躲避技能，换身体后复用。更换Avatar或解除控制时，CancelCharacterActions同时取消轻攻击与躲避，再清理ActorInfo（能力系统角色信息），防止PlayerState留下旧身体的动作状态。

## 动画资产

直接使用用户已生成的 /Game/Asset/Fenrir_UE5/Anim/evade/Anim_ARPGSamurai_Evade_* 八个Fenrir动画，没有再次重定向，也没有修改这些原文件。除了Skeleton引用，还抽查了右前臂在0.3秒的局部位置，八个方向都符合Fenrir约Y=-34.4厘米的骨骼坐标。

在 /Game/BP/MyCharatcer/Animations/Evade 新建八个 AS_Evade_* 播放副本和八个 AM_Evade_* 蒙太奇。副本启用 Root Motion（根运动）、Force Root Lock（强制根锁定）、Anim First Frame（动画第一帧）根锁；蒙太奇使用现有FullBody Slot（全身插槽），单次播放，混入0.06秒、混出0.1秒，不修改动画蓝图结构。

根位移保留用户目标动画的比例：四个正方向约568.73厘米，斜方向两个轴各约426.55厘米。考虑当前网格相对角色的-90度偏航后，已读回确认根位移与八方向扇区对齐。实际移动由CharacterMovement（角色移动组件）应用动画根运动并处理碰撞，不用Teleport（瞬移）或手工移动Actor。

## 移动约束与配置

ARPGCharacterMovementComponent 的 IsActionMovementLocked 同时查询 Attacking 和 Evading 标签。躲避时限制自主加速度、速度与朝向旋转，暂停奔跑判定，仍允许引擎应用蒙太奇根运动。GA结束移除标签后自动恢复原规则，没有保存或强行还原一套速度/旋转字段，也没有全局DisableInput（禁用输入）或修改MovementMode（移动模式）。

Config/DefaultCombat.ini 的 [/Script/ThirdPersonARPG.EvadeAbility] 保存八个蒙太奇软引用，顺序按上述方向排列。PlayRate默认1.0；RootMotionTranslationScale默认1.0，可用于整体调整根运动距离。C++不硬编码资源路径。配置由原生类默认对象加载，修改配置后应重新加载对应默认对象或重启编辑器。

## 已做检查与用户观察步骤

最终Development Editor（开发编辑器）构建通过并重新加载。UE MCP读回八项配置、骨架、FullBody插槽、根运动与长度，按网格偏航换算后八方向根位移对齐值均为1。角色和动画蓝图均BS_UP_TO_DATE，编辑器无脏内容或地图。后撤资源在动画编辑器0.3秒预览中未见此前攻击的肢体坍缩错位。

未运行PIE（在编辑器中运行）、自动化、模拟输入或游戏功能测试；编译、资源数据与预览检查不能代替游戏内移动与碰撞验证。

用户可观察：静止按空格是否后撤；按WASD及斜方向同时按空格，是否沿输入方向躲避；换相机角度后重复；收刀和持刀都尝试；长按空格不连发；躲避中点击左键或Alt不抢动作；结束后能正常移动、奔跑、攻击和收刀；靠墙观察根运动碰撞。当前不应期待无敌帧或攻击取消。
