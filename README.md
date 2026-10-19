# ThirdPersonARPG

基于 Unreal Engine 5.8 的第三人称 ARPG 项目。

- `Source/ThirdPersonARPG/`：C++ 源码。
- `Content/`：蓝图、关卡和其他游戏资源，通过 Git LFS 管理 UE 二进制资源。
- `Config/`：项目配置。
- `Docs/ExecutionLog.md`：每次项目任务的执行记录。
- `AGENTS.md`：持续记录、提交和推送规范。

## 获取项目

安装 Git、Git LFS 和 Unreal Engine 5.8。首次克隆时先运行 `git lfs install`，然后克隆仓库并在仓库目录执行 `git lfs pull`。右键 `ThirdPersonARPG.uproject` 生成 IDE 项目文件，再编译 `ThirdPersonARPGEditor` 的 Development / Win64 目标。

## 版本管理

每次任务完成后，在执行日志中记录需求、操作及验证结果，将相关文件与日志一起提交并推送。缓存、编译输出和 IDE 项目文件由本机生成，不提交。已有缓存不会因忽略规则而被删除。

常用检查命令：

```powershell
git status
git log --oneline -10
git lfs ls-files
```

修改 C++ 构造函数或反射声明后，建议关闭编辑器再完整编译。编译被 Live Coding 阻止时，应在执行日志中保留这一结果。
