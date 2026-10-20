# 收刀与持剑移动：现有素材评估及免费资源

核对日期：2026-10-20。结论：当前已导入素材尚未确认能直接组成「收刀走路」和「持剑走路」两套完整循环；角色骨架本身可以接入这套功能。建议先用普通移动动作与免费持剑移动动作补齐，再处理武器姿势和切换。

## 当前素材实际检查结果

- 已导入的 `AN_Unagi_01`～`AN_Unagi_11`：通过 UE 的动画姿势接口只读采样，每段包含起止共 21 个时间点；按骨骼父子关系绘制正交投影姿势序列并查看。
- `AN_Unagi_01`：采样点的骨盆、腿、脚、手等主体关键骨骼位置不变，不能据此当作走路动作。
- `02`～`11`：采样姿势主要表现为突进、大幅手臂挥动和恢复等变化，目前没有确认到所需的两种完整行走循环。这是骨架姿势抽样观察，不是逐帧的完整角色动画预览，也没有据此给片段命名为某个具体攻击。
- 源 MAX 有 `Unagi_Idle.max`、`Unagi_Walk.max`、`Unagi_Run.max` 等名称，但尚未成功转换并预览。两级目录的同名 MAX 内容不同，不能直接认为一套收刀、一套持剑；`SwitchIn/Out` 名称也不能证明是拔刀/收刀。

因此，目前的结论是「可用动作尚未凑齐」，不是「整个原资源肯定没有这些动作」。MAX 里的 Walk 仍值得后续优先导出检查。

本次姿势采样、临时脚本、接触表图片和日志保存在 `Saved/LocomotionReview/`，未修改或保存原动画资产。

## 推荐的免费组合

