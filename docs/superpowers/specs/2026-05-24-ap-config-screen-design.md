# AP 配置屏幕设计

## 1. 背景

HomeDeck 进入 AP 配置模式时，需要在 M5Stack PaperColor 的 E Ink 屏幕上显示一个配置入口页面，供用户扫码连接 Wi-Fi 并打开配置门户。

## 2. 目标

实现一个 AP 配置屏幕，包含以下元素：
1. LOGO 图片（水平居中，顶部 86px）
2. AP 名字（MiSans-Bold 20px，水平居中，距离 LOGO 26px）
3. IP 地址（MiSans-Bold 20px，水平居中，距离 AP 名字 26px）
4. WiFi 二维码（256×256，水平居中，距离 IP 26px）

## 3. 设计方案

### 3.1 方案选择

采用**方案 A：直接屏幕绘制**，理由：
- 所有内容均为运行时确定（AP 名字、IP、二维码数据）
- 无需预生成
- 改动最小，易于测试
- PaperColor 为 e-ink 屏幕，无需局部刷新

### 3.2 屏幕布局（竖屏 400×600）

所有元素**水平居中**，使用 `middle_center` datum：

| 元素 | 尺寸/字体 | 中心 Y 坐标 | 说明 |
|------|----------|-------------|------|
| Logo | 297×40 | `y = 106` | 顶部距离屏幕 86px |
| AP 名字 | MiSans-Bold 20px | `y = 162` | 距离 Logo 底部 26px |
| IP 地址 | MiSans-Bold 20px | `y = 208` | 距离 AP 底部 26px |
| 二维码 | 256×256 | `y = 372` | 距离 IP 底部 26px |

### 3.3 字体生成

新增 **MiSans-Bold 20px** VLW 字体，专用于 AP 配置屏：
- 修改 `tools/generate_device_font.py`，加入第四个 `FontResource`
- 字符集复用现有的 `NUMERIC_TEXT`（AP 名字和 IP 都是 ASCII）
- 生成符号：`kConfigPortalFontVlw`

### 3.4 Logo 资源

按 **PlatformIO 标准方式**：
- 创建 `data/logo.png`（从 `assets/images/logo.png` 复制或软链接）
- `platformio.ini` 添加 `board_build.filesystem = littlefs`
- 运行时 `LittleFS.begin()` 挂载文件系统
- `home_renderer.cpp` 中用 `M5.Display.drawPngFile(LittleFS, "/logo.png", centerX, 106, ..., middle_center)` 绘制

### 3.5 二维码

- **内容格式**：`WIFI:T:nopass;S:HomeDeck-XXXX;;`
- **库**：复用已有的 `QRCode`（已在 `lib_deps`）
- **绘制方式**：计算单个模块大小 `256 / qrcode.size`，用 `fillRect` 绘制黑色模块
- **兼容性**：WiFi 格式二维码在 iOS 11+ 和 Android 9+ 上均受支持

### 3.6 代码结构

修改文件：
- `src/home_renderer.h` / `.cpp`：实现新的 `renderConfigPortal`
- `tools/generate_device_font.py`：添加配置屏幕字体
- `platformio.ini`：添加 LittleFS 支持
- `test/native/support/fake_arduino/M5Unified.h`：扩展 fake 支持
- `test/native/test_home_renderer/test_main.cpp`：更新测试

### 3.7 测试策略

扩展 FakeDisplay / FakeM5Global：
- 记录 `drawPngFile` 调用（路径、坐标、datum）
- 记录二维码绘制（验证 `qrcode_initText` 的字符串正确）
- 验证字体加载（`kConfigPortalFontVlw`）
- 更新 `test_home_renderer`，检查所有元素的坐标和水平居中

## 4. 限制与注意事项

1. **E-ink 屏幕特性**：全屏刷新较慢，但这是配置模式，性能要求不高
2. **二维码格式**：标准二维码只能执行单一操作（连 Wi-Fi 或打开 URL），选择 WiFi 格式让手机自动连接 AP
3. **字体大小**：20px 的 MiSans-Bold 在 400×600 屏幕上可读性良好

## 5. 依赖

- M5Unified
- M5GFX
- QRCode
- LittleFS（ESP32 内置）
- MiSans-Bold.ttf（现有）
