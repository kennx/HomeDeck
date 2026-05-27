# PaperColor 字体优化调研与方案

> **调研日期**: 2026-05-27
> **设备**: M5Stack PaperColor (SKU: C151)
> **屏幕**: 4" E Ink Spectra 6 (ED2208-DOA / EL040EF1)

---

## 📋 目录

1. [E Ink Spectra 6 屏幕特性分析](#1-e-ink-spectra-6-屏幕特性分析)
2. [当前字体问题的根因分析](#2-当前字体问题的根因分析)
3. [解决方案](#3-解决方案)
4. [实施计划](#4-实施计划)
5. [参考资料](#5-参考资料)

---

## 1. E Ink Spectra 6 屏幕特性分析

### 1.1 物理参数

| 参数 | 数值 | 说明 |
|------|------|------|
| **分辨率** | 400 × 600 px | 竖屏模式（rotation=0） |
| **活动区域** | 56.4mm × 84.6mm | 来源: Good Display GDEP040E01 数据表 |
| **像素间距** | 0.141 × 0.141 mm | 正方形像素 |
| **PPI** | **180** | 计算验证: 400 / (56.4 / 25.4) ≈ 180.1 |
| **对角线** | 4 英寸 | √(56.4² + 84.6²) ≈ 101.7mm ≈ 4.0" |
| **可显示颜色** | **6 种原色** | 黑、白、红、黄、蓝、绿 |
| **灰度级别** | **2 级** | ⚠️ 仅黑和白，无中间灰度 |
| **刷新时间** | 14–19 秒 | 全刷新 |
| **工作温度** | 0–50°C | |

### 1.2 关键技术特性

#### 色彩系统

E Ink Spectra 6 使用 **ACeP（Advanced Color ePaper）** 技术：

- 每个微杯（microcup）内含 4 种颜色粒子（红、黄、蓝、白），通过电压控制粒子层叠顺序
- 每个物理像素一次只能显示 **6 种颜色之一**（黑、白、红、黄、蓝、绿）
- "全彩" 效果通过 **空间抖动（spatial dithering）** 实现 —— 相邻像素用不同颜色组合来模拟混合色
- 营销宣称的 "60,000 色" 或 "百万色" 均为抖动模拟的视觉效果

#### ⚠️ 灰度能力：仅 2 级

这是影响字体渲染的**最关键特性**：

- Spectra 6 **不支持多级灰度**（不同于 E Ink Carta 的 16 级灰度）
- 每个像素只有 **全黑** 或 **全白** 两种状态
- 中间灰度只能通过 **抖动** 模拟（即在黑白像素间交替排列）
- 对比度约 30:1（远低于 LCD/OLED）

### 1.3 对字体渲染的影响

| LCD/OLED 屏幕 | E Ink Spectra 6 |
|:---:|:---:|
| 256 级灰度 | **仅 2 级（黑/白）** |
| 抗锯齿通过中间灰度平滑边缘 | 灰度像素被量化为黑或白 |
| 300+ PPI 隐藏锯齿 | 180 PPI，锯齿更明显 |
| 抗锯齿 = 更平滑 | 抗锯齿 = **更模糊** |

**结论：在 Spectra 6 上，抗锯齿不但无法提升文字质量，反而是导致模糊的直接原因。**

---

## 2. 当前字体问题的根因分析

### 2.1 问题一：20px VLW 字体模糊

**根因：抗锯齿灰度值在 2 级灰度屏幕上产生伪影**

当前工具链：
1. `font_to_vlw.cpp` 使用 `FT_LOAD_TARGET_NORMAL` 渲染 FreeType 字体
2. 这会生成 **8-bit 灰度 alpha bitmap**（每个像素 0–255）
3. M5GFX 的 `VLWfont::drawChar()` 对每个像素做 **alpha 混合**：
   ```cpp
   int32_t p = 1 + (uint32_t)pixel[j];
   color = (fore * p + back * (257 - p)) >> 8;
   ```
4. 对于像素值为 128 的半透明像素，混合结果是前景色和背景色的中间值
5. **但 Spectra 6 无法显示中间色！**
6. E Ink 驱动会将这些中间色量化（threshold）或抖动

实际视觉效果：
- 字符边缘出现 **"毛刺"/"颗粒感"** — 半透明像素被抖动成黑白交替的噪点
- 小字体（20px）笔画本身就只有 2-3 像素宽，抖动噪点会严重影响可读性
- 整体看起来 **"模糊"** 而非 "锐利"

### 2.2 问题二：日期字体使用 2x 放大

**根因：担心 DRAM OOM，使用 78px 字体 + `setTextSize(2)` 模拟 156px**

当前实现（`home_renderer.cpp` 第 449–456 行）：
```cpp
if (canvas.loadFont(generated::kDeviceLargeDateFontVlw)) {  // 78px, 47KB
    canvas.setTextSize(2);   // 最近邻 2x 放大 → 有效 156px
    canvas.drawString(data.day.c_str(), kCalendarCenterX, kCalendarDayTopY);
    canvas.setTextSize(1);
    canvas.unloadFont();
}
```

**`setTextSize(2)` 的工作原理**（从 M5GFX 源码确认）：
- 每个源像素画成一个 **2×2 的矩形**
- 这是**最近邻缩放**，不做任何插值
- 结果：文字边缘呈现明显的 **阶梯状/方块化** 失真

**OOM 担忧是否成立？**

经过源码分析，**这个担忧是多余的**：

| 数据 | 存储位置 | 占用 RAM |
|------|----------|----------|
| VLW 字节数组 (`const PROGMEM`) | **Flash** (.rodata) | **0 字节** |
| Metric 索引表 | **PSRAM** (heap_alloc_psram) | ~68KB (7541 glyph) |
| Glyph bitmap (渲染时) | **栈** (alloca) | 单个 glyph 大小 |

- `PROGMEM` 在 ESP32-S3 上是空宏，`const` 全局数组自动放入 Flash 的 `.rodata` 段
- ESP32-S3 的 MMU 将 Flash 映射到地址空间，通过 cache 透明访问
- **不占用 DRAM，不占用 PSRAM**
- 16MB Flash 中固件占用约 3MB + 当前字体 2.93MB = ~6MB，还剩 ~10MB
- 即使新增 156px 原生字体（~190KB），也完全不成问题

---

## 3. 解决方案

### 3.1 方案一：字体阈值化（解决 20px 模糊）

**核心思路**：在字体生成阶段将灰度 bitmap 转为 1-bit 二值 bitmap

**最佳方法**：修改 `font_to_vlw.cpp`，使用 `FT_LOAD_TARGET_MONO` + 解包

```
FT_LOAD_TARGET_NORMAL (当前)     FT_LOAD_TARGET_MONO (建议)
┌─────────────────┐              ┌─────────────────┐
│ 灰度 hinting    │              │ 单色 hinting    │
│ (优化平滑过渡)   │              │ (优化像素对齐)   │
│                 │              │                 │
│ 0-255 灰度值    │              │ 0 或 1 值       │
│ → 在 E-Ink 上   │              │ → 在 E-Ink 上   │
│   产生抖动噪点   │              │   显示锐利清晰   │
└─────────────────┘              └─────────────────┘
```

**为什么 `FT_LOAD_TARGET_MONO` 优于简单阈值化？**

- `FT_LOAD_TARGET_MONO` 让 FreeType 的 hinter 专门为 **1-bit 输出优化网格对齐**
- 笔画宽度会对齐到整像素边界（如 1.3px 的笔画 → 1px 或 2px，而非灰度模糊）
- 简单阈值化（`pixel >= 128 ? 0xFF : 0x00`）保留了灰度 hinting 的网格对齐，不如 mono hinting 精确

**实现修改**：

`font_to_vlw.cpp` 中 `renderGlyph()`:
```cpp
// 改为：
FT_Load_Char(face, code_point, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO)
```

`copyBitmap()` 需要修改以解包 1-bit packed 格式到 8-bit VLW 格式：
```cpp
// FT_PIXEL_MODE_MONO: 每字节 8 个像素, MSB first
// 解包为 VLW 的 8-bit 格式 (0x00=透明, 0xFF=不透明)
for (unsigned int y = 0; y < bitmap.rows; ++y) {
    const unsigned char* row = bitmap.buffer + y * std::abs(bitmap.pitch);
    for (unsigned int x = 0; x < bitmap.width; ++x) {
        bool set = (row[x / 8] >> (7 - (x % 8))) & 1;
        bytes.push_back(set ? 0xFF : 0x00);
    }
}
```

这样 **M5GFX 渲染端完全不需要修改** — VLW 格式仍然是 8-bit per pixel，只是值只有 0x00 和 0xFF。

### 3.2 方案二：原生 156px 日期字体（解决 2x 放大）

**核心思路**：直接生成 156px 的 VLW 字体，去掉 `setTextSize(2)`

**修改 `generate_device_font.py`**：

```python
# 将 device_large_date_font 的 pixel_size 从 78 改为 156
FontResource(
    "device_large_date_font",
    "kDeviceLargeDate",
    156,                          # 原来是 78
    sorted(numeric_codepoints),
    TIME_FONT,
)
```

**修改 `home_renderer.cpp`**：

```cpp
if (canvas.loadFont(generated::kDeviceLargeDateFontVlw)) {
    canvas.setTextColor(themeColor, TFT_WHITE);
    canvas.setTextDatum(textdatum_t::top_center);
    // 删除 setTextSize(2) — 字体已经是 156px
    canvas.drawString(data.day.c_str(), kCalendarCenterX, kCalendarDayTopY);
    canvas.unloadFont();
}
```

**资源开销估算**：

| | 78px (当前) | 156px (改后) | 增量 |
|---|---|---|---|
| 单个数字 glyph 大小 | ~78 × 50 = 3.9KB | ~156 × 100 = 15.6KB | +11.7KB |
| 19 个 glyph 总大小 | 47KB | **~190KB** | **+143KB** |
| Flash 占用影响 | | | 可忽略 (16MB Flash) |
| DRAM 占用影响 | | | **0** (PROGMEM → Flash) |
| 渲染时栈占用 | 3.9KB (alloca) | **15.6KB** | 需确认栈大小足够 |

> ⚠️ **栈空间注意**：`alloca(w * h)` 在渲染 156px 字体时需要约 15.6KB 栈空间。ESP32 默认任务栈 8KB 可能不够。但由于只有数字 glyph（宽度约为高度的 60%），实际约 156 × 94 ≈ 14.7KB，且 Arduino 的 `loopTask` 默认栈为 8192 字节，**这可能导致栈溢出**。
>
> **解决方案**：在 `platformio.ini` 中增大主任务栈大小：
> ```ini
> build_flags =
>   ...
>   -DARDUINO_LOOP_STACK_SIZE=32768
> ```

### 3.3 方案组合效果预期

| 修改 | 效果 |
|------|------|
| FT_LOAD_TARGET_MONO | 20px 中文字体边缘从 "模糊抖动" 变为 **锐利清晰** |
| 原生 156px 字体 | 日期数字从 "方块化阶梯" 变为 **原生渲染质量** |
| 两者结合 | 所有字体在 Spectra 6 上达到**最佳显示效果** |

---

## 4. 实施计划

### Phase 1: 修改字体生成工具

1. **修改 `tools/font_to_vlw.cpp`**
   - `renderGlyph()` 改用 `FT_LOAD_TARGET_MONO | FT_LOAD_RENDER`
   - `copyBitmap()` 支持 `FT_PIXEL_MODE_MONO` 解包到 8-bit
   - 通过编译参数或命令行标志控制渲染模式（可选，便于 A/B 对比）

2. **修改 `tools/generate_device_font.py`**
   - `device_large_date_font` 的 `pixel_size` 从 78 改为 156

### Phase 2: 修改渲染代码

3. **修改 `src/home_renderer.cpp`**
   - 删除 `canvas.setTextSize(2)` 和配对的 `canvas.setTextSize(1)`

### Phase 3: 调整构建配置

4. **修改 `platformio.ini`**
   - 添加 `-DARDUINO_LOOP_STACK_SIZE=32768` 以支持大字体渲染时的栈分配

### Phase 4: 重新生成字体并验证

5. 运行 `python tools/generate_device_font.py` 重新生成所有字体
6. 编译烧录，在实际屏幕上验证效果
7. 可选：对比 `FT_LOAD_TARGET_MONO` vs 阈值化的显示效果

---

## 5. 参考资料

### E Ink Spectra 6 技术文档

| 来源 | 链接 |
|------|------|
| Good Display GDEP040E01 数据表 | https://www.good-display.com/product/532.html |
| Waveshare 4" E Ink Spectra 6 | https://www.waveshare.com/4inch-e-paper-hat-plus-e.htm |
| BECK Electronics Spectra 6 规格 | https://www.beck-elektronik.de/en/newsroom/news/article/e-ink-spectratm-6-der-hingucker-des-jahres-2024 |
| E Ink 官方技术博客 | https://blog.eink.com/evolution-to-conquering-color |
| Viwoods Gallery 3 vs Kaleido 3 对比 | https://viwoods.com/blogs/ereader/kaleido-3-e-ink-display-guide |

### M5GFX / LovyanGFX 源码

| 文件 | 说明 |
|------|------|
| `lgfx_fonts.cpp` L1903-2089 | VLW drawChar() alpha 混合渲染逻辑 |
| `lgfx_fonts.hpp` L309-341 | VLWfont 结构定义 |
| `LGFXBase.cpp` L2543-2619 | loadFont() 入口 |
| `LGFX_Sprite.cpp` | createSprite/setPsram |
| `platforms/esp32/common.hpp` L145 | heap_alloc_psram() |

### FreeType 渲染模式

| 模式 | 说明 |
|------|------|
| `FT_LOAD_TARGET_NORMAL` | 灰度 hinting，生成 8-bit 灰度 bitmap |
| `FT_LOAD_TARGET_MONO` | 单色 hinting，生成 1-bit packed bitmap |
| `FT_LOAD_TARGET_LIGHT` | 轻量 hinting，保留更多字体原始形状 |

### E Ink 字体最佳实践

| 来源 | 链接 |
|------|------|
| MobileRead 抗锯齿讨论 | https://www.mobileread.com/forums/showthread.php?t=371957 |
| StackExchange E-Ink 字体选择 | https://graphicdesign.stackexchange.com/questions/11372 |

### ESP32-S3 内存模型

| 来源 | 链接 |
|------|------|
| ThingPulse PSRAM 教程 | https://thingpulse.com/esp32-how-to-use-psram |
| Daniel Mangum 静态分配 | https://danielmangum.com/posts/static-alloc-external-ram-esp32 |
| LVGL PSRAM 讨论 | https://forum.lvgl.io/t/help-setting-correct-memory-placement-to-save-ram-in-esp32-s3-platformio/13154 |

---

## 附录 A: PPI 计算验证

```
水平 PPI = 400 / (56.4 / 25.4) = 400 / 2.2205 ≈ 180.1
垂直 PPI = 600 / (84.6 / 25.4) = 600 / 3.3307 ≈ 180.1
对角线 = √(400² + 600²) / √(56.4² + 84.6²) mm
       = 721.1 / 101.7mm = 721.1 / 4.003" ≈ 180.1 PPI
```

## 附录 B: VLW 字体格式

```
┌────────────────────────────────────────┐
│ Header (6 × uint32 BE = 24 bytes)      │
│  glyphCount | version=11 | yAdvance    │
│  0          | ascent     | descent     │
├────────────────────────────────────────┤
│ Glyph Metrics (N × 7 × uint32 = 28N)  │
│  [0] unicode|height|width|xAdv|dY|dX|0 │
│  [1] ...                               │
│  [N-1] ...                             │
├────────────────────────────────────────┤
│ Glyph Bitmaps (连续存储)                │
│  [0] W₀×H₀ bytes (8-bit alpha)        │
│  [1] W₁×H₁ bytes                      │
│  ...                                   │
│  [N-1] Wₙ₋₁×Hₙ₋₁ bytes               │
└────────────────────────────────────────┘
```

## 附录 C: 字体像素大小与实际尺寸换算 (180 PPI)

| pt | px (≈ pt × 180/72) | 物理高度 (mm) | 适用场景 |
|----|-----|------|------|
| 8 | 20 | 2.8 | 当前正文字号（偏小，但可用） |
| 10 | 25 | 3.5 | 推荐最小中文字号 |
| 12 | 30 | 4.2 | 舒适阅读正文 |
| 16 | 40 | 5.6 | 标题 |
| 62 | 156 | 22.0 | 日期大数字 |

## 附录 D: 当前字体资源清单

| 字体 | 像素大小 | Glyph 数 | VLW 大小 | 存储位置 | 用途 |
|------|---------|----------|---------|---------|------|
| kDeviceFontVlw | 20px | 7541 | 2.7MB | Flash | 主字体（中文+ASCII） |
| kDeviceLargeDateFontVlw | 78px → ×2=156px | 19 | 47KB | Flash | 日期数字 |
| kDeviceMetricFontVlw | 28px | 19 | 6.4KB | Flash | 温湿度数据 |
| kDeviceTimeFontVlw | 42px | 19 | 14KB | Flash | 时间显示 |
| kConfigPortalFontVlw | 20px | 95 | 18KB | Flash | 配置页面 |
| **总计** | | | **~2.93MB** | | 16MB Flash 中仅占 18% |
