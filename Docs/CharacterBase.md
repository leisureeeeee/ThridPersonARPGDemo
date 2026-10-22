# 角色与 GAS 基类

本轮只增加 C++ 基类、指针和继承关系，不实现具体战斗能力、属性或 PlayerState（玩家状态）。

```text
ACharacter
  └─ ACharacterBase : IAbilitySystemInterface
       └─ AMyCharacter

UAbilitySystemComponent
  └─ UAbilitySystemComponentBase

UAttributeSet
  └─ UAttributeSetBase
```

`ACharacterBase` 声明 `AbilitySystemComponent` 和 `AttributeSet` 两个受保护指针，类型分别为两个 GAS 基类。受保护是为了后续角色子类从实际拥有者取得引用；蓝图可读取但不能直接写入。指针使用 `UPROPERTY` 与 `TObjectPtr`，会被 Unreal 的垃圾回收识别；`Transient` 表示不把运行时引用保存进资源。

指针默认均为空，基类不调用 `CreateDefaultSubobject` 创建 GAS 实例。保存引用与决定实例的拥有者是两个职责：以后玩家仍可由 PlayerState 创建具体 ASC（能力系统组件）及属性集，MyCharacter 只引用它们；其他具体角色也可选择自己的拥有方式。本轮不增加 PlayerState 类型或绑定生命周期。

`IAbilitySystemInterface` 是 GAS 用于从角色查询能力系统组件的引擎接口。`GetAbilitySystemComponent()` 返回上述 ASC 指针，`GetAttributeSet()` 提供属性集读取入口。因为本轮尚未赋值，返回空指针是预期行为；调用方需要检查有效性。

三个基类均标记 `Abstract`（抽象类），供具体子类派生；`AMyCharacter` 保持可实例化。ASC 基类不预设复制策略、能力授予或激活逻辑，属性集基类不声明生命值等实际属性，也不包含结算规则。

MyCharacter 原有相机、武器和移动实现留在原类，只把直接父类改成 CharacterBase。`ACharacterBase` 将 `FObjectInitializer` 原样传给 `ACharacter`，所以 MyCharacter 原有的 `SetDefaultSubobjectClass<UARPGCharacterMovementComponent>` 仍可指定自定义移动组件。角色蓝图的直接父类仍是 MyCharacter，新增基类位于其 C++ 继承链中，无需手动重新指定蓝图父类。

为了让头文件引用 GAS 类型，项目启用 GameplayAbilities 插件，并增加 GameplayAbilities、GameplayTags、GameplayTasks 模块依赖。模块/插件可用不表示已经创建或初始化 ASC 实例。

新增反射类型和原生父类调整需要完整 C++ 编译并重新加载编辑器；实际结果见 `Docs/ExecutionLog.md`。在角色蓝图的 Class Settings（类设置）中仍应看到 MyCharacter；可以在继承属性中查看两个空引用。此轮不运行 PIE（在编辑器中运行）、模拟输入或功能测试，编译与类型加载检查不能替代对原有移动和武器表现的运行验证。
