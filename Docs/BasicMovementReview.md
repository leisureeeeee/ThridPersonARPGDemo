# 基础移动审查：先确认输入、位移和转向

检查日期：2026-10-20（北京时间）。本次为静态代码与编辑器属性检查，没有编译、PIE、播放或功能测试，没有修改运行时代码或资产。

## 当前阶段的结论

现有项目已经具备第三人称自由移动所需的基础结构。先保留这条链路，在编辑器确认移动手感，再接最简单的待机/行走动画。当前无需为移动引入完整 Lyra、ALS 或运动匹配框架。

```text
WASD
  → AMyPlayerController：Enhanced Input（增强输入）绑定和二维输入
  → 根据基础相机的水平朝向计算世界空间方向
  → APawn::AddMovementInput：提交移动意图
  → UCharacterMovementComponent：加速、位移、碰撞、制动和转向
  → 骨骼网格体跟随角色；之后由动画实例读取真实速度驱动动画
```

## 已有实现

- `Source/ThirdPersonARPG/PlayerController/MyPlayerController.cpp`：`SetupInputComponent()` 创建、绑定并注册输入；`HandleMoveInput()` 每次读取当前控制的 Pawn，不缓存可能因重新附身而过期的角色。
- `CreateInputMappingContext()`：D/A 对应二维输入 X 的正/负，W/S 经轴交换对应 Y 的正/负。
- `HandleMoveInput()`：先用 `GetClampedToMaxSize(1.0f)` 限制斜向输入幅度；调用 `CalcCamera()` 获取角色基础相机朝向，只保留 Yaw（偏航角），把前后/左右输入转为世界方向并提交 `AddMovementInput()`。基础相机可包含作者设置的局部旋转，和直接使用控制器朝向的做法有明确区别。
- `Source/ThirdPersonARPG/MyCharacter/MyCharacter.cpp`：关闭 `bUseControllerRotationYaw` 和 `bUseControllerDesiredRotation`，开启 `bOrientRotationToMovement`，`RotationRate.Yaw = 500`。默认实现朝加速度方向转身，反向输入时该方向可能不同于当前速度方向。
- `Source/ThirdPersonARPG/GameMode/ARPGGameMode.cpp`：指定 `AMyPlayerController` 和 `AMyCharacter`。角色又设置了 `AutoPossessPlayer = Player0`；后续若统一改为 GameMode（游戏模式）生成角色，应再明确摆放角色和默认出生角色的使用方式，当前没有进入 PIE，因此没有确认运行时实际生成/附身链路。

`AddMovementInput()` 提交的是输入比例，不是直接设置位置，调用处不应额外乘以帧时间。角色移动组件负责按帧时间积分并处理碰撞。

## 编辑器中场景角色的实际参数

只读查询对象：`/Game/NewMap.NewMap:PersistentLevel.BP_Character_C_1`。这是编辑器实例的属性值，不能替代运行时验证。

| 属性 | 当前值 | 作用 |
|---|---:|---|
| `MaxWalkSpeed` | 600 cm/s | 最大地面移动速度，名称包含 Walk，但不代表动画一定是步行 |
| `MaxAcceleration` | 2048 cm/s² | 输入驱动速度改变的上限 |
| `BrakingDecelerationWalking` | 2048 cm/s² | 无输入时的恒定制动减速度 |
| `GroundFriction` | 8 | 地面方向控制与默认制动摩擦的相关参数 |
| `BrakingFrictionFactor` | 2 | 制动摩擦系数倍率 |
| `bUseSeparateBrakingFriction` | false | 当前制动使用地面摩擦，不使用单独的制动摩擦参数 |
| `RotationRate.Yaw` | 500°/s | 朝目标方向转身的角速度上限 |

不能仅凭速度和恒定减速度推算实际停步距离；制动还受摩擦等因素影响。

## 与成熟项目的基本结构对照

