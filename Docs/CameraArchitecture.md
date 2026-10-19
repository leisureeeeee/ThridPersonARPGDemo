# ARPG 摄像机架构与实现

状态：第一阶段已实现 CameraManager、CameraRigComponent、参数平滑过渡及受控角色视角接线。CameraMode、模式请求优先级、瞄准与锁定仍为后续设计。适用于当前 PlayerController 接收输入、Character 执行角色行为的架构。

## 当前开局视角

- `ARPGGameMode` 指定 `MyPlayerController` 和 `MyCharacter`。
- 本地玩家控制器控制目标 Pawn 后，UE 默认自动管理 ViewTarget，以该 Pawn 的激活摄像机组件计算视角。
- 当前 `MyCharacter` 已有 `FollowCamera` 和 `SpringArm`。应先确认玩家实际控制了预期角色、摄像机处于激活状态、关卡没有优先自动激活的独立 CameraActor。
- 当前项目重写了 `MyPlayerController::AutoManageActiveCameraTarget`：开启自动管理且目标为当前带 Rig 的受控 Pawn 时，由自定义 CameraManager 优先选择该 Pawn。其他情况沿用 UE 默认流程；手动过场需要独占视角时，可关闭自动管理再设置 ViewTarget。
- 若使用关卡内的角色蓝图，检查该实例的 Auto Possess Player；若由 GameMode 生成角色，检查 Default Pawn Class 指向预期的角色类或蓝图。还需确认关卡 World Settings 未覆盖预期 GameMode。
- 手动切换时，将角色 Actor 交给 PlayerController 的 SetViewTarget / SetViewTargetWithBlend；不要将 UCameraComponent 当作 ViewTarget，也不要每帧重复设置目标。需要在角色控制关系建立之后操作。

当前代码中，`SpringArm->bUsePawnControlRotation = true` 表示弹簧臂使用 Pawn 的视角朝向；`FollowCamera->bUsePawnControlRotation = false` 表示摄像机沿用弹簧臂末端的变换。控制器现已绑定 WASD 和鼠标视角：Mouse2D → LookAction → AddYawInput / AddPitchInput → UE 更新 ControlRotation → SpringArm → FollowCamera。蓝图中的 SpringArm 应保持 Use Pawn Control Rotation、Inherit Pitch、Inherit Yaw 开启；摄像机与弹簧臂之间若有人为旋转偏移，基础相机朝向可能不同于 ControlRotation。

## 移动与鼠标输入

- 输入动作与映射继续由 `MyPlayerController` 用 C++ 在运行时创建，保留在私有 Transient 成员中；无需编辑器 IA / IMC 资产。移动动作 Axis2D 约定 X 为左右、Y 为前后：W=(0,1)、S=(0,-1)、A=(-1,0)、D=(1,0)。键盘原始输入落在 X，W/S 经 Swizzle 交换到 Y，A/S 使用 Negate 反号。
- `HandleMoveInput` 通过当前 Pawn 的标准 `CalcCamera` 接口读取其基础视角，只保留 Yaw。用世界前向乘输入 Y、世界右向乘输入 X，再调用 `AddMovementInput`。因此 W 跟随角色基础相机的水平前方，相机俯仰不产生上下移动。输入长度限制为 1，避免斜走加速；相反方向按键通过 Cumulative 累加抵消。
- 这里读取 Pawn 基础视角，不读取 CameraManager 应用震动和修饰器后的最终旋转；过场镜头也不会接管角色的移动参考。当前实现读取最近更新的基础相机姿态，SpringArm 旋转延迟会体现在移动参考中；若未来玩法需要独立、无延迟的移动朝向，应在 CameraMode 阶段明确提供该参考。
- `LookAction` 是 Axis2D，直接绑定 Mouse2D；根据用户实际体验反馈，已移除原先额外的 Y 轴 Negate，鼠标 Y 原值乘灵敏度后交给 AddPitchInput，反转此前的上下操作。配置保持关闭 `bEnableLegacyInputScales`，防止 UE 旧输入比例叠加。`MouseSensitivity` 是控制器的私有 EditDefaultsOnly 参数，默认每单位鼠标位移 0.15 度，蓝图类默认值中可调整；鼠标位移不再乘 DeltaTime。
- 本地控制器 `BeginPlay` 设置 GameOnly 输入模式并隐藏光标。PIE 应使用 Play，点击游戏视口捕获鼠标；Shift+F1 可释放鼠标。处理器分别尊重移动/视角输入禁用状态，没有受控角色时不处理。
- Character 继续负责移动与组件装配，Rig 负责相机参数，CameraManager 负责视角输出；键位和输入事件仍封装在控制器中。

