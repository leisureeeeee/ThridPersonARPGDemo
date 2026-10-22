# 第一段左键轻攻击

## 操作和范围

Alt 拔刀后，左键单次按下播放一刀；收刀、拔收刀过程中、离地、已有 Montage 或攻击进行中不激活。按住左键不连发，动作中额外点击不缓存；完成后需要下一次点击。攻击中 Alt 不打断，攻击结束后可正常收刀。第一版仅做完整动作，不含命中检测、伤害、体力消耗、连招、弹反、锁定或自动转向。

## 从输入到 GAS

MyPlayerController 创建 LightAttackAction，映射鼠标左键，用 Started（开始）事件请求 Character.RequestLightAttack。Character 只转发到当前 Avatar 对应 ASC，不依赖具体键位。

MyCharacterAbilitySystemComponent 在权威端绑定 Avatar 后授予一次 ULightAttackAbility，保存 AbilitySpecHandle；重新绑定复用已有规格。请求通过 TryActivateAbility 激活，不直接调用 GA 的 ActivateAbility。更换 Avatar 或解除绑定时先取消此技能，再清理装备效果和 ActorInfo，避免动作及临时 Tag 残留在 PlayerState。

ULightAttackAbility 是 InstancedPerActor（每个拥有者一个实例）的原生 GA。Ability.Attack.Light 标识技能；要求 State.Weapon.Drawn，阻塞 State.Action.Drawing/Sheathing/Attacking；激活期间通过 ActivationOwnedTags 持有 State.Action.Attacking。当前没有数值消耗和冷却，所以不额外创建攻击 GE。

CanActivateAbility 同时核对地面、有效武器、当前动画实例、无其他 Montage、配置资源与网格骨架一致。ActivateAbility 提交技能，清除攻击开始时的自主移动速度，再通过 PlayMontageAndWait（播放蒙太奇并等待）驱动完整动作。仅在 OnCompleted 正常完成时结束，不在混出刚开始时提前解锁；中断、取消、播放失败均走结束清理，允许混出后中断回调，避免只收混出却等不到完成而卡住。动画实际碰撞和挥刀外观仍待用户运行确认。

## 动画资源与全身输出

当前使用 /Game/BP/MyCharatcer/Animations/LightAttack/AS_LightAttack_Fenrir：从 Katana 原始 UE4 Mannequin 攻击动画，经现有 IKRIg_Man_to_ironman 的 IK Retarget（IK 重定向）生成，源网格为 ARPG_Samurai/SK_Mannequin，目标网格为 SK_Fenrir。骨骼局部旋转、轴向与位置均转换到目标骨架。用户原资源 /Game/Asset/Fenrir_UE5/Anim/light_attack/AS_Combo_Attack_05_01_02_V02_Seq 原样保留。

AS_LightAttack_Fenrir 长约1.416667秒，启用 Root Motion（根运动）、Force Root Lock（强制根锁定），锁定为 Anim First Frame（动画第一帧）；通过 Montage 提取位移让胶囊跟随，避免仅网格前移后回弹。根运动路径仍受角色移动组件碰撞处理；没有额外做运动扭曲或目标吸附。旧 AS_LightAttack 保留作对照，当前蒙太奇不再引用它。

新增 AM_LightAttack 蒙太奇，Slot 为 FullBody，播放一次，倍率1.0，混入0.08秒、混出0.12秒。BP_CharacterAnim1 的原上半身 DefaultSlot 和按骨骼混合保持原链路，在其结果与最终输出之间增加 FullBody Slot。这样拔收刀保持上半身表现，轻攻击覆盖全身。动画实例已有 Root Motion from Montages Only（仅从蒙太奇提取根运动），无需改变模式；7个移动状态、20条转换保持不变。

## 移动约束与恢复

ARPGCharacterMovementComponent 通过 IAbilitySystemInterface 查询 ASC 的 State.Action.Attacking，不依赖具体玩家类。攻击期间 GetMaxAcceleration/GetMaxSpeed 返回0，PhysicsRotation 不执行自主转向，IsSprinting 为 false；蒙太奇根运动仍由引擎移动流程驱动。相机输入保持可用。

没有把 MovementMode 改为 None，也没有全局 DisableInput。GA 结束移除 Tag 后，移动组件立即恢复原计算和持续按住的移动/Shift请求，不需要复制一套旧速度/旧旋转参数再还原。

## 配置位置

Config/DefaultCombat.ini 的 [/Script/ThirdPersonARPG.LightAttackAbility] 配置 AttackMontage 和 PlayRate。GA 使用 Config 属性和 TSoftObjectPtr（软对象引用），C++ 不硬编码动画资源路径。未来替换蒙太奇只需改此配置；当前以新原生类的默认对象读取配置，不需修改用户已有 BP_Character 资产。

## 验证范围与操作

完成 C++ 构建、完整编辑器重新加载、原生类默认对象及配置读回、骨架一致性与动画图连线检查；角色/动画蓝图 BS_UP_TO_DATE，移动状态机结构验证无错误或警告。未运行自动化、PIE（在编辑器中运行）、模拟输入或功能测试。这些检查不等于挥刀手感、根位移碰撞和打断行为实测。

用户可运行后：Alt拔刀，左键观察一刀及其前进；结束后再点应能再次攻击；长按不连发、动作中多点不缓存；挥刀时按Alt不收刀，结束后Alt可收刀；持W/Shift点攻击后观察动作结束能恢复移动；观察靠墙时胶囊跟随是否符合预期。当前延续单机流程，GA采用ServerOnly（仅服务器执行），未增加客户端输入RPC、动作预测或联机表现同步，不宣称联机功能完成。

## 骨骼坐标适配的检查

Skeleton（骨架）引用相同，不代表动画轨道已正确重定向。先前播放资源抽查10根骨骼、7个时间点，局部旋转与小白人原始动作几乎完全一致，右前臂仍沿局部X轴约30.34厘米；Fenrir正常动画应沿局部Y轴约34.40厘米。新资源经IK转换后使用目标轴向、长度和旋转，避免不同参考骨架直接解释同一组局部变换导致的躯干肢体错位。

通过UE MCP打开新动画资源预览，检查了0.3秒与0.8秒画面，未见此前截图中躯干四肢坍缩错位。没有运行PIE或游戏功能测试，最终游戏内持刀与根运动表现仍需用户运行观察。
