# MiSans 多字体源实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 修改 `tools/generate_device_font.py`，让时间/指标/正文三种 VLW 字体分别使用 MiSans-Heavy/Bold/Medium 生成，设备端代码零改动。

**Architecture:** 扩展 `FontResource` 类使其携带独立的 `ttf_path`，将全局 `SOURCE_FONT` 替换为每个角色的独立配置，其余生成流程（编译编码器、VLW 生成、C++ 输出）保持不变。

**Tech Stack:** Python 3, FreeType (font_to_vlw.cpp)

---

### Task 1: 修改 FontResource 类支持独立 TTF 路径

**Files:**
- Modify: `tools/generate_device_font.py:34-48`

- [ ] **Step 1: 为 FontResource 添加 `ttf_path` 字段**

  在 `FontResource.__init__` 中新增 `ttf_path: Path` 参数，并保存为实例属性。

  ```python
  class FontResource:
      def __init__(
          self,
          name: str,
          symbol_prefix: str,
          pixel_size: int,
          codepoints: list[int],
          ttf_path: Path,
      ) -> None:
          self.name = name
          self.symbol_prefix = symbol_prefix
          self.pixel_size = pixel_size
          self.codepoints = codepoints
          self.ttf_path = ttf_path
          self.codepoints_path = BUILD_DIR / f"{name}_codepoints.txt"
          self.vlw_path = BUILD_DIR / f"{name}.vlw"
          self.vlw_data = b""
  ```

- [ ] **Step 2: Commit**

  ```bash
  git add tools/generate_device_font.py
  git commit -m "feat(fonts): FontResource 增加独立 ttf_path 字段

  - 为后续多字体源生成做准备
  - 设备端无改动"
  ```

### Task 2: 修改 run_encoder 使用 resource 自身的 ttf_path

**Files:**
- Modify: `tools/generate_device_font.py:141-150`

- [ ] **Step 1: 将 SOURCE_FONT 替换为 resource.ttf_path**

  ```python
  def run_encoder(resource: FontResource) -> None:
      run(
          [
              str(ENCODER_PATH),
              str(resource.ttf_path),
              str(resource.codepoints_path),
              str(resource.pixel_size),
              str(resource.vlw_path),
          ]
      )
  ```

- [ ] **Step 2: Commit**

  ```bash
  git add tools/generate_device_font.py
  git commit -m "feat(fonts): run_encoder 使用 resource 自身的 ttf_path

  - 不再依赖全局 SOURCE_FONT
  - 每个字体资源独立指定源 TTF"
  ```

### Task 3: 在 main() 中配置三种字体的独立源 TTF 并生成

**Files:**
- Modify: `tools/generate_device_font.py:10-14, 221-274`

- [ ] **Step 1: 定义字体源配置**

  删除全局 `SOURCE_FONT`，新增三个 TTF 路径常量：

  ```python
  BODY_FONT = ROOT / "fonts" / "misans" / "MiSans-Medium.ttf"
  METRIC_FONT = ROOT / "fonts" / "misans" / "MiSans-Bold.ttf"
  TIME_FONT = ROOT / "fonts" / "misans" / "MiSans-Heavy.ttf"
  ```

  同时把 `SOURCE_FONT` 的缺失检查替换为三个字体的检查：

  ```python
  for font_path in (BODY_FONT, METRIC_FONT, TIME_FONT):
      if not font_path.exists():
          raise SystemExit(f"source font missing: {font_path}")
  ```

- [ ] **Step 2: 创建 resources 时传入各自的 ttf_path**

  修改 `main()` 中 `resources` 列表的构造：

  ```python
  resources = [
      FontResource("device_font", "kDevice", BODY_PIXEL_SIZE, body_codepoints, BODY_FONT),
      FontResource(
          "device_metric_font",
          "kDeviceMetric",
          METRIC_PIXEL_SIZE,
          sorted(numeric_codepoints),
          METRIC_FONT,
      ),
      FontResource(
          "device_time_font",
          "kDeviceTime",
          TIME_PIXEL_SIZE,
          sorted(numeric_codepoints),
          TIME_FONT,
      ),
  ]
  ```

- [ ] **Step 3: Commit**

  ```bash
  git add tools/generate_device_font.py
  git commit -m "feat(fonts): 配置三种字体分别使用 MiSans Medium/Bold/Heavy

  - Body: MiSans-Medium.ttf @ 14px
  - Metric: MiSans-Bold.ttf @ 28px
  - Time: MiSans-Heavy.ttf @ 42px"
  ```

### Task 4: 运行生成脚本并验证

**Files:**
- Modify: `src/generated/device_font_vlw.h`
- Modify: `src/generated/device_font_vlw.cpp`

- [ ] **Step 1: 运行生成脚本**

  ```bash
  python tools/generate_device_font.py
  ```

  预期输出（示例，实际数值可能不同）：
  ```
  $ c++ -std=c++17 -O2 ... tools/font_to_vlw.cpp -o .pio/build/font-tools/font_to_vlw ...
  $ .pio/build/font-tools/font_to_vlw fonts/misans/MiSans-Medium.ttf ...
  $ .pio/build/font-tools/font_to_vlw fonts/misans/MiSans-Bold.ttf ...
  $ .pio/build/font-tools/font_to_vlw fonts/misans/MiSans-Heavy.ttf ...
  Generated ...
  device_font: 7500 glyphs, 1600000 VLW bytes
  device_metric_font: 19 glyphs, 5000 VLW bytes
  device_time_font: 19 glyphs, 11000 VLW bytes
  ```

  关键验证点：
  - 脚本成功完成，无报错
  - 三个 `$ font_to_vlw` 命令分别使用了 Medium/Bold/Heavy TTF
  - `device_font` 的字形数 >= 6000（MIN_GLYPH_COUNT）

- [ ] **Step 2: 验证生成的 C++ 文件结构正确**

  检查 `src/generated/device_font_vlw.h`：
  - 仍包含 `kDeviceFontVlw`、`kDeviceMetricFontVlw`、`kDeviceTimeFontVlw` 三个声明
  - 变量名与设备端代码一致

  检查 `src/generated/device_font_vlw.cpp`：
  - 三个 `PROGMEM` 数组内容已更新（时间戳变化或文件大小变化）

  ```bash
  ls -la src/generated/device_font_vlw.*
  git diff --stat src/generated/device_font_vlw.cpp
  ```

- [ ] **Step 3: 编译项目确认无编译错误**

  ```bash
  pio run
  ```

  预期：编译成功，无错误。

- [ ] **Step 4: Commit 生成的字体数据**

  ```bash
  git add src/generated/device_font_vlw.h src/generated/device_font_vlw.cpp
  git commit -m "feat(fonts): 重新生成 VLW 字体数据（MiSans 多字体源）

  - 使用 MiSans-Medium/Bold/Heavy 分别生成正文/指标/时间字体
  - 设备端代码零改动"
  ```

---

## 自审清单

| Spec 要求 | 对应任务 |
|-----------|---------|
| 时间字体用 MiSans-Heavy @ 42px | Task 3 Step 1-2 |
| 温度/湿度字体用 MiSans-Bold @ 28px | Task 3 Step 1-2 |
| 其他字体用 MiSans-Medium @ 14px | Task 3 Step 1-2 |
| 设备端零改动 | 无设备端任务 |
| 验证生成成功 | Task 4 |
| 验证编译通过 | Task 4 Step 3 |

无占位符，所有步骤包含完整代码和命令。