本机 `G:/UEProject/LyraStarterGame/Source/LyraGame/Character/LyraHeroComponent.cpp` 的 `Input_Move()` 同样把二维输入按水平朝向转换为世界方向，然后调用 `Pawn->AddMovementInput()`。它使用控制器的水平朝向；本项目使用基础相机朝向。核心移动仍由角色移动组件执行。

Lyra 将输入封装到 HeroComponent（英雄组件），本项目按照已选定架构将输入封装在 PlayerController（玩家控制器）。当前规模下这两者不需要统一；值得借鉴的是输入、移动模拟、动画表现分离的职责边界。Lyra 默认射击角色面向控制器，其默认转向参数不能直接套用到自由朝向角色。

此前同时查阅了 ALS-Refactored 的 `UAlsAnimationInstance`：它区分游戏线程的数据收集与线程安全动画计算，并使用独立设置数据；这些是后续动画阶段的参考，本次不引入这些层次。

参考：

- Epic 官方：<https://www.unrealengine.com/tech-blog/adapting-lyra-animation-to-your-ue5-game>
- ALS-Refactored 原始代码：<https://github.com/Sixze/ALS-Refactored/blob/main/Source/ALS/Public/AlsAnimationInstance.h>
- 官方游戏动画示例：<https://dev.epicgames.com/documentation/en-us/unreal-engine/game-animation-sample-project-in-unreal-engine>

## 下一步只做最小移动表现

1. 用户在编辑器运行后，先确认 W 朝镜头水平前方移动、A/D 侧向移动时角色转向，斜向没有额外提速；转动镜头但不输入移动时角色不跟着原地转。
2. 确认松键能减速停下；180°反向时可以先接受基础转向表现，暂不把它作为高级急转动画验收标准。
3. 根据目标步行速度调整 `MaxWalkSpeed`，只逐项调整加速度、制动或转速。600 cm/s 与现有步行动画是否匹配尚未测量，不能直接作为最终参数。
4. 动画阶段先做一个 C++ `UAnimInstance` 子类收集 `Speed2D` 和 `bIsInAir`，薄的 Animation Blueprint（动画蓝图）只组装 Idle（待机）/Walk（行走）姿势。移动组件继续独立决定真实位移，不在动画代码中调用移动。

## Katana 资源前置检查（本次不修复）

- `/Game/Katana_Animations` 注册表中共 259 项：246 个动画序列、13 个重定向器。动画分类为待机 4、攻击 134、行走 108；行走包含起步、循环、停步各 36 个。该统计来自路径与名称，不等于动作视觉效果验证。
- 245 个动画序列的注册表骨架标签指向 `/Game/Katana_Animations/Demo/Mannequin_UE4/Character/Mesh/UE4_Mannequin_Skeleton`，其资产存在性查询为 false；`AS_Idle_Combat_Seq` 加载报骨架缺失，并在随后的注册表查询中显示骨架 None。尚未逐个加载其余 245 个序列。
- 当前场景角色使用 `ARPG_Samurai` 的 `SK_Mannequin`；动画模式为 Animation Blueprint（动画蓝图），但 `AnimClass` 与动画实例均为空。
- 前向普通行走循环注册表时长为 1.066667 秒，`bEnableRootMotion` 为 false，曲线和同步标记列表为空。这不能证明源根骨骼没有位移；本次根运动与脚步测量因骨架问题未完成。
- 接动画前应先恢复原始骨架依赖，再检查与目标角色的兼容性；不能只给残缺动画随意指定一个同名骨架来推定重定向已正确。

## 检查限制

- 未编译、未测试、未运行 PIE，未改动源文件、配置和二进制资产。
- 项目规则要求路径输入使用 see 技能；本会话技能目录、已安装插件目录及 Claude 技能目录未找到 `see/SKILL.md` 或 `see.sh`。本次没有请求视觉判断，使用 Unreal MCP 的资产注册表、属性和源码完成检查，不声称已预览动画。
