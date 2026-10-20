# 当前角色的基础移动动画接入

这是操作说明，尚未创建动画蓝图、混合空间、跳跃输入或其他运行逻辑。当前使用 UE 5.8，人物为 `SK_Unagi`，关联骨架为 `SKEL_Unagi`。

## 先理解各部分职责

当前 `MyPlayerController` 接收移动输入，通过 `AddMovementInput` 把移动意图交给角色。`CharacterMovementComponent`（角色移动组件）执行位移、加速、减速和转向，产生真实速度。

`AnimInstance`（动画实例）属于人物的 Skeletal Mesh Component（骨骼网格体组件）。指定 Animation Blueprint（动画蓝图）类后，网格组件会创建对应的动画实例；它读取当前角色的运动状态，计算混合参数并输出骨骼姿势。

整体调用关系为：

```text
玩家输入 → PlayerController（玩家控制器）
         → CharacterMovement（角色移动）产生速度
         → AnimInstance（动画实例）读取速度/离地状态
         → Blend Space / State Machine（混合空间/状态机）
         → Skeletal Mesh（骨骼网格体）显示动作
```

动画读取实际速度，不直接读取 W/A/S/D。这样松键减速、被阻挡、手柄小幅输入等情况都能得到相应动画，也保持控制器、角色运动与动画的职责边界。

当前 `MyCharacter.cpp` 已设置 `bOrientRotationToMovement = true`、`bUseControllerRotationYaw = false`。角色朝运动方向转身，因此初版采用按速度驱动的 Blend Space 1D（一维混合空间），无需先准备左移/右移/倒退整套动作。以后若增加锁定目标时的横移，再扩展方向参数和对应动画。

## 第一步：确认三个动作素材

最小起点是同一骨架上的 Idle（待机）、Walk（行走）、Run（跑步）三个循环动作。第一阶段只接这三个，跳跃作为后续阶段。

现有 `Asset/Unagi/Animations/SourceFBX/AN_Unagi_01`～`11` 已成功导入。2026-10-20 补充只读骨骼姿势采样后，01 的主体关键点在采样中静止，02～11 主要呈现突进、挥动和恢复等姿势变化，尚未确认所需的两套行走循环；这不等于完成了逐帧的完整角色预览。不要根据编号认定待机或跑步，也不要用短攻击动作凑成移动循环。详见 [收刀与持剑移动的素材评估和免费资源](SwordLocomotionResources.md)。

若缺少相应片段，可以通过 Max 导出原包 `Unagi_Idle.max`、`Unagi_Walk.max`、`Unagi_Run.max`，或者把其他骨架的动作通过 IK Retargeter（IK 重定向器）转换给当前角色。Max 转换目前仍缺可用 Max；也不能直接把旧 Mannequin（人体模型）的动作赋给不兼容的 `SKEL_Unagi`。

基础移动优先选择 In-place（原地）动画，让角色移动组件负责位移。若动作自带根骨位移，需要进一步选择 Root Motion（根运动）方案或处理根位移，避免视觉位移和角色移动驱动不一致。本次未检查各片段根位移，不统一建议打开根运动。

## 第二步：建立走跑混合空间

1. 在 Content Browser（内容浏览器）新建 Animation（动画）→ Blend Space 1D（一维混合空间），选择 `SKEL_Unagi`，命名 `BS_Unagi_Locomotion`。
2. 在 Axis Settings（坐标轴设置）中将轴名设为 `Speed`，单位按角色速度使用厘米/秒。
3. 可以暂用如下示例采样点；它们是演示值，不是已测得的原动画适配速度：

| Speed（速度） | 动画 |
| ---: | --- |
| 0 | 待机循环 |
| 150 | 行走循环 |
| 600 | 跑步循环 |

4. 轴最大值应覆盖角色实际最高移动速度；最终根据 `CharacterMovement` 的 `Max Walk Speed`（最大行走速度）和动作步频调整采样点，不能只沿用示例数值。
5. 在预览中沿轴移动，检查姿势能否平滑混合，并让地面移动所需的循环动画保持循环播放。

混合空间会根据速度计算各样本的权重。例如速度处在走路、跑步两个采样点之间时，输出是两种骨骼姿势的混合。它控制动作外观，不负责改变角色实际移动速度。

若键盘按住移动始终达到同一最高速度，角色最终会进入跑步区域；行走只可能出现在加减速途中。要稳定慢走，需要另外通过游戏逻辑改变最大移动速度或提供模拟量输入，不能仅靠添加一个走路样本实现慢走按键。

## 第三步：创建动画蓝图并计算 Speed

