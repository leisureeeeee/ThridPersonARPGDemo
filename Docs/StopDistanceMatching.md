# 停步距离与动画进度同步

实现日期：2026-10-21。替代原 Stop 固定 1.5 倍速；Start/Walk/Idle 和角色制动参数保持原样。

## 运行流程

1. NativeInitializeAnimation（动画实例初始化）清除上一轮运行状态，并检查停步采样配置是否有效：时间递增、距离非负递减、末端归零。
2. NativeUpdateAnimation（每帧动画更新，游戏线程）读取通用 ACharacter 的 CharacterMovement，不缓存角色/世界指针，不依赖具体按键。松键边沿开始停步；再次输入则清空停步阶段，原 Stop→Start 高优先级过渡继续生效。
3. 根据真实水平速度、GroundFriction、BrakingFriction、BrakingFrictionFactor、BrakingDecelerationWalking 估计剩余制动距离。预测采用连续模型 dv/dt=-(f*v+a) 的解析积分；不是 UE 内置预测器，也不是碰撞轨迹查询，与引擎离散子步制动会有误差。没有给项目新增插件或改变物理移动。
4. 从 StopDistanceSamples 查找对应动画时间，线性插值输出 StopAnimationTime。动画时间只向前，避免接触/速度扰动时动画倒播。
5. Stop 内由 Sequence Evaluator（序列评估器）的 Explicit Time（显式时间）读取 StopAnimationTime 并输出姿势，取代原 Sequence Player（序列播放器）。评估器非循环、跳转到指定时间，不提取根运动或触发该节点上的动画通知。当前停步动画根锁设置保留。
6. 实际速度 <=3 cm/s 后，将根骨骼已经停止的尾段在 StopSettleDuration=0.12 秒内收完；bStopAnimationComplete 触发 Stop→Idle，仍有原 0.12 秒混合。因此 0.12 秒不代表整个松键到完全待机的总时长。停步中再次输入仍立即优先切换起步。

## Katana 采样配置

动画：AS_Walk_Stop_F_0_Seq。原长 1.183333 秒；以约 60 Hz 采样得到 72 个点，保存到 BP_CharacterAnim 类默认值。点的 X 为秒、Y 为该时刻到根骨骼路径末端的剩余水平厘米数；从末尾向前累计二维路径长度，避免依赖特定前向轴。总距离 113.838814 cm，首个零距离样本约 0.883333 秒，之后为静止恢复尾段。

配置属于动画蓝图，C++ 没有硬编码 Katana 路径、动画长度或 114 cm。更换停步动画时，评估器和采样必须一起更新。Content/Python/bake_stop_profile.py 提供编辑器内 bake_stop_profile(anim_blueprint_path, stop_sequence_path) 重建配置；会拒绝 PIE、无 root、无水平位移等情况。VibeUE 某些版本无法读取评估器资产路径，此时调用者需在编辑器中核对动画一致。本轮通过直接读取评估器 node.sequence 已核对一致。

## 与 Lyra 的关系和取舍

参考 Epic 距离匹配与 Lyra 动画适配说明：通过剩余距离选择动画姿势，而不强迫胶囊跟着动画移动。本项目现在用 C++ 和类默认采样表实现小范围版本，没有复制 Lyra 的多层动画架构，也没有使用其官方距离曲线压缩/节点更新函数。

- https://dev.epicgames.com/documentation/en-us/unreal-engine/distance-matching-in-unreal-engine
- https://www.unrealengine.com/tech-blog/adapting-lyra-animation-to-your-ue5-game

当前强制动会让实际停止距离远小于动画原有 114 cm，因此一松键可能直接选择靠后的姿势。这是按距离选帧的结果，不能保证固定前向动画适合任意脚相位。若测试发现腿跳变，下一步需要左右脚停步选择、匹配过渡姿势或调整制动，不能单靠加大混合解决。

## 编译和检查

用户明确授权本轮编译并加载 C++，不启动关卡测试。Live Coding（实时编码）编译/加载成功，读取到新原生属性；动画蓝图编译后状态 UpToDate（最新），采样配置与评估器连接保存成功；PIE 始终未启动。

新增反射字段通过 Live Coding 加载。下次重启编辑器前应保存并关闭编辑器，完整构建 Development Editor（开发编辑器）目标，再打开项目，使基础 DLL 包含新字段。当前热补丁的编译成功不代表冷启动构建或打包已验证。

用户测试：长按 W 后松键，观察收脚拖尾；轻点 W，观察起步打断；停步时重新按 W，观察是否能立刻起步；结束运行关卡，确认是否再次出现此前闪退。运行效果和闪退复现均未由助手测试，本次也不声称修复崩溃。