输入回归测试 `ThirdPersonARPG.Input.CameraRelativeMoveAndMouseLook` 使用真实控制器绑定、运行时映射修饰器和 Enhanced Input 注入，覆盖相机与控制器错开 90° 的 WASD、相机转向后的 W、俯仰时平面移动、斜向限速、反向抵消、鼠标方向和忽略输入。该测试不覆盖桌面鼠标捕获、关卡蓝图覆盖或实体硬件操作，仍需在项目 PIE 中验证。

2026-10-19 验证：在 `Saved/InputValidation_20261019_1732` 使用与主项目哈希一致的源码构建独立 Editor 模块，并以 UnrealEditor-Cmd、NullRHI 运行上述测试；报告为 Success，1 项测试通过、0 错误、0 警告。临时工程使用独立文件缓存，避免与当前编辑器共享 Zen 缓存服务；全部临时文件均被 Git 忽略。

## 职责划分

角色转身与模型朝向分别处理：`bUseControllerRotationYaw=false`、`bUseControllerDesiredRotation=false`、`bOrientRotationToMovement=true`，由 CharacterMovement 根据加速度方向和 RotationRate 转动角色胶囊。当前蓝图使用 UE4 `SK_Mannequin`，其模型前向需要相对胶囊做 Yaw=-90° 的对齐；在 `MyCharacter` 构造函数中设置 `GetMesh()->SetRelativeRotation(FRotator(0,-90,0))`，不用旋转 Actor、相机或移动输入来补偿模型偏移。

构造函数提供默认对齐，蓝图仍可针对不同模型调整 Mesh 的 Transform → Rotation。重新编译加载后，检查角色蓝图 Mesh 的相对 Rotation 是否为 `(0,0,-90)`（编辑器按 X/Roll、Y/Pitch、Z/Yaw 显示），并确认人物正面朝向胶囊的前向箭头。若已有蓝图覆盖，则将该 Rotation 重置为父类默认值。换成原本面向 +X 的模型时，应将 Mesh 相对 Yaw 调回 0。此调整未运行编译或测试，遵照用户要求由用户验证。

模型高度也需相对胶囊对齐：当前模型原点在脚下，胶囊原点在中心。`MyCharacter` 构造函数将 Mesh 的相对位置设为 `(0,0,-CapsuleHalfHeight)`；默认胶囊半高为 88 cm，因此默认 Mesh Z=-88。胶囊负责落地碰撞，模型脚底对齐胶囊底部，避免胶囊已站在地上而模型仍悬空。这里设置的是 Mesh，不移动 Actor、相机或弹簧臂。蓝图已有位置覆盖时，将 Mesh → Transform → Location 重置为父类默认值；改变胶囊半高或换用不同原点的模型后，应在蓝图重新对齐脚底。本次未编译或运行，实际落地效果由用户验证。

