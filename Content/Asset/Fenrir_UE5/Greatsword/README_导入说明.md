# Fenrir 大剑

- `SM_Fenrir_Greatsword.fbx`：独立武器模型，导入 UE 时使用 Static Mesh（静态网格体），不勾选 Skeletal Mesh（骨骼网格体）。统一缩放使用 1。
- `Fenrir_Greatsword_UE5.blend`：以 Blender 4.5.9 LTS 保存，贴图已内嵌；建议 Blender 4.5 或更新版本打开。
- `Textures/`：基础颜色、金属度、粗糙度、OpenGL 法线、自发光五张 2048×2048 贴图。

模型原始长度约 189.91 厘米，3876 顶点、7820 三角面，单材质槽。保留原作者的原点，没有猜测并重置握持点；挂到手部插槽后，需要通过武器组件相对位置/旋转或插槽预览调整握持姿势。

UE 材质连接对应 Base Color（基础颜色）、Metallic（金属度）、Roughness（粗糙度）、Normal（法线）、Emissive Color（自发光颜色）。颜色与自发光贴图开启 sRGB，其余关闭；法线使用 Normalmap（法线贴图）压缩，并勾选 Flip Green Channel（翻转绿色通道），适配 UE 的切线空间约定。FBX 自动材质不保证正确转换金属度和粗糙度，需要核对连接。

已用 Blender 渲染检查外观，并把导出的 FBX 重新导入，确认仅一个网格、没有骨架、材质槽与三角面数一致、长度约1.89908米。本轮只提取源文件，没有导入UE资产、配置游戏武器或替换现有光剑。
