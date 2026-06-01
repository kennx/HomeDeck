# HomeDeck Agent 指南

使用用户输入的语言回复用户。

这是 HomeDeck 固件项目的根级 Agent 指南。保持简洁，只保留高频规则：项目地图、硬约束和工作流要求 —— 每个任务都需要知道的事情。

## 工作原则

- 从第一性原理出发。基于真实硬件约束、代码事实和验证结果进行思考；如果目标不明确，先与用户讨论。
- 将代码而非文档视为真理之源。除非用户明确要求，否则不要为了理解实现而阅读普通 Markdown 文档。
- 在修改代码前，先阅读相关代码和最新的约束，并遵循目录树中最近的 `AGENTS.md`。
- 保持修改聚焦。不要顺带进行无关重构。
- 嵌入式优先：始终考虑内存占用、功耗和硬件约束。

## 项目地图

```
src/
├── app/              # 应用生命周期与协调（app_runtime、boot_controller）
├── views/            # 所有视图渲染器（calendar_view、almanac_view、countdown_view 等）
├── system/           # 硬件交互与系统服务（wifi_connection、sht40_reader、time_service 等）
├── config/           # 配置与配网（config_store、config_portal、setup_page）
├── providers/        # 数据/内容提供者（almanac_provider、timezone_catalog）
└── generated/        # 工具生成文件（设备字体），对 Agent 只读

test/native/          # 单元测试（Unity），每个被测模块对应一个 test_<module>/
tools/                # 构建时 Python 脚本（字体/老黄历数据生成）
data/                 # 运行时资源（logo.png、almanac.bin），打包到 LittleFS
docs/                 # 产品文档和开发记录
fonts/                # 字体源文件，经 tools/ 生成 src/generated/
```

## 环境要求

- **PlatformIO Core**：用于构建、烧录和测试（`pio run`、`pio test`）。
- **Python 3**：运行 `tools/` 下的生成脚本。
- **目标环境**：`env:m5stack-papercolor`——ESP32-S3，16MB Flash，QIO OPI PSRAM，Arduino 框架。
- **核心库**：M5Unified、M5GFX、M5PM1。
- **C++ 标准**：gnu++17（通过 `build_flags` 强制）。
- **本机测试**：`env:native`——使用 UNITY 和 fake Arduino 存根的主机端编译。
- 上传波特率：115200。

## 通用编码规则

- 所有业务代码放在 `homedeck` namespace 下。
- 文件名：`snake_case`。类名：`PascalCase`。函数/变量：`camelCase`。私有成员：尾部下划线（`name_`）。
- 头文件保护：优先使用 `#pragma once`。
- 优先通过 `std::function` 和普通结构体进行依赖注入（`XxxDeps` 模式），而非静态全局变量或深度继承。
- 类成员变量使用 trailing underscore 命名（如 `deps_`、`currentView_`）。
- 枚举使用 `enum class`。
- 不要在头文件中引入不必要的 Arduino / M5 头文件，保持头文件轻量；实现文件再包含具体平台头。
- 最小化全局状态。如果不可避免，说明原因并限制在翻译单元内使用 `static`。
- 墨水屏注意：不要在没有速率限制的情况下在循环中调用渲染函数。尊重刷新生命周期；不必要的重绘会导致闪烁和屏幕老化。
- 睡眠/功耗：设备使用深度睡眠（`enterHomeDeepSleep`）。不要添加会阻止睡眠的忙等待或轮询循环。
- 内存：ESP32 的 RAM 有限。小缓冲区优先使用栈分配；谨慎使用 `std::vector`/`std::string`。避免在主循环中进行动态分配。
- 对 Agent 而言，`src/generated/` 是只读的。如果生成的文件需要更新，修改 `tools/` 中的生成器脚本并重新运行。
- 不要添加过多新的测试文件。优先把测试加到对应模块已有的测试文件中。
- 当测试因用户修改而失败时，默认先修复测试；除非实现确实有 bug，否则不要为了迁就旧测试而改动实现。
- 不要为了外部兼容性牺牲代码质量，除非用户明确要求。破坏性改动需要用户确认。

## 指令更新位置

- 影响几乎所有任务的硬性规则：更新根目录的 `AGENTS.md`。
- 只影响特定目录的规则：更新该目录下最近的 `AGENTS.md`。
- 保持指令更新聚焦，并以代码事实为依据。

## 工作流要求

- 如果 `rg` / `rg --files` 可以使用，读取代码时优先使用。
- 在设计修改时，优先遵循现有的边界和本地模式。
- 完成任务后，在声称完成前，使用 `pio run -e m5stack-papercolor`（构建）和 `pio test -e native`（单元测试）进行验证。
- 当添加新的硬件相关代码时，在 `test/native/support/` 下提供配套的主机端 fake 或 stub，以确保本机测试能够继续编译。
- **git commit 规范**：
  - 标题行使用 conventional commits 格式（`feat:` / `fix:` / `refactor:` / `chore:` 等）。
  - body 中按文件或功能分组，说明改了什么、为什么改、影响范围。
  - 修复 bug 需说明根因；架构决策需简要说明理由。

## 查询与验证顺序

遇到不确定时：

1. 能否用 **context7** 查 M5Unified / M5GFX / M5PM1 官方文档？
   - 是 → 查文档，获取准确信息。
2. 能否用 **GitHub MCP** 阅读相关开源库源码、Issues 或 Release Notes？
   - 是 → 直接阅读仓库源码或历史 issue，验证 API 行为、已知 bug、版本兼容性。
3. 能否用 **tavily** / **searxng** 搜索社区方案？
   - 是 → 搜索并交叉验证 2–3 个来源。
4. 否 / 搜索结果矛盾 → 停下来，向用户说明困惑点，请求澄清。

## 绝不编造的规则

- **不编造 API 参数**：如果不确定 `M5.Power.getBatteryVoltage()` 的返回值类型，用 context7 查 M5Unified 文档。
- **不编造引脚定义**：如果不确定硬件接口对应哪个 GPIO，用 context7 查 Cardputer-Adv PinMap。
- **不编造硬件限制**：如果不确定 ESP32-S3 的某项能力，搜索官方 specs。
- **不编造库版本**：如果不确定某个库是否支持特定功能，查库文档或 GitHub release notes。