| 层 | 建议类型 | 职责 |
| --- | --- | --- |
| 玩家输入 | `AMyPlayerController` | 接收视角、瞄准、锁定等输入，发出意图；配置 PlayerCameraManagerClass。 |
| 玩家最终视角 | `AARPGPlayerCameraManager` | 当前 ViewTarget、角色与演出镜头间的切换、视角限制及 CameraModifier / CameraShake；沿用 UE 输出流程。 |
| 角色摄像机装置 | `UARPGCameraRigComponent` | 持有对当前 SpringArm 和 FollowCamera 的引用，统一应用摄像机装置参数并平滑过渡；后续在这一层扩展模式请求与选择。 |
| 基础镜头模式 | `UARPGCameraMode` 及按需派生类 | 根据角色与锁定目标等上下文，计算跟随、瞄准、锁定所需的距离、偏移、FOV、朝向等参数。 |
| 角色与玩法组件 | `AMyCharacter`、战斗和锁定组件 | 提供角色锚点、速度、瞄准状态和目标；请求镜头行为，不直接改动摄像机内部状态。 |

推荐数据流：输入或玩法状态 → 摄像机模式请求 → RigComponent 选择/混合参数 → SpringArm 与 FollowCamera 计算基础视角 → PlayerCameraManager 应用切换和修饰效果 → 玩家画面。

## 边界与生命周期

1. 近阶段继续由 SpringArm 负责基础跟随碰撞与其已有的延迟行为。RigComponent 通过明确的 Tick 依赖在 SpringArm 更新前应用参数。若未来改为模式直接计算最终相机位置，应同时替换相应的 SpringArm 计算职责，保持基础视角只有一个计算来源。
2. 玩法代码通过模式请求操作相机，例如 `RequestCameraMode(Mode, Priority, Owner)` 返回请求句柄，结束时使用 `ReleaseCameraMode(Handle)` 释放。瞄准、技能、锁定只能释放各自的请求；模式结束后重新选择仍有效的最高优先级请求，并平滑过渡。
3. 基础构图与叠加效果分开：跟随、瞄准、锁定作为基础模式；受击震动、后坐力等交给独立修饰器处理。提前规定各层允许修改哪些参数，避免同一参数被多个不协调的系统反复覆盖。
4. CameraManager 属于具体玩家的控制器。角色装置属于当前 Pawn；控制目标切换或销毁时解除旧引用及请求，不用静态全局对象保存所有玩家共用的镜头状态。
5. 摄像机层通过组件或明确上下文读取目标，不依赖 `AMyCharacter` 的内部字段。初期用具体组件和窄接口即可，出现多类目标后再评估统一目标接口。
6. 输入的控制朝向与最终画面效果保持区别：移动使用当前 Pawn 的基础相机水平朝向，不使用 CameraManager 震动后的最终相机旋转作为移动方向。
7. 过场交给 Sequencer / CineCameraActor，CameraManager 协调 ViewTarget 交接。结束或中断后恢复到当前有效 Pawn，而非保存已可能销毁的旧角色。

## 已实现的第一阶段

- `AMyCharacter` 创建并保存 CameraRig 组件，在 `PostInitializeComponents` 绑定原有 SpringArm 与 FollowCamera。此时蓝图构造已经完成，因此 Rig 捕获的是实际调好的默认距离、偏移和 FOV。
- `FARPGCameraRigSettings` 是参数值对象；Rig 的公开接口为 `GetCurrentCameraSettings`、`SetCameraSettings`、`ResetCameraSettings`。外部通过参数副本提出调整，不直接访问组件内部指针。
- 调整时从当前实际参数出发，以指定时长执行 SmoothStep 插值；中途再次调整会以当前视角参数作为新的起点。过渡结束后关闭 Rig 自身 Tick。SpringArm 的碰撞检测和原有更新继续工作。
- Rig 建立 SpringArm 对自身的 Tick 前置依赖，确保先更新参数，再计算弹簧臂的最终位置，避免参数晚一帧生效。
- `AMyPlayerController` 通过 `PlayerCameraManagerClass` 使用新的 Manager。Manager 不覆写最终 POV 计算，沿用 UE 的 CameraComponent、镜头切换及修饰器流程。
- `RestoreControlledPawnCamera` 在调用时查找控制器当前 Pawn，支持从其他镜头恢复；不缓存可能已经销毁的旧角色。它不是完整的 Sequencer 中断管理器。

参数调用示例（角色组件初始化之后）：

