# 持刀行走停步右手回正修复

用户反馈：持刀行走后停下，右手有明显抖动回正。

检查发现当前 BP_CharacterAnim1 的 Idle（待机）已按装备状态选择 AS_Idle_Combat_Seq，但 Walk（行走）和 Stop（停步）仍固定使用普通 Walk_Loop/Walk_Stop。普通停步结束时右手姿势与战斗待机不一致，是此次优先修复的资源衔接问题；没有运行游戏复现，不能据此排除所有视觉原因。

修复是在 C++ 动画实例新增收刀/持刀行走、收刀/持刀停步四个配置引用及 ActiveWalkAnimation、ActiveStopAnimation 两个运行输出。每帧按角色 IsWeaponDrawn 选择序列，动画图原 Walk/Stop 节点改为读取动态 Sequence（序列）引脚，原 WalkPlayRate、StopAnimationTime、距离匹配和过渡结构保持原样。

持刀动画采用 Katana 03_Walk/02_Walk_Combat/01_Walk_Combat/01_Walk_Combat_F_0 下 AS_Walk_Combat_Loop_F_0_Seq、AS_Walk_Combat_Stop_F_0_Seq，用现有 IKRIg_Man_to_ironman 从 Samurai 人偶重定向至 Fenrir；新文件在 /Game/BP/MyCharatcer/Animations/CombatWalk 下加 RT_ 前缀，原资产不覆盖。根运动关闭、强制根锁定开启、根锁定参考姿势，与原行走/停步一致，防止角色网格额外位移。

普通/战斗停步均为1.183333秒，目标动画根轨迹按60Hz采样最大差异约0.05269厘米，因此沿用现有停步距离曲线。该核对属于资产数据检查，不代表实机脚步或手部效果验证通过。

本次不更改拔收刀蒙太奇、奔跑速度、起步方向配置、插槽或武器偏移。起步系列仍沿用现有配置，尚未统一为战斗起步；若后续反馈起步阶段手臂不连续，再按相同方向逐步完善。

用户验证：左键拔刀，W行走一段后松开，观察停步到持刀待机时右手是否还会额外回正；对比收刀后的普通行走停步。额外观察持刀奔跑松移动键后的停步。未主动运行PIE或输入测试。
