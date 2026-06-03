# 2026-06-03-smooth-font-design

本项目为 M5Stack PaperColor 设备的字形生成和渲染引入灰度抗锯齿平滑字体，解决小字体（BODY）在 E-Paper 屏幕上锯齿严重、边缘不够平滑的问题。

## 1. 背景与目标
在当前的 HomeDeck 固件中，设备端所有的 VLW 字体（包括 20px 正文、大日期、时间等）均是由编译期的 FreeType 栅格化工具以 1-bit（纯黑白二值化）模式生成的。这导致在屏幕显示时边缘没有平滑的灰度过渡，高 DPI 屏幕下的锯齿感极强。
本设计的目的是通过更改 FreeType 栅格化参数，使生成的 VLW 字体包含 8-bit 灰度（Alpha）通道数据，利用 LovyanGFX 在 16-bit 绘图画布（Canvas）上的 Alpha 混合技术与屏幕的色彩抖动，在物理屏幕上呈现出平滑的字体边缘，同时不增加任何 Flash 的存储开销。

## 2. 提议的变更

### 2.1. 字体编码转换工具 (tools)
修改 [font_to_vlw.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/tools/font_to_vlw.cpp) 中加载和渲染字形时的 FreeType 标志：
- 移除 `FT_LOAD_TARGET_MONO`，仅使用 `FT_LOAD_RENDER` 渲染字形。
- FreeType 将自动按默认的 `FT_LOAD_TARGET_NORMAL` 方式输出灰度级位图 (`FT_PIXEL_MODE_GRAY`)。
- 原有的 `copyBitmap` 函数已经具备处理 `FT_PIXEL_MODE_GRAY` 的逻辑，它将字形的 8-bit 透明度灰度值（0-255）完整保留下来，存入最终生成的 VLW 格式的字形数组中。

#### [MODIFY] [font_to_vlw.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/tools/font_to_vlw.cpp)
```diff
@@ -151,3 +151,3 @@
 Glyph renderGlyph(FT_Face face, std::uint32_t code_point) {
-  if (FT_Load_Char(face, code_point, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) !=
+  if (FT_Load_Char(face, code_point, FT_LOAD_RENDER) !=
       0) {
```

### 2.2. 资源生成 (src/generated)
使用 Python 脚本生成器重新编译字形工具并更新全部 5 个设备字体：
- 运行 `python tools/generate_device_font.py`
- 将重新生成以下资源文件：
  - [device_font_vlw.cpp](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/generated/device_font_vlw.cpp)
  - [device_font_vlw.h](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/src/generated/device_font_vlw.h)
- 经对比，生成的 vlw 数据二进制数组总大小不变，因为在 vlw 格式的实现中，之前的 Mono 模式也是以每像素 1 字节（0x00 或 0xFF）对齐存储，现在仅把 0xFF/0x00 替换为了 0-255 的中间透明度。

---

## 3. 验证计划

### 3.1. 自动化单元测试 (Host-side Unit Tests)
运行宿主机单元测试以验证字体的格式解析正常：
```bash
pio test -e native
```
确保相关的 native 测试用例通过，尤其是对生成的 `kDeviceFontVlw` 字体资源做验证的单元测试：
- [test_device_font_resource](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_device_font_resource/test_main.cpp)
- [test_home_renderer](file:///Users/kenn/PROJECTS/m5stack/HomeDeck/test/native/test_home_renderer/test_main.cpp)

### 3.2. 设备端编译构建 (Target Compilation)
运行 PlatformIO 编译，确保在 ESP32 目标架构下成功构建：
```bash
pio run -e m5stack-papercolor
```
确保无编译错误、PSRAM 与 Flash 的内存使用符合规范。

### 3.3. 手动验证 (Manual Verification)
重新烧录固件到 M5Stack PaperColor 硬件上，在屏幕上观察：
- 20px 正文小字体 (BODY) 的边缘锯齿感明显消除，呈现平滑过渡。
- 其他大字体（时间、大日期、度量数字）无显示破损或渲染错位。
