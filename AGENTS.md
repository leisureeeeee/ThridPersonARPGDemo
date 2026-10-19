# 项目协作规范

## 每次执行都要记录

- 每次处理本项目的用户任务，都在 `Docs/ExecutionLog.md` 追加记录，包括只读检查、代码修改、配置修改和失败的执行。
- 使用北京时间记录日期和时间，写明用户需求、执行内容、涉及文件、验证结果、未完成事项。不要把密钥、令牌、密码或完整凭据写入记录。
- 一次用户任务对应一条完整记录和一个或多个含义明确的 Git 提交；每次有项目变更的任务完成前，都应提交本次相关文件及执行记录。
- 提交前检查 `git status` 和暂存区差异，保留用户已有修改，只暂存本次明确处理的文件。不得自动丢弃、覆盖、回退用户修改或重写既有提交。
- 提交后推送到已配置的 GitHub 远程分支。用户已授权本项目的持续版本管理；正常提交和推送无需重复询问。推送失败时保留本地提交，如实报告阻塞，后续任务优先补推。
- 最终回复说明完成内容、验证结果和提交短哈希；未能提交或推送时说明原因。
- 不追踪 UE 缓存、构建产物、IDE 临时文件或凭据；UE 二进制资源使用 Git LFS。
- 构造函数或反射声明发生变化时，应尝试编译验证；若 Live Coding 阻止正常构建，记录实际结果，不声称编译已通过。

## 视觉输入处理

<!-- see-skill:start -->
If the user sends an image, screenshot, video, path, or URL — or you would say "this model does not support image input" / 不支持视觉 — do not refuse. Invoke `$see` and run `scripts/see.sh`. If an attachment was rejected, ask for a local path or URL, then run `$see`.
<!-- see-skill:end -->