1. 新建 Animation Blueprint（动画蓝图），Skeleton（骨架）选择 `SKEL_Unagi`，命名 `ABP_Unagi`。
2. 新建浮点变量 `GroundSpeed`（地面速度）。
3. 先在普通 Event Graph（事件图表）的 Event Blueprint Update Animation（蓝图更新动画事件）实现以下逻辑：

```text
Try Get Pawn Owner（尝试获取 Pawn 所有者）
  → Is Valid（是否有效）
  → Get Velocity（获取速度）
  → Vector Length XY（二维向量长度）
  → 设置 GroundSpeed
```

只取 XY，避免将跳跃的垂直速度算成跑步速度。`Try Get Pawn Owner` 在动画编辑器预览或对象尚未绑定时可能为空，所以先检查有效性。

这里使用普通更新事件读取对象，先完成容易理解的基础方案。以后需要优化时，可将读取运动状态改为 Property Access（属性访问）和 Blueprint Thread Safe Update Animation（蓝图线程安全动画更新）；不要直接把普通对象调用挪到工作线程函数中。

4. 在 AnimGraph（动画图表）放入 `BS_Unagi_Locomotion` 播放节点，将 `GroundSpeed` 接到它的 Speed 输入，将姿势输出接到 Output Pose（输出姿势）。第一阶段可以直接这样连通；加入跳跃时再把地面混合空间放进状态机。
5. 编译并保存自己编辑的动画蓝图。此为用户编辑器操作步骤，本次未执行蓝图编译。

## 第四步：指定给角色网格组件

打开实际受控角色的蓝图，例如 `BP_Character`，选择继承的人物 Mesh（网格体组件），设置：

| 设置 | 值 |
| --- | --- |
| Skeletal Mesh Asset（骨骼网格体资产） | 当前 `SK_Unagi` |
| Animation Mode（动画模式） | Use Animation Blueprint（使用动画蓝图） |
| Anim Class（动画类） | `ABP_Unagi` |

要设置的是人物网格组件，不是胶囊体，也不是仅在动画编辑器里选择预览网格。角色组件实例若覆盖了旧动画类，也要核对。

基础运动可先使用完整原角色验证。独立武器及人物去武器副本是另一项资源整理；当前五份静态武器不包含原蒙皮，不能替代原版多骨武器动画。详见 `UnagiWeaponExtraction.md`。

## 第五步：再加入离地、跳跃与落地

有合适跳跃动作后，再增加布尔变量 `bIsInAir`：从角色的 Get Character Movement（获取角色移动组件）读取 Is Falling（是否正在下落）。获取移动组件时需要角色类型引用，可从所有者转换为 `Character`（角色）后读取，并检查引用有效性。

最简单的初版状态机包含：

```text
Grounded（地面：播放走跑混合空间）
  → bIsInAir 为真 → Airborne（空中：播放合适的空中循环）
  → bIsInAir 为假 → Grounded
```

`Is Falling` 包括起跳后的上升和下落，也包括走出悬崖，不是“跳跃按键是否按下”。只用一个空中循环是初版方案；有素材时再拆成起跳、空中、落地状态，配置非循环起跳/落地和剩余时间转换。

当前控制器源码只查到移动和观察输入，没有跳跃绑定。动画读取 `Is Falling` 不会让人物起跳；以后需在既有 PlayerController 中接收跳跃输入，通过角色的 `Jump`（跳跃）/`StopJumping`（停止跳跃）行为驱动移动组件，保持动画只读取运动结果。

## 用户在编辑器验证

以下步骤尚未由本次任务执行：

1. 站立不输入时，预期 `GroundSpeed` 接近 0，播放待机。
2. 向不同方向移动时，预期人物转向运动方向，速度上升并从待机混合到移动动作。
3. 松开输入减速时，预期速度下降，动作回到待机。若身体在移动但脚明显滑动，核对实际速度、采样位置和动画步频。
4. 配置跳跃后，预期离地进入空中状态，落地回到地面。没有跳跃输入或动作素材时不要把动画切换当成已经完成跳跃功能。
5. 若始终保持参考姿势，检查骨架是否匹配、Mesh 的动画模式/动画类，以及 AnimGraph 是否连接到最终输出；若始终待机，检查所有者是否有效、`GroundSpeed` 是否更新。

本次实际只核对角色源码、已导入片段清单和官方资料，未修改 C++/资产、未编译、未执行自动化或 PIE 测试。

## 官方参考

- [Epic：混合空间与一维混合](https://dev.epicgames.com/documentation/en-us/unreal-engine/blend-spaces-in-unreal-engine)
- [Epic：动画变量、水平速度与离地状态](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-get-animation-variables-in-animation-blueprints-in-unreal-engine)
- [Epic：动画状态机](https://dev.epicgames.com/documentation/en-us/unreal-engine/state-machines-in-unreal-engine)
