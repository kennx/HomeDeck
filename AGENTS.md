# HomeDeck Agent 指南

<!-- Last updated: 2026-06-18 -->

使用用户输入的语言回复用户（git commit 除外，统一使用英文 Conventional Commits）。

本项目是面向 [M5Stack PaperColor](./docs/PaperColor.md) 设备的 ESP32-S3 嵌入式固件，硬件使用 E Ink Spectra 6 六色墨水屏（黑、白、红、黄、蓝、绿）。架构为 Arduino 框架 + PlatformIO 构建，无操作系统。本文件仅覆盖项目特定约束；通用 C++/嵌入式规范由编译器与硬件决定。

## 工作原则

- 从第一性原理出发。基于真实硬件约束、代码事实和验证结果进行思考；目标不明确时先与用户讨论。
- 代码而非文档是真理之源。除非用户明确要求，否则不要为了理解实现而阅读普通 Markdown 文档。
- 修改代码前先阅读相关代码与最新约束，并遵循目录树中最近的 `AGENTS.md`。
- 保持修改聚焦。不要顺带进行无关重构。
- 嵌入式优先：始终考虑内存占用、功耗和硬件约束。
- 每完成一项任务（编辑修改代码）要进行 commit。

## 项目地图

- `src/app/main.cpp` — 入口点，仅委托给 `app_runtime`。
- `src/app/app_runtime.cpp/h` — 应用生命周期（`appSetup`、`appLoop`、`enterHomeDeepSleep`），编排各子系统。
- `src/app/boot_controller.cpp/h` — 启动模式决策（配置模式 vs 系统模式）、设置快捷键、睡眠调度。
- `src/app/view_manager.cpp/h` — 视图路由与切换。`SystemView` 枚举（`Almanac`/`Calendar`/`Countdown`/`Weather`）定义于此。
- `src/views/` — 视图组件。`almanac_view` 为主 UI；`calendar_view`、`countdown_view`、`weather_view` 为其他视图；`view_common` 为共享渲染工具（状态栏、时间格式化）。
- `src/config/` — 配置子系统：`config_types`（结构）、`config_store`（NVS 持久化）、`config_validator`、`config_portal`（AP 门户）、`config_portal_renderer`（墨水屏门户页面：二维码/AP 信息/Logo）、`setup_page`（设置页 HTML 入口）。
- `src/system/` — 系统服务：`time_service`（NTP/RTC）、`wifi_connection`、`sht40_reader`（温湿度）、`render_context`（封装 Canvas 操作：`prepareScreen`/`pushScreen`/`sprite`/`formatCurrentTimeHHMM`）。
- `src/providers/` — 数据提供者：`almanac_provider`（读 `data/almanac.bin`）、`timezone_catalog`、`weather_provider`、`webcal_provider`（解析 ICS WebCal 订阅，为倒数日提供节日/事件数据）。
- `src/generated/` — 自动生成资源（`device_font_vlw` 设备字体、`setup_page_html` 设置页 HTML）。**只读**，需更新时改 `tools/` 生成器并重跑。
- `data/` — 运行时数据文件（`almanac.bin` 年鉴数据、`logo.png` 设备 Logo）。构建时通过 PlatformIO 打包进 LittleFS。
- `fonts/` — 原始字体源（`misans/` 等），供 `tools/` 转换为设备字体。
- `test/native/` — 使用 Unity 框架的本机（宿主机）单元测试。每个组件有独立测试目录（如 `test_almanac_provider`、`test_boot_controller` 等），按需向对应目录追加用例。
- `test/native/support/` — 本机测试的 fake/stub：`fake_arduino`、`fake_uical`。
- `tools/` — 生成器脚本：`generate_almanac_data.py`（年鉴二进制）、`generate_device_font.py`/`font_to_vlw.cpp`（设备字体）、`generate_setup_page.py`（设置页 HTML）、`sync_preview.py`（从 `scratch/preview.html` 同步到生成头文件）、`test_generate_almanac_data.py`（生成器测试）。
- `docs/PaperColor.md` — 设备硬件文档（GPIO、分辨率、刷新时序）。
- `docs/eink_spectra6_colors.md` — 六色墨水屏颜色参考与 UI 设计约束。
- `docs/features/` — 功能开发记录与待办。
- `docs/issues/` — 已知问题与排查记录。
- `docs/superpowers/{plans,specs}/` — 功能实现计划与设计规格。
- `platformio.ini` — 构建配置（双环境：`m5stack-papercolor` 目标、`native` 测试）。

## 脚本

数据生成脚本需要 Python 3.10+（使用 `from __future__ import annotations`）。第三方依赖见 `tools/requirements-almanac.txt`。

```bash
# 安装依赖
pip install -r tools/requirements-almanac.txt

# 运行生成器测试
python3 tools/test_generate_almanac_data.py

# 重新生成资源（按需）
python3 tools/generate_almanac_data.py   # 重新生成 data/almanac.bin
python3 tools/generate_device_font.py    # 重新生成 src/generated/device_font_vlw.*
```

## 环境要求

