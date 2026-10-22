# 角色与 GAS 基类

当前建立 C++ 基类、指针和继承关系。BossCharacter 在构造函数中创建 ASC 与属性集；玩家由 ARPGPlayerState（玩家状态）分别创建 ASC/AS，MyCharacter 绑定引用。玩家持刀/收刀状态已接入 GAS，详见 WeaponStateGAS.md；仍不实现具体战斗技能或数值属性。

```text
ACharacter
  └─ ACharacterBase : IAbilitySystemInterface
       ├─ AMyCharacter
       └─ ABossCharacter

UAbilitySystemComponent
  └─ UAbilitySystemComponentBase

UAttributeSet
  └─ UAttributeSetBase
```

`ACharacterBase` 声明 `AbilitySystemComponent` 和 `AttributeSet` 两个受保护指针，类型分别为两个 GAS 基类。受保护是为了后续角色子类从实际拥有者取得引用；蓝图可读取但不能直接写入。指针使用 `UPROPERTY` 与 `TObjectPtr`，会被 Unreal 的垃圾回收识别；`Transient` 表示不把运行时引用保存进资源。

`ACharacterBase` 中的两个指针默认均为空，该类不调用 `CreateDefaultSubobject` 创建 GAS 实例。保存引用与决定实例的拥有者是两个职责：玩家现在由 ARPGPlayerState 创建具体 ASC（能力系统组件）及属性集，MyCharacter 在 PossessedBy/OnRep_PlayerState 中取得引用，OwnerActor=PlayerState、AvatarActor=Character；其他具体角色也可选择自己的拥有方式。解除控制与结束时清理玩家身体的装备效果并释放引用。

`IAbilitySystemInterface` 是 GAS 用于从角色查询能力系统组件的引擎接口。`GetAbilitySystemComponent()` 返回上述 ASC 指针，`GetAttributeSet()` 提供属性集读取入口。CharacterBase 与 MyCharacter 的默认对象仍未赋值，非运行关卡角色尚无 PlayerState 时返回空指针是预期行为；运行中的玩家在绑定 PlayerState 后取得有效引用。BossCharacter 的构造函数直接填入两个子对象，调用方仍应检查有效性。

`ACharacterBase` 标记 `Abstract`（抽象类）；`AMyCharacter` 和 `ABossCharacter` 均可实例化。ASC 与属性集基类保留可派生能力，同时允许直接创建基础实例，因此不标记 Abstract。ASC 基类不预设复制策略、能力授予或激活逻辑，属性集基类不声明生命值等实际属性，也不包含结算规则。

MyCharacter 原有相机、武器挂接和移动实现留在原类；拔刀/收刀过程仍由既有 Montage 驱动，三个装备状态查询已改为读取 PlayerState ASC 的 GameplayTag（游戏标签）。`ACharacterBase` 将 `FObjectInitializer` 原样传给 `ACharacter`，所以 MyCharacter 原有的 `SetDefaultSubobjectClass<UARPGCharacterMovementComponent>` 仍可指定自定义移动组件。角色蓝图的直接父类仍是 MyCharacter，新增基类位于其 C++ 继承链中，无需手动重新指定蓝图父类。

为了让头文件引用 GAS 类型，项目启用 GameplayAbilities 插件，并增加 GameplayAbilities、GameplayTags、GameplayTasks 模块依赖。模块/插件可用不表示已经创建或初始化 ASC 实例。

新增反射类型和原生父类调整需要完整 C++ 编译并重新加载编辑器；实际结果见 `Docs/ExecutionLog.md`。在角色蓝图的 Class Settings（类设置）中仍应看到 MyCharacter；可以在继承属性中查看两个空引用。此轮不运行 PIE（在编辑器中运行）、模拟输入或功能测试，编译与类型加载检查不能替代对原有移动和武器表现的运行验证。


## BossCharacter 的构造初始化

`ABossCharacter` 继承 `ACharacterBase`，在构造函数调用 `CreateDefaultSubobject<UAbilitySystemComponentBase>` 和 `CreateDefaultSubobject<UAttributeSetBase>`，分别赋给继承的 `AbilitySystemComponent`、`AttributeSet` 指针。两个对象均以 Boss 角色为 Outer（所属对象），随角色对象管理；基类的查询接口直接返回这两个引用。

默认子对象会参与 Unreal 的类默认对象、蓝图派生和实例构造流程，所以在构造函数创建，而不使用普通 C++ `new`。这里的 AttributeSet 是 UObject（引擎对象），不是 ActorComponent（Actor 组件），因此不应以是否出现在组件列表中判断它是否创建成功。

本次没有创建 Boss 蓝图、选择网格/动画、将 Boss 放入地图、修改玩家角色，或新增 AI（人工智能）、ActorInfo（能力系统角色信息）初始化、复制策略和技能授予。下一步接入实际 GAS 运行逻辑时再处理相应生命周期。

编辑器中可创建以 BossCharacter 为父类的 Blueprint Class（蓝图类），在组件面板查看 AbilitySystemComponent，在继承的 Abilities（能力）属性中查看两个引用。本轮仅做编译和加载检查，不主动创建资源、运行 PIE（在编辑器中运行）或执行功能测试。
