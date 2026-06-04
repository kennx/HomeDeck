# HomeDeck Agent 指南

使用用户输入的语言回复用户

本项目是面向 [M5Stack PaperColor](./docs/PaperColor.md) 设备的 ESP32 嵌入式项目。根目录 `AGENTS.md` 应限于热路径规则：项目地图、硬性约束和工作流要求——每项任务都需要知道的内容。

## 工作原则

- 从第一性原理出发。基于真实硬件约束、代码事实和验证结果进行思考；如果目标不明确，先与用户讨论。
- 将代码而非文档视为真理之源。除非用户明确要求，否则不要为了理解实现而阅读普通 Markdown 文档。
- 在修改代码前，先阅读相关代码和最新的约束，并遵循目录树中最近的 `AGENTS.md`。
- 每完成一项任务（编辑修改代码）要进行 commit。
- 保持修改聚焦。不要顺带进行无关重构。
- 嵌入式优先：始终考虑内存占用、功耗和硬件约束。
- 彩色墨水屏约束：本设备使用 E Ink Spectra 6（六色：黑、白、红、黄、蓝、绿），非全 RGB 色域。UI 设计避免渐变、半透明、复杂混色；刷新代价高且伴随闪烁，禁止动画和频繁重绘；追求"一次刷好、长期保持"以配合深度睡眠实现零功耗静态显示。

## 项目地图

- `src/app/main.cpp`：入口点。委托给 `app_runtime.cpp`。
- `src/app/app_runtime.cpp/h`：应用生命周期（`appSetup`、`appLoop`）。负责编排各子系统。
- `src/app/boot_controller.cpp/h`：启动模式决策逻辑（配置模式 vs 系统模式、设置快捷键、睡眠调度）。
- `src/app/view_manager.cpp/h`：视图路由与切换管理。
- `src/views/`：视图组件。`almanac_view` 为主 UI；`calendar_view`、`countdown_view`、`weather_view` 为其他视图；`view_common` 为共享渲染工具（状态栏、时间格式化等）。
- `src/config/`：配置子系统（`config_types`、`config_store`、`config_validator`、`config_portal`、`setup_page`、`config_portal_renderer`）。持久化状态保存在 NVS/LittleFS 中；`config_portal_renderer` 负责配置门户的墨水屏页面渲染（二维码、AP 信息、Logo）。
- `src/system/`：系统服务（`time_service`、`wifi_connection`、`sht40_reader`、`render_context`）。
- `src/providers/`：数据提供者（`almanac_provider`、`timezone_catalog`、`weather_provider`）。
- `src/generated/`：自动生成资源（设备字体、设置页 HTML）。请勿手动编辑。
- `test/native/`：使用 Unity 框架的本机（宿主机）单元测试。
- `tools/`：用于生成年鉴数据和设备字体的 Python 脚本。
- `docs/PaperColor.md`：这台设备的文档查询、设备信息、GPIO…… 

## 环境要求

- **PlatformIO**：构建、烧录和测试所必需。
- **目标环境**：`env:m5stack-papercolor`——ESP32-S3，16MB Flash，QIO OPI PSRAM，Arduino 框架。
- **C++ 标准**：gnu++17（通过 `build_flags` 强制）。
- **本机测试**：`env:native`——使用 UNITY 和 fake Arduino 存根的主机端编译。
- 串口波特率：`115200`（`monitor_speed`）。esptool 默认上传波特率为 `921600`。

## 通用编码规则

- 所有应用代码使用 `homedeck` 命名空间（扁平，不按子目录嵌套）。
- 文件名：`snake_case`。目录名：`snake_case`。类名：`PascalCase`。函数/变量：`camelCase`。私有成员：尾部下划线（`name_`）。
- 头文件保护：优先使用 `#pragma once`。
- 头文件 include：使用 `"目录/文件名.h"` 相对路径（如 `"views/almanac_view.h"`），不要带 `src/` 前缀。PlatformIO 已将 `src/` 加入 include path。
- 新代码文件按功能放入对应子目录：`app/`（生命周期/启动/视图管理）、`views/`（渲染/视图组件）、`system/`（硬件服务）、`config/`（配置子系统）、`providers/`（数据提供者），不要放回 `src/` 根目录。
- 优先通过 `std::function` 和普通结构体进行依赖注入（参见 `BootControllerDeps` 模式），而非静态全局变量或深度继承。
- 最小化全局状态。如果不可避免，说明原因并限制在翻译单元内使用 `static`。
- 墨水屏注意：不要在没有速率限制的情况下在循环中调用渲染函数。尊重刷新生命周期；不必要的重绘会导致闪烁和屏幕老化。
- 睡眠/功耗：设备使用深度睡眠（`enterHomeDeepSleep`）。不要添加会阻止睡眠的忙等待或轮询循环。
- 内存：ESP32 的 RAM 有限。小缓冲区优先使用栈分配；谨慎使用 `std::vector`/`std::string`。避免在主循环中进行动态分配。
- 对 Agent 而言，`src/generated/` 是只读的。如果生成的文件需要更新，修改 `tools/` 中的生成器脚本并重新运行。
- 不要添加过多新测试文件。优先将测试添加到 `test/native/` 下对应组件或模块的现有测试文件中。
- 当测试因用户修改而失败时，默认先修复测试；除非实现确实存在 bug，否则不要为了满足旧测试而更改实现。

## 指令更新位置

- 影响几乎所有任务的硬性规则：更新根目录 `AGENTS.md`。
- 仅影响特定目录的规则：更新最近的子目录 `AGENTS.md`。
- 保持指令更新聚焦，并以代码事实为依据。

## 工作流要求

- git commit 规范：
  - 标题行使用 conventional commits 格式（feat: / fix: / refactor: / chore: 等）。
  - body 中按文件或功能分组，说明改了什么、为什么改、影响范围。
  - 修复 bug 需说明根因；架构决策需简要说明理由。
- 如果 **rg** 可用优先使用 `rg` / `rg --files` 读取文件。
- 在设计修改时，优先遵循现有的边界和本地模式。
- 完成任务后，在声称完成前，使用 `pio run -e m5stack-papercolor`（构建）和 `pio test -e native`（单元测试）进行验证。
- 当添加新的硬件相关代码时，在 `test/native/support/` 下提供配套的主机端 fake 或 stub，以确保本机测试能够继续编译。