- **PlatformIO**：构建、烧录和测试所必需。目标环境 `env:m5stack-papercolor`：ESP32-S3，16MB Flash（QIO），OPI PSRAM，Arduino 框架，LittleFS 文件系统。
- **C++ 标准**：`gnu++17`（通过 `build_flags` 强制，`build_unflags` 移除默认 `gnu++11`）。
- **本机测试**：`env:native`——使用 Unity 框架和 `fake_arduino`/`fake_uical` 存根的主机端编译，`test_build_src = yes`。
- **串口/烧录**：`monitor_speed = 115200`，esptool 默认上传波特率 `921600`。
- **核心库**（`lib_deps`，版本由 `platformio.ini` 管理）：M5Unified、M5GFX、M5PM1、QRCode、uICAL、Adafruit NeoPixel、ArduinoJson（native 测试环境仅含 ArduinoJson）。
- **构建钩子**：`tools/generate_setup_page.py` 作为 `pre` 脚本在两个环境下均会运行。

## 硬性约束

- **六色墨水屏，非全 RGB**：仅黑/白/红/黄/蓝/绿。UI 设计禁止渐变、半透明、复杂混色。
- **刷新代价高且伴随闪烁**：禁止动画和频繁重绘。追求"一次刷好、长期保持"，配合深度睡眠实现零功耗静态显示。不要在没有速率限制的情况下在循环中调用渲染函数；尊重刷新生命周期，不必要的重绘会导致闪烁和屏幕老化。
- **内存约束**：ESP32-S3 RAM 有限，`ARDUINO_LOOP_STACK_SIZE=32768`。小缓冲区优先栈分配；谨慎使用 `std::vector`/`std::string`。避免在主循环中动态分配；充分利用 PSRAM（`BOARD_HAS_PSRAM`）。
- **深度睡眠优先**：设备通过 `enterHomeDeepSleep` 进入深度睡眠。不要添加会阻止睡眠的忙等待或轮询循环。
- **持久化分层**：用户配置（WiFi、时区、NTP、坐标、WebCal URL 等）通过 `config_store` 存入 **NVS**（`Preferences` API）；运行时数据文件（`almanac.bin` 等）通过 **LittleFS** 访问。持久化行为变更时需更新对应测试（`test_config_store`、`test_config_validation`）。
- **依赖注入而非全局状态**：优先通过 `std::function` 和普通结构体注入依赖（参见 `BootControllerDeps`、`ViewManagerDeps` 模式），而非静态全局变量或深度继承。最小化全局状态；不可避免时说明原因并限制在翻译单元内使用 `static`。
- **`src/generated/` 只读**：对 Agent 而言只读。需要更新生成文件时，修改 `tools/` 生成器脚本并重新运行。
- **测试聚焦**：不要添加过多新测试文件。优先向 `test/native/` 下对应组件的现有测试目录追加用例。新增硬件相关代码时，在 `test/native/support/` 下提供配套的 fake/stub，确保本机测试可继续编译。
- **测试冲突处理**：当测试因用户修改而失败时，默认先修复测试；除非实现确实存在 bug，否则不要为了满足旧测试而更改实现。

## 通用编码规则

- 所有应用代码使用 `homedeck` 命名空间（扁平，不按子目录嵌套）。
- 文件名/目录名：`snake_case`。类名：`PascalCase`。函数/变量：`camelCase`。私有成员：尾部下划线（`name_`）。
- 头文件保护：优先 `#pragma once`。
- 头文件 include：使用 `"目录/文件名.h"` 相对路径（如 `"views/almanac_view.h"`），不带 `src/` 前缀（PlatformIO 已将 `src/` 加入 include path）。
- 新代码文件按功能放入对应子目录：`app/`（生命周期/启动/视图管理）、`views/`（渲染/视图组件）、`system/`（硬件服务）、`config/`（配置子系统）、`providers/`（数据提供者），不要放回 `src/` 根目录。

## 工具使用指南

- 优先使用 `rg` / `rg --files`（若可用）读取与搜索文件。
- 验证外部事实（库 API、版本号、硬件时序、墨水屏颜色规格）时，先用搜索/文档工具核实，不要凭记忆断言。
- 修改前阅读相关代码与最近 `AGENTS.md`；设计修改时优先遵循现有边界和本地模式。

## 验证清单

完成任务后，在声称完成前运行（无需等待用户许可）：

```bash
pio run -e m5stack-papercolor      # 目标固件构建
pio test -e native                  # 本机单元测试
```

数据或脚本变更时另加：`python3 tools/test_generate_almanac_data.py`

未通过命令验证其存在前，不要假定运行其他 lint/format 任务。

## 指令更新位置

- 影响几乎所有任务的硬性规则：更新根目录 `AGENTS.md`。
- 仅影响特定目录的规则：更新最近的子目录 `AGENTS.md`。
- 保持指令更新聚焦，并以代码事实为依据。

## 工作流

- **Git commits**：Conventional Commits（`feat:` / `fix:` / `refactor:` / `chore:` 等），标题行使用英文。body 按文件或功能分组，说明改了什么、为什么改、影响范围；修复 bug 需说明根因；架构决策需简要说明理由。
- **忽略文件**：`.DS_Store`、`.pio/`、`.pioenvs/`、`.piolibdeps/`、`.worktrees/`、`.claude/`、`.superpowers/`、`compile_commands.json` 必须保持被 `.gitignore` 忽略，不要提交。
- **每完成一项代码修改任务即 commit。**