```cpp
if (UARPGCameraRigComponent* Rig = Pawn->FindComponentByClass<UARPGCameraRigComponent>())
{
    FARPGCameraRigSettings Settings = Rig->GetCurrentCameraSettings();
    Settings.ArmLength = 200.0f;
    Settings.FieldOfView = 70.0f;
    Rig->SetCameraSettings(Settings, 0.25f);
}
```

效果结束时单独调用 `Rig->ResetCameraSettings(0.25f)`，恢复该角色初始化时的默认参数。当前 setter 是一次参数调整，不是已经实现的瞄准模式或多请求优先级系统。

第二步在加入瞄准或锁定时引入 CameraMode 和过渡管理。第三步随技能演出需求加入请求优先级、修饰器及过场交接；无需现在一次实现所有模式或复制完整 Lyra 初始化框架。

建议目录：`Camera/ARPGPlayerCameraManager.*`、`Camera/ARPGCameraRigComponent.*`，模式增多后增加 `Camera/Modes/`。

## 实现后的验证方式

最初摄像机基础层与输入实现的独立游戏目标 `ThirdPersonARPG / Win64 / Development` 已通过完整编译和链接，当时主项目 Editor 的 DLL 链接被运行中的 UE 占用；临时工程自动化结果见上文。后续 Mesh 朝向对齐与鼠标 Y 方向修正遵照用户要求未编译、未运行测试，已有自动化代码仅同步调整对应的 Pitch 期望值，不代表当前版本已通过。实际项目画面、鼠标捕获和参数过渡由用户重新编译加载后验证。

- 使用 Play 而非 Simulate 启动；检查实际控制器与角色类型，确认开局 ViewTarget 为期望角色且只有预期摄像机激活。
- 可在游戏视口控制台使用 `showdebug camera` 辅助观察摄像机信息。
- 点击游戏视口后，检查鼠标左右转向、上下俯仰；将相机转动约 90°，分别按 WASD，预期沿画面前后左右移动。抬头/低头时仍在地面移动，W+D 不比单按 W 更快，W+S 停止输入。可用 `showdebug enhancedinput` 查看移动动作：单按 W 应为 (0,1)，D 为 (1,0)。若移动方向正确而模型侧身，检查 SkeletalMesh 相对 Capsule 的朝向，模型朝向与移动向量是两项独立设置。
- 可在角色蓝图 BeginPlay 中临时取得 CameraRig，复制当前参数并将 ArmLength 设为 200、FOV 设为 70，调用 SetCameraSettings（0.25 秒），等待后调用 ResetCameraSettings（0.25 秒）。预期距离和视野平滑变化并回到原有参数；验证后删除临时测试节点。
- 基础跟随阶段检查摄像机跟随角色、靠墙缩短距离及离墙恢复。
- 加入模式后检查进入与退出瞄准的平滑过渡，以及瞄准期间触发技能、技能结束后仍恢复瞄准的情况。
- 加入镜头切换后检查角色销毁、重生和过场中断时的目标恢复；新增锁定和镜头震动后，检查移动方向仍使用稳定参考朝向。

## 依据与选型

- [Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)：Axis2D、运行时映射、Negate / Swizzle 及输入注入。
- [UE 摄像机概览](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/cameras-in-unreal-engine)：CameraComponent、ViewTarget、PlayerCameraManager 的职责。
- [自动摄像机目标管理](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerController/AutoManageActiveCameraTarget)：自动激活 CameraActor 的优先行为。
- [PlayerCameraManager API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerCameraManager)：目标视角、修饰器与最终视角数据。
- [Lyra 技能与摄像机模式](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)：技能可设置及清理 CameraMode。本提案参考职责分离思路，不声称与 Lyra 内部实现一致。
- [Gameplay Camera System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-camera-system)：可另行评估的数据驱动方案。当前官方 UE 5.8 文档和本机 GameplayCameras.uplugin 均标记为 Experimental；本提案不要求替换当前摄像机基础设施。
