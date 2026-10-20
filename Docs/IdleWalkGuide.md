# 用 C++ 动画实例连接待机与行走

本次为接线说明与待添加代码示例，尚未修改 Source、蓝图或动画资产，没有编译或运行测试。用户已明确以 UE4 人偶骨架为基础，本步骤按该前提说明，不继续把骨架恢复作为当前讨论阻塞。

## 最小职责划分

现有 PlayerController 继续处理输入，CharacterMovement 继续控制实际位移；新增 `UMyCharacterAnimInstance` 读取角色实际水平速度，Animation Blueprint（动画蓝图）只负责混合姿势。第一版不加入起停动画、运动匹配、急转、IK 或独立运动组件。

`APawn::GetVelocity()` 的单位是 cm/s。使用实际速度而不是是否按住 WASD，角色减速时仍可播放行走，撞墙停下时不会仅因按键仍被按住而继续播放走路。

## 待添加示例

建议位置：`Source/ThirdPersonARPG/Animation/MyCharacterAnimInstance.h`。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyCharacterAnimInstance.generated.h"

UCLASS()
class THIRDPERSONARPG_API UMyCharacterAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

protected:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient,
        Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
    float GroundSpeed = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient,
        Category = "Locomotion", meta = (AllowPrivateAccess = "true"))
    bool bShouldMove = false;
};
```

建议位置：`Source/ThirdPersonARPG/Animation/MyCharacterAnimInstance.cpp`。

```cpp
#include "Animation/MyCharacterAnimInstance.h"
#include "GameFramework/Pawn.h"

void UMyCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    const APawn* Pawn = TryGetPawnOwner();
    GroundSpeed = IsValid(Pawn) ? Pawn->GetVelocity().Size2D() : 0.0f;
    bShouldMove = GroundSpeed > 3.0f;
}
```

`UAnimInstance` 是动画实例基类；`NativeUpdateAnimation()` 在动画更新时采集数据。`TryGetPawnOwner()` 获取拥有当前骨骼网格体的 Pawn，在动画编辑器预览中可以为空，故置零处理。`Size2D()` 忽略竖直速度。本阶段仅描述地面待机/行走，跳跃分支后续再添加。3 cm/s 是消除微小速度噪声的起始阈值，不是测量得出的最终值。

变量设为 private 并允许蓝图只读，蓝图不能任意写回速度。这里在游戏线程的更新阶段访问 Pawn；第一版不在工作线程直接访问角色或移动组件。

## 编译与编辑器接线

1. 添加上述文件后，由用户编译并让编辑器加载新类。新增反射类需要 UHT 生成代码，不把保存 .cpp 当作已经加载新类型。若编辑器中没有出现新类，保存工作后关闭编辑器，以 Development Editor（开发编辑器）目标构建再重新打开。不要强制结束有未保存内容的编辑器。
2. 创建 `ABP_ARPG_UE4`：选择 UE4 人偶 Skeleton（骨架），将 Parent Class（父类）设为 `MyCharacterAnimInstance`。也可创建后在 Class Settings（类设置）修改父类。
3. 在 AnimGraph（动画图表）中添加 State Machine（状态机），命名 `Locomotion`，连接到 Output Pose（输出姿势）。
4. 状态机中建立 Idle（待机）和 Walk（行走）两个状态，入口连 Idle。Idle 内播放 `AS_Idle_Seq`，Walk 内播放 `AS_Walk_Loop_F_0_Seq`，均启用 Loop Animation（循环动画）。普通/战斗待机应与对应行走组配套，本例先使用普通组。
5. Idle → Walk 过渡使用 `bShouldMove`；Walk → Idle 使用 `!bShouldMove`。从 My Blueprint（我的蓝图）显示继承变量，拖入变量读取即可；显示名可能是 Should Move。过渡混合时间可先尝试 0.15 秒，这是调试起点。
6. 在当前使用的 `BP_Character` 中选择 Mesh（骨骼网格体组件），确认网格使用匹配的 UE4 人偶骨架；Animation Mode（动画模式）设为 Use Animation Blueprint（使用动画蓝图），Anim Class（动画类）设为 `ABP_ARPG_UE4`。

Event Graph（事件图表）无需重复实现逐帧速度计算。只使用前向行走循环，因为角色已经朝移动方向转身。第一版快速反向时可能出现视觉滑步，后续再做急转与动画速度匹配。

## 用户验证步骤

- 在 PIE（编辑器中运行）后，给动画蓝图选择实际角色的调试实例，观察 GroundSpeed：站立接近 0，移动时增大，松键后减小。
- 站立播放待机，移动播放前向行走，停下回到待机；只转镜头不应触发行走。
- 当前最大移动速度为 600 cm/s，尚未测量该步行动画的原始速度，因此本步骤只验证接线和状态切换，不保证脚步与位移完全吻合。
- 当前没有真正创建这些类或绑定蓝图，以上为待执行步骤；本次仅完成说明文档和引擎声明的静态核对。