| 资源 | 建议用途 | 已核对内容与限制 |
| --- | --- | --- |
| [MoCap Online：Free 3D Sword Animation Pack](https://mocaponline.itch.io/free-3d-sword-animation-pack) | 持剑待机、向前走、向前跑 | 作者免费包已下载并检查目录及说明，含对应 FBX 和 UE4 资产；不是原角色专属武士刀动作，握姿需适配，未见拔刀/收刀过渡片段 |
| [Epic：Game Animation Sample Project（游戏动画示例项目）](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-animation-sample-project-in-unreal-engine) | 收刀状态下的普通待机、走跑 | 官方免费移动动画来源；可以迁移动画序列，不必整套照搬 Motion Matching（运动匹配）系统。剑在鞘中的显示和固定关系需要自己配置 |
| [Quaternius：Universal Animation Library（通用动画库）](https://quaternius.itch.io/universal-animation-library) | 普通基础动作的另一个免费来源 | Standard（标准版）可免费获取，CC0；Pro（专业版）和 Source（源文件版）收费，不要把页面的整个 120+ 动作库当成全部免费。本次未逐条核实免费标准版内的持剑步行，因此不把它作为已验证的两套持剑状态解决方案 |

Epic 示例从 [Fab 官方项目页](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016) 获取；按下载时的项目版本与 UE 版本选择。Quaternius 当前说明中 UE 导出为 GLB，需要 UE5 以上；本项目 UE 5.8 满足版本方向要求，实际导入效果尚未验证。

MoCap Online 采用作者自己的许可，并非 CC0；使用时保留并遵守作者当前说明和[标准许可](https://mocaponline.com/pages/standard-license)。Epic 资源按其提供的授权使用。

## 已核对的持剑免费包文件

已从作者 itch.io 下载 `MCO_TC_Sword_Free_Pack_01.zip`，58,107,313 字节，放在：

`Saved/AnimationResourceResearch/MCO_TC_Sword_Free_Pack_01.zip`

实际 FBX 位于 `TC_Sword_Free_Pack/FBX_Pack/Animation/`：

| 文件 | 用途 |
| --- | --- |
| `KBS_Ready_Idle_001.fbx` | 持剑待机 |
| `KBS_Walk_F_001_IP.fbx` | 持剑向前行走，原地版本 |
| `KBS_Run_F_001_IP.fbx` | 持剑向前跑步，原地版本 |
| `KBS_Sword_ATK_Combo_01_001_IP.fbx` | 剑攻击组合，原地版本 |
| 对应不含 `_IP` 的 Walk / Run / Combo 文件 | 带位移动作版本，需另行选择根运动处理方案 |
| `MotusMan_v50 FBX T Pose.fbx` | 源角色参考姿势 |

作者说明中的源道具挂接点为 `RH_MidFin_Prop`，握持偏移是动作适配的一部分；该骨骼并不是当前雅角色的骨骼。此处文件分类依据作者命名、包内说明和官方用途，本次没有导入外部动画、逐帧预览或完成重定向。

下载包只留在忽略的 Saved 缓存中，没有将第三方完整资源包加入仓库或重新分发。

## 对本项目的接入方案

以下是设计建议，尚未创建对应资产或运行逻辑。

1. 把两种来源的人体动作分别通过 IK Rig（IK 骨架）和 IK Retargeter（IK 重定向器）转换到 `SKEL_Unagi`。需要匹配骨盆、脊柱、四肢等骨骼链并调整参考姿势，不能直接把不兼容动画的 Skeleton（骨架）下拉框改成 `SKEL_Unagi`。参见 [Epic IK 重定向说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)。
2. 创建 `BS_Unagi_Sheathed` 与 `BS_Unagi_Drawn` 两个 Blend Space 1D（一维混合空间），分别放入收刀和持剑的待机/行走/跑步。二者都读取角色实际水平速度。当前角色朝运动方向转身，先使用向前移动片段即可；锁定目标横移以后再补方向动作。
3. 由角色或装备组件维护 `bSwordDrawn`，Animation Blueprint（动画蓝图）读取状态并选择混合空间。输入仍由现有 PlayerController（玩家控制器）接收，动画不直接依赖具体按键。
4. 收刀时保持剑在鞘中，持剑时保持剑柄与手对齐。若仅做两种稳定状态，可以先直接切换；想要手伸向剑鞘并完整拔出/收回，还需要 Draw / Sheath（拔刀/收刀）过渡动作。上述免费剑术包没有补齐这一部分。
5. 后续加入过渡时，用 Anim Notify（动画通知）在手真正握住剑、或剑进入鞘的时刻切换武器挂接/显示，而不是在输入触发的瞬间改变武器位置。双手握剑可能还需要手部 IK（逆向运动学）修正。

当前雅角色的武器不是统一的单骨刚体：原 FBX 包含多骨蒙皮、剑鞘相关骨骼及混合权重。外部人体动作也不会自动生成这些自定义武器骨骼的正确轨迹。五份已拆出的 Static Mesh（静态网格体）不保留原蒙皮，不能直接承诺一挂右手就保持原动画效果；需要选择合适的刚性部件方案，或保留原蒙皮并单独处理武器/剑鞘姿势。具体事实见 [武器拆分与兼容限制](UnagiWeaponExtraction.md) 和 `UnagiWeaponBindings.json`。

「剑在鞘里」既包含人物身体姿势，也包含武器装配状态。普通走路可以作为收刀移动的起点；如果希望左手始终扶鞘、衣服或剑鞘随动符合角色风格，还需要上半身姿势修正或专门的收刀步行动作。

## 后续在编辑器中检查

本次只做素材评估与资源查找，没有运行以下功能检查：

1. 重定向后双击 Animation Sequence（动画序列），查看脚底、膝盖、手腕朝向和身高比例；预期四肢方向正确，没有明显扭曲。
2. 单独循环每个 Walk（行走）片段，查看首尾衔接；采用原地版时，预期角色不会在预览中持续漂走。
3. 接入两套混合空间后，在相同移动速度下切换收刀/持剑状态；预期步态及握姿改变，实际移动仍由角色移动组件负责。
4. 查看剑柄是否对齐手掌、剑鞘是否固定、是否重复显示内嵌武器；有拔收刀过渡后再检查通知时机。
5. 若脚底滑动，按实际速度与动作步频调整混合空间采样值；不同来源的动作还需要统一身体姿态与风格。

本次 UE 只读姿势采样进程成功退出，日志为 0 errors / 0 warnings；这不代表完成了人物持剑效果验证。未修改 C++、蓝图或动画资产，未编译，未运行自动化、PIE 或功能测试。
