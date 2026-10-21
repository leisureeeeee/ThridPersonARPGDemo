# Katana 动画统一绑定 UE4 人偶骨架

目录：/Game/Katana_Animations（用户口述 katana_ANIM，项目中实际存在的目录）。目标：/Game/ARPG_Samurai/Demo/Mannequin/Character/Mesh/UE4_Mannequin_Skeleton，即当前角色与已完成步行动画使用的骨架。

共 246 个 AnimSequence：原本正确 4 个，替换 242 个。13 个重定向器不属于动画序列，没有修改。没有修改骨架资产或批量设置根运动。逐项结果见 KatanaSkeletonRebind.csv。

先将全部 246 个 uasset 备份到工作区 .ue-katana-rebind/backup-before-rebind，并生成 SHA256 清单。备份不纳入 Git。

## 原生工具与检查

新增 ARPGAnimationAssetTools，通过 UE 原生 SetSkeleton 绑定，而不直接写只读 Skeleton 属性或修改 uasset 二进制。Content/Python/rebind_animation_skeleton.py 驱动分批检查、绑定、保存。C++ 操作仅在编辑器构建有效，其他构建返回错误。

源骨架已缺失，因此无法从源骨架完整验证层级与参考姿势。本次验证的是动画数据模型的轨道名称能对应现用 UE4 骨架；结合用户此前已确认使用 UE4 人偶的资源约定完成绑定。这是重新指定兼容骨架，不是 IK 重定向或姿势转换。

所有未映射名称只有以下四个武器附加轨道：katana_sheath_01、katana_sheath_02、katana_weapon_01、katana_weapon_02。仅显式允许这四个名称，其余任何缺失名称会拒绝修改。保留源轨道列表和关键帧数量；这四个轨道在普通 UE4 人偶上不会产生武器/刀鞘运动。后续需通过合适的武器骨架或插槽方案处理，不能声称武器动画已接好。

每个资产写入后检查原始轨道列表、关键帧数量、时长、EnableRootMotion 和 ForceRootLock。发生不一致时工具返回错误并拒绝保存。没有主动编辑动画关键帧、播放倍率或根锁选项。

## 验证结果与限制

完整 Development Editor 构建成功后重开项目，再从磁盘加载全部 246 段动画：全部指向目标骨架，零加载错误。注册表亦为 246/246 一致。未启动关卡、未运行播放效果测试或逐帧视觉检查。

三段方向起步现在可读取，但根骨骼旋转在首/中/尾采样中均为单位旋转；左90/右90/后180的根平移分别指向 +X/-X/-Y。不能直接把资产名中的角度当作 root yaw 曲线，也不能声称本次已完成转身动画接线。之前的方向起步实现仍需进一步分析身体姿势，工作区里的通用配置草稿尚未接入项目。
