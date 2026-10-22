# 持刀与收刀状态的 GAS 重构

## 本次范围

仅迁移现有装备状态的唯一数据源，并接入 PlayerState 持有的 ASC/AS。现有按键、动画资源、动态 Montage、插槽、光剑激活/关闭、动画通知时机和移动参数保持原行为。没有恢复此前撤回的战斗框架，也没有新增攻击、招架、属性数值或技能树。

## 持有关系与生命周期

ARPGPlayerState 构造函数分别创建 MyCharacterAbilitySystemComponent 和 AttributeSetBase，两个对象的 Outer 都是 PlayerState。ASC 使用 Mixed 复制模式。AS 仍为空基类，本轮不引入实际属性。

GameMode 指定 ARPGPlayerState。MyCharacter 在服务器 PossessedBy、客户端 OnRep_PlayerState 中取得两个引用，InitAbilityActorInfo 的 OwnerActor 是 PlayerState、AvatarActor 是 Character；BeginPlay 补齐初始化，OnRep_Controller 刷新控制器缓存。Character 的构造函数不创建第二个 ASC 或 AS。

UnPossessed/EndPlay 停止拔收刀 Montage、解除结束委托，只移除本模块保存句柄的两个 GE，再清除 ActorInfo 和 Character 引用。旧 Character 通过 ExpectedAvatar 检查，不能清除新身体的状态。重新绑定时从实际武器挂接插槽恢复稳定状态；武器初始化成功时默认收刀。没有有效武器时不授予 Drawn 或 Sheathed。

## 状态唯一来源

四个 Native GameplayTag（原生游戏标签）：

- State.Weapon.Drawn：武器已经成功挂到手上并激活。
- State.Weapon.Sheathed：武器已经成功挂回刀鞘并关闭。
- State.Action.Drawing：拔刀 Montage 正在进行。
- State.Action.Sheathing：收刀 Montage 正在进行。

稳定状态和动作状态分别由一个 Infinite GameplayEffect（无限期游戏效果）持有。使用原生 UWeaponStateEffect 定义和每份 Spec 的 DynamicGrantedTags，不动态创建临时 GE 定义。每次替换移除旧句柄，不对 Tag 引用计数反复加减；只操作本模块自己的效果。效果替换期间会同步发出 Tag 变更回调，回调不应假定多标签是原子事务；当前消费者在动画更新/输入调用时读取完成后的状态。

MyCharacter 删除 bWeaponDrawn/bDrawingWeapon/bSheathingWeapon。IsWeaponDrawn/IsDrawingWeapon/IsSheathingWeapon 保留原签名，改为查询当前 Avatar 对应 ASC 的标签，所以原动画接口不需修改或重新配置蓝图。

## 调用链与中断

PlayerController 输入 → MyCharacter.DrawWeapon/SheatheWeapon → 播放原动态 Montage → ASC 记录动作 Tag → 原动画 Notify 执行 Attach 和武器激活/关闭 → 成功后提交稳定状态 Tag → Montage 结束移除动作效果。

通知之前中断，保持原稳定状态；通知之后中断，保留已提交的挂接与稳定状态。播放失败不添加动作 Tag；挂接或激活失败沿用原回退；效果应用失败也回退物理挂接。角色解除控制/销毁清理两个效果，不将临时状态带入新身体。

这次只迁移状态管理，拔收刀流程仍由现有 Character/Montage 驱动，没有为了迁移三项状态新建 GA。以后迁移 GA 时可以复用相同标签和状态提交时机。

## 编译与验证范围

新增反射类及 GameMode 构造默认值，必须完整 C++ 编译并重新加载。实际构建结果见本任务 ExecutionLog。

当前沿用项目已有的单机拔收刀流程。GE 标签具备 GAS 复制能力，但输入到服务器、客户端 Montage、武器 Actor/挂接复制和动作预测尚未接入，不宣称本次完成联机拔收刀。状态修改限定服务端，独立运行的本地玩家即为权威端。

未请求自动化或功能测试，不运行 PIE（在编辑器中运行）或模拟输入。用户可检查：左键拔刀/右键收刀；连续重复输入不能重入；移动中切换后，待机/行走/停步与武器姿势匹配；持刀和收刀奔跑沿用既有行为。在 GAS 调试器观察稳定状态随通知切换、动作状态随 Montage 结束清除。
