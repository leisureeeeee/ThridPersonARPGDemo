# Unreal MCP 与 VibeUE 连接

本项目通过 UE 5.8 内置的 ModelContextProtocol（模型上下文协议）服务对外提供编辑器工具，VibeUE 在同一连接上补充工具。已启用 ModelContextProtocol、AllToolsets 和 VibeUE；工具服务随编辑器自动启动，地址为 `http://127.0.0.1:8000/mcp`。

## 本机客户端

在 PowerShell 中执行 `codex mcp add unreal --url http://127.0.0.1:8000/mcp`。配置后，在客户端 MCP servers（MCP 服务器）页面重新连接；若当前会话未出现新工具，重启客户端或新开会话。该连接属于本机配置，不将个人 Codex 配置提交到仓库。

保持 ThirdPersonARPG 的 UE 编辑器打开，然后询问：“通过 Unreal MCP 列出当前关卡路径和 Actor 数量，先不要修改资产。”能返回项目的真实信息才表示连接成功。更换项目时，该端口连接的是实际打开并运行 MCP 的编辑器。

## 编辑器设置

在 编辑 → 插件 中启用 Unreal MCP、All Toolsets（全部工具集）和 VibeUE。项目默认设置已打开 Auto Start Server（自动启动服务器）及 Tool Search（工具搜索），无需另外运行 VibeUE 服务器，也不需要真实地形服务的 API Key。

如果端口没有启动，在控制台执行 `ModelContextProtocol.StartServer`。若 8000 已被占用，修改编辑器中的端口和客户端 URL，使两者一致。编辑器启动失败时，先检查输出日志，不自动编译或改动项目资产。

## 编辑原则

先发现实际工具与 Python 类签名，再调用。优先通过 MCP 和 Unreal Python 读取、编辑资产；不直接修改 uasset 二进制。需要时按域读取 VibeUE 技能。现有 AGENTS.md 中的编译、测试、资产保护及版本管理规则继续适用；连接检查不包含 PIE（编辑器内运行）或功能测试。

该端点只用于同机连接，不向公网暴露。普通 ChatGPT 网页不会自动读取本机 Codex 的配置。
