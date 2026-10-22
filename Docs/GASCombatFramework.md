# C++ GAS 战斗框架

本轮只建立 Gameplay Ability System（游戏能力系统，GAS）的基础设施与扩展契约。生命值和招架条供双方共用，能量供玩家使用；敌人可以不使用能量属性。没有实现任何实际战斗动作、伤害判定、恢复计时、处决动画或能量消耗。

## 类型与职责

| 类型 | 当前职责 |
| --- | --- |
| `UARPGAbilitySystemComponent` | 持有属性集；初始化 Owner/Avatar（拥有者/表现角色）；服务端一次性应用初始属性效果及授予能力；四动作请求与服务端事件分发 |
| `UARPGCombatAttributeSet` | `Health/MaxHealth`、`Posture/MaxPosture`、`Energy/MaxEnergy`；属性复制通知与数值边界保护 |
| `UARPGCombatAbility` | 能力基类，实例策略、网络执行策略、公共阻止标签；遗漏实现时取消结束，避免空技能一直激活 |
| `UARPGAbility_NormalAttack` | 普通攻击能力扩展点 |
| `UARPGAbility_SpecialAttack` | 特殊攻击能力扩展点 |
| `UARPGAbility_Parry` | 招架能力扩展点 |
| `UARPGAbility_Dodge` | 躲避能力扩展点 |
| `UARPGAbility_Execution` | 处决能力扩展点；预留服务端 `Execution.Requested` 事件触发器 |
| `UARPGCombatDefinition` | 玩家/敌人可分别配置的 Data Asset（数据资产）；初始能力、初始属性效果、战斗参数、效果类型引用 |
| `ARPGCombatTags` | 原生注册的能力、状态、事件、SetByCaller（调用方传值）标签 |

当前 `AMyCharacter` 通过组合添加 ASC（能力系统组件）并实现 `IAbilitySystemInterface`，使 GAS 可以从角色查到组件。组件自己的 `BeginPlay` 初始化角色信息；角色的 `PossessedBy`、`OnRep_Controller` 在控制关系变化后刷新信息。重复刷新不会重复授予能力或重置属性。

Owner 和 Avatar 都指向当前角色，能力与属性随角色生命期存在。未引入 PlayerState（玩家状态）持久能力方案；以后若要求死亡换角色后保留能力，再专门迁移。其他敌人 Character（角色）可以组合相同组件并实现同一引擎接口，不必继承玩家相机或输入代码。

## 当前可用的基础设施

- 六个属性已注册，当前值限制在 0 到对应上限之间，上限至少为 1。生命/招架/能量上限的临时变化也会约束当前值。默认生命 100、招架 0、能量 0，上限均为 100，只作框架占位。
- 服务端读取配置并授予非抽象的能力类；初始属性效果必须为 Instant（即时）。初始效果先设置最大值，再设置当前值，防止当前值提前被上限截断。
- `RequestCombatAction` 只接受四个动作标签，通过 GAS 尝试激活已授予的匹配能力。返回值表示激活请求被接受，不表示命中或执行完成。普通攻击等五个原生占位类都标为抽象，不能直接当作已完成能力授予。
- `DispatchCombatEvent` 只在服务端本地分发 `ARPG.Event` 下的事件，复制有效载荷并统一 `EventTag`。它不是远程调用，也不会自动判断招架、寻找目标或触发处决。
- 属性复制与基础执行策略已声明；没有进行多人、预测或延迟验证。ASC 使用 Minimal（最小）效果复制模式；属性复制独立于效果复制。

没有创建或指定 `CombatDefinition` 资源，因此默认不会授予技能或应用效果。没有改控制器输入、地图、角色蓝图、动画蓝图或武器行为。空配置时原有移动、拔收刀逻辑继续走原来的路径。

## 后续机制契约（未实现）

