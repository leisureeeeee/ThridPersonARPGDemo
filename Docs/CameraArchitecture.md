# ARPG 摄像机架构提案

状态：设计建议，尚未实现。适用于当前 PlayerController 接收输入、Character 执行角色行为的架构。

## 当前开局视角

- `ARPGGameMode` 指定 `MyPlayerController` 和 `MyCharacter`。
- 本地玩家控制器控制目标 Pawn 后，UE 默认自动管理 ViewTarget，以该 Pawn 的激活摄像机组件计算视角。
- 当前 `MyCharacter` 已有 `FollowCamera` 和 `SpringArm`。应先确认玩家实际控制了预期角色、摄像机处于激活状态、关卡没有优先自动激活的独立 CameraActor。
- 若使用关卡内的角色蓝图，检查该实例的 Auto Possess Player；若由 GameMode 生成角色，检查 Default Pawn Class 指向预期的角色类或蓝图。还需确认关卡 World Settings 未覆盖预期 GameMode。
- 手动切换时，将角色 Actor 交给 PlayerController 的 SetViewTarget / SetViewTargetWithBlend；不要将 UCameraComponent 当作 ViewTarget，也不要每帧重复设置目标。需要在角色控制关系建立之后操作。

当前代码中，`SpringArm->bUsePawnControlRotation = true` 表示弹簧臂使用 Pawn 的视角朝向；`FollowCamera->bUsePawnControlRotation = false` 表示摄像机不再自行套用控制朝向，而是沿用弹簧臂末端的变换。两者组成同一条旋转链。当前控制器仅绑定 WASD，尚未绑定鼠标视角输入；这些设置本身不会产生鼠标转向。

## 职责划分

| 层 | 建议类型 | 职责 |
| --- | --- | --- |
| 玩家输入 | `AMyPlayerController` | 接收视角、瞄准、锁定等输入，发出意图；配置 PlayerCameraManagerClass。 |
| 玩家最终视角 | `AARPGPlayerCameraManager` | 当前 ViewTarget、角色与演出镜头间的切换、视角限制及 CameraModifier / CameraShake；沿用 UE 输出流程。 |
| 角色摄像机装置 | `UARPGCameraRigComponent` | 持有对当前 SpringArm 和 FollowCamera 的引用，接收模式请求，选择并过渡基础模式，统一应用摄像机装置参数。 |
| 基础镜头模式 | `UARPGCameraMode` 及按需派生类 | 根据角色与锁定目标等上下文，计算跟随、瞄准、锁定所需的距离、偏移、FOV、朝向等参数。 |
| 角色与玩法组件 | `AMyCharacter`、战斗和锁定组件 | 提供角色锚点、速度、瞄准状态和目标；请求镜头行为，不直接改动摄像机内部状态。 |

推荐数据流：输入或玩法状态 → 摄像机模式请求 → RigComponent 选择/混合参数 → SpringArm 与 FollowCamera 计算基础视角 → PlayerCameraManager 应用切换和修饰效果 → 玩家画面。

## 边界与生命周期

1. 近阶段继续由 SpringArm 负责基础跟随碰撞与其已有的延迟行为。RigComponent 通过明确的 Tick 依赖在 SpringArm 更新前应用参数。若未来改为模式直接计算最终相机位置，应同时替换相应的 SpringArm 计算职责，保持基础视角只有一个计算来源。
2. 玩法代码通过模式请求操作相机，例如 `RequestCameraMode(Mode, Priority, Owner)` 返回请求句柄，结束时使用 `ReleaseCameraMode(Handle)` 释放。瞄准、技能、锁定只能释放各自的请求；模式结束后重新选择仍有效的最高优先级请求，并平滑过渡。
3. 基础构图与叠加效果分开：跟随、瞄准、锁定作为基础模式；受击震动、后坐力等交给独立修饰器处理。提前规定各层允许修改哪些参数，避免同一参数被多个不协调的系统反复覆盖。
4. CameraManager 属于具体玩家的控制器。角色装置属于当前 Pawn；控制目标切换或销毁时解除旧引用及请求，不用静态全局对象保存所有玩家共用的镜头状态。
5. 摄像机层通过组件或明确上下文读取目标，不依赖 `AMyCharacter` 的内部字段。初期用具体组件和窄接口即可，出现多类目标后再评估统一目标接口。
6. 输入的控制朝向与最终画面效果保持区别：移动使用稳定的参考朝向，不使用震动后的最终相机旋转作为移动方向。
7. 过场交给 Sequencer / CineCameraActor，CameraManager 协调 ViewTarget 交接。结束或中断后恢复到当前有效 Pawn，而非保存已可能销毁的旧角色。

## 建议逐步落地

第一步新增 CameraManager 和 RigComponent，保持当前第三人称视角行为，先把摄像机参数修改集中起来。第二步在加入瞄准或锁定时引入 CameraMode 和过渡管理。第三步随技能演出需求加入请求优先级、修饰器及过场交接；无需现在一次实现所有模式或复制完整 Lyra 初始化框架。

建议目录：`Camera/ARPGPlayerCameraManager.*`、`Camera/ARPGCameraRigComponent.*`，模式增多后增加 `Camera/Modes/`。

## 实现后的验证方式

- 使用 Play 而非 Simulate 启动；检查实际控制器与角色类型，确认开局 ViewTarget 为期望角色且只有预期摄像机激活。
- 可在游戏视口控制台使用 `showdebug camera` 辅助观察摄像机信息；本次仅提供验证方法，未实际运行验证。
- 基础跟随阶段检查摄像机跟随角色、靠墙缩短距离及离墙恢复。
- 加入模式后检查进入与退出瞄准的平滑过渡，以及瞄准期间触发技能、技能结束后仍恢复瞄准的情况。
- 加入镜头切换后检查角色销毁、重生和过场中断时的目标恢复；新增锁定和镜头震动后，检查移动方向仍使用稳定参考朝向。

## 依据与选型

- [UE 摄像机概览](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/cameras-in-unreal-engine)：CameraComponent、ViewTarget、PlayerCameraManager 的职责。
- [自动摄像机目标管理](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerController/AutoManageActiveCameraTarget)：自动激活 CameraActor 的优先行为。
- [PlayerCameraManager API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerCameraManager)：目标视角、修饰器与最终视角数据。
- [Lyra 技能与摄像机模式](https://dev.epicgames.com/documentation/en-us/unreal-engine/abilities-in-lyra-in-unreal-engine)：技能可设置及清理 CameraMode。本提案参考职责分离思路，不声称与 Lyra 内部实现一致。
- [Gameplay Camera System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-camera-system)：可另行评估的数据驱动方案。当前官方 UE 5.8 文档和本机 GameplayCameras.uplugin 均标记为 Experimental；本提案不要求替换当前摄像机基础设施。
