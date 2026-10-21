# Fenrir UE5 骨骼模型

交付文件：

- `SK_Fenrir.fbx`：角色骨骼网格体交换文件，不含动画。
- `Fenrir_UE5.blend`：清理后的可编辑模型，使用 Blender 4.5.9 LTS 保存；建议 Blender 4.5 或更新版本打开。贴图已内嵌。
- `Textures/`：24 张原模型贴图，供 UE 材质使用；保留源文件的白、黑、橙配色。
- `Fenrir_preview.png`：处理后的 Blender 静态预览，不是 UE 实机画面。

## 本次处理

采用原文件中已有的简化人形骨架和蒙皮。剔除另一套重复角色、控制器形状及武器，清除失效 IK（反向动力学）约束，补充地面 `root` 根骨骼并将 `pelvis` 置于其下。保留原始 58 根骨骼及裙甲附加骨骼，最终共 59 根骨骼。合并为一个网格，保留五个材质槽；规范化权重，三角化导出，关闭末端附加骨骼和动画导出。场景采用厘米，物体变换为单位变换。

统计：21,439 个顶点、37,452 个三角面、5 个材质槽；模型原始高度约 233.27 厘米，没有缩成默认小白人高度。未分配有效权重的顶点为 0，单顶点最多 7 个骨骼影响。

## UE5 导入

本目录已加入当前项目的自动导入排除规则 `Asset/Fenrir_UE5/*`，用于保持“只交付源文件”的要求。该规则不阻止手动导入；本轮没有生成 `.uasset`。

1. 将 `SK_Fenrir.fbx` 导入为 Skeletal Mesh（骨骼网格体）。Skeleton（骨架）先留空，让 UE 新建骨架。Import Uniform Scale（导入统一缩放）使用 1。
2. 关闭 Import Animations（导入动画）。按需要创建 Physics Asset（物理资产）；本交付不含 `.uasset` 或物理资产。
3. 使用 Import Normals and Tangents（导入法线与切线），或按项目规则重新计算切线。导出的网格已经三角化。
4. 五个材质槽分别为 Arm、Body、Head、Legs、Skeleton。对应贴图连接 Base Color（基础颜色）、Metallic（金属度）、Roughness（粗糙度）、Normal（法线）及 Emissive Color（自发光颜色）。金属度、粗糙度和法线贴图关闭 sRGB；法线贴图使用 Normalmap（法线贴图）压缩。
5. 文件名含 `NormalOpenGL` 的贴图采用 OpenGL 法线约定。UE 中勾选 Flip Green Channel（翻转绿色通道），将其适配 DirectX 切线空间。自发光强度可在材质里乘系数调节。

Blender 材质节点不能完整地自动转换成 UE 材质；FBX 自动导入材质后仍应检查贴图连接。

## 使用 Manny / Quinn 动画

本模型保留原作者的人形骨架；它不是 Manny / Quinn 的同一套骨架，不能仅因骨骼同名就直接指定 Manny 的 Skeleton（骨架）或复用其动画蓝图。应新建目标 IK Rig（IK绑定），再用 IK Retargeter（IK重定向器）从 Manny / Quinn 传递动画。

目标 Retarget Root（重定向根）使用 `pelvis`，Root Motion（根运动）骨骼使用 `root`。建议主要链如下；手指链各由对应 `01` 到 `03`：

| 链 | 起始骨骼 | 结束骨骼 |
| --- | --- | --- |
| Spine | spine_01 | spine_03 |
| Neck | neck_01 | neck_01 |
| Head | head | head |
| LeftClavicle | clavicle_l | clavicle_l |
| RightClavicle | clavicle_r | clavicle_r |
| LeftArm | upperarm_l | hand_l |
| RightArm | upperarm_r | hand_r |
| LeftLeg | thigh_l | ball_l |
| RightLeg | thigh_r | ball_r |

在源 Manny / Quinn 和目标 Fenrir 间匹配对应链，并对齐 Retarget Pose（重定向姿势）。模型保留原始 A 姿势，肩宽、四肢比例和身高不同，实际动画仍需调整姿势、足部接触和裙甲运动。裙甲附加骨骼默认随其父骨骼运动；没有在本轮新增裙甲动画或物理。

## 验证范围

已重新打开处理后的 Blender 文件并渲染检查贴图和外观；已将导出的 FBX 重新导入 Blender，确认单一骨架、单一根骨骼、全部 59 根骨骼保留、网格蒙皮、五个材质槽、三角面数量与 2.3327 米高度一致，没有附加末端骨骼。

遵照本轮要求，没有导入 UE 项目，没有创建重定向器，也没有在 UE 播放或验证动画。因此交付为可供 UE 导入和配置重定向的源文件，不声称已完成 Manny 动画适配。

官方参考：

- [Epic：FBX 骨骼网格体流程](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-skeletal-mesh-pipeline-in-unreal-engine)
- [Epic：IK Rig 动画重定向](https://dev.epicgames.com/documentation/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)