| 机制 | 后续归属和约定 |
| --- | --- |
| 攻击命中 | 服务端命中结算层验证目标与有效命中；事件中 Instigator 为攻击者、Target 为受击者。通过 Gameplay Effect（游戏效果）修改属性，不由控制器直接减血 |
| 招架抬手窗口 | 招架能力/动画通知负责打开和关闭 `State.Parry.PerfectWindow`，并发出对应窗口事件；状态只描述事实，本轮没有开启窗口 |
| 完美招架 | 命中结算层读取受击者窗口状态；自身招架条增量为 0，向攻击者施加较大的招架条增量。`Event.PerfectParry` 发送给防御者 ASC：Instigator=防御者、Target=原攻击者 |
| 招架后摇 | 招架能力管理普通后摇与完美招架后摇；收到完美招架事件后缩短等待，并在出口清理恢复状态。本轮没有计时器、取消关系或动作互斥 |
| 招架条满 | 服务端结算层检测上限并标记 `State.PostureBroken`；发出破防事件，再向合法处决者发送 `Event.Execution.Requested`，Instigator=处决者、Target=破防者 |
| 处决 | 处决能力验证目标、占用双方、等待动画完成；完成时清空目标招架条并大量恢复处决者能量。中断、死亡、失效目标必须有清理出口，事件本身不代表处决成功 |
| 特殊攻击 | 特殊攻击能力配置并提交 Cost Gameplay Effect（消耗效果），确认能量足够后执行；预留 `SpecialAttackCostEffect` 不等于已把 GAS 的 CostGameplayEffectClass 配好 |
| 招架条衰减 | 双方共用规则：受到有效攻击后重置延迟；超过 `PostureDecayDelay` 才按 `PostureDecayPerSecond` 衰减。即使完美招架自身增量为 0，也要记录本次受攻击时刻，不能只监听属性值变化 |

`FARPGCombatTuning` 保留完美窗口、普通/完美招架后摇、衰减延迟与速率、完美招架施加的招架压力、处决能量奖励。全部默认 0，尚未消费这些参数。招架条是“累计压力”：0 表示恢复，满值表示破防，不能按生命值的方向理解。

`UARPGCombatDefinition` 的命中、完美招架、招架条衰减、处决清条、处决回能、特殊攻击消耗六个效果引用仅为扩展槽；当前代码没有自动应用它们。`ARPG.Data.HealthDamage/PostureDamage/EnergyAmount` 是将来给效果传值的约定，也没有内置数值计算。

## 后续接入顺序

1. 在 C++ 中继承对应动作能力并重写 `ActivateAbility`。实现成功路径的 `CommitAbility`、任务等待及所有完成/取消路径的 `EndAbility`；不要调用占位基类的 `ActivateAbility`，它会立即取消。
2. 配置初始属性效果与玩家/敌人数据资产，把具体能力类填入初始能力列表；同一动作标签只授予一个入口能力。效果引用仍需具体能力或结算代码显式读取使用。
3. 实现伤害/招架结算和双方状态，再实现完美招架窗口、处决与衰减。事件和状态的有效性由服务端规则层保障，不能信任客户端目标或伤害。
4. 最后在现有 PlayerController（玩家控制器）构建输入映射，通过 `AMyCharacter::RequestCombatAction` 传动作标签。角色不依赖按键或 Enhanced Input（增强输入）类型；现有左/右键拔收刀保留，实际战斗键位另行安排。
5. 设置动画资产、窗口通知、UI（用户界面）属性监听与表现。本轮没有选择动画、创建 HUD（抬头显示）或编写命中扫描。

## 本轮验证与编辑器查看

新增构造函数子对象与反射类型，需要完整编译后重新打开编辑器，不能仅依赖 Live Coding（实时编码）替换旧布局。实际构建/加载结果记录于 `Docs/ExecutionLog.md`。

重新加载后，在角色蓝图的组件面板可查看 `CombatAbilitySystem`，并在其 Combat（战斗）配置中查看数据资产入口。在编辑器创建数据资产时可查找 `ARPGCombatDefinition` 类；本轮不需要创建或运行它。默认没有配置技能，按键不会产生新的战斗效果，这属于预期范围。

未新增/执行自动化、PIE（在编辑器中运行）或其他功能测试；编译和类型加载检查仅证明框架能构建并被编辑器识别，不证明实际机制已经可玩。
