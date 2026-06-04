# 重新设计日历视图（周视图）与 G1 长按刷新设计文档

本设计文档旨在将 HomeDeck 日历视图重构为周视图（而非月视图），为农历应用专属的 12px 字体，并通过 Webcal 获取节日进行缓存，同时在天气和日历视图下引入长按 G1 按键（BtnC）3 秒强制刷新（联网更新天气、NTP 时间、Webcal 节日）的功能。

## 1. 需求分析与约束

### 1.1 硬件与内存约束
*   **屏幕刷新开销**：M5Stack PaperColor 采用 E Ink Spectra 6 墨水屏，刷新耗时久（几秒）且会有彩色闪烁。因此要避免不必要的重绘，并且将按键翻页的上限严格限制。
*   **内存（RAM）限制**：拉取 `.ics` 文件（Webcal）时，文件可能会非常庞大。一次性载入内存会导致 ESP32 的 OOM (Out Of Memory)。必须在 HTTP 读取时使用流式（Streaming）单行解析。
*   **功耗与休眠**：设备在休眠期间不应运行，节日数据必须缓存至 LittleFS，在渲染时仅从本地快速提取。

### 1.2 功能需求
1.  **日历周视图（7列）**：
    *   第一排：星期标签（一、二、三、四、五、六、日）。
    *   第二排：每位日期的数字，当天显示黑色填充圆底和白色文本高亮。
    *   第三排：农历字样（例如：十六、十七、芒种、端午等），使用 12px 的 MiSans-Normal VLW 字体。
    *   第四排：从 Webcal 本地缓存中加载的节日名字，如果该天没有节日但有节气，则备选显示节气。
2.  **界面简化**：
    *   移除日历页面底部的“未来节日”预告和“今天是几月几号”的文本块，仅保留原有的底部温度、湿度、当前时间状态栏。
3.  **按周翻页**：
    *   翻页上限限制在当前周的 **±5周（约前后各1个月）** 以内。
    *   双击 G1（BtnC）重置偏移量归零（即返回当前周）。
4.  **G1 长按 3 秒刷新**：
    *   在天气视图、日历视图下，长按 G1 按钮（BtnC）3 秒，触发强制联网更新（包括：WiFi 连接、天气重新抓取、NTP 时间重新同步、Webcal ICS 节日数据重新下载解析存盘）。

---

## 2. 系统设计与架构

### 2.1 配置参数与 Web Portal (设置页)
在 `SetupConfig` 结构体和 Preferences 存储库中新增 `webcalUrl` 参数，并在网页模板中添加对应的输入控件：

*   **Preferences 键名**：`"webcal_url"`
*   **Web Portal (`scratch/preview.html`)**：
    ```html
    <div class="form-group">
      <label for="webcal_url">节日 Webcal 地址</label>
      <input id="webcal_url" name="webcal_url" value="" placeholder="例如: webcal://example.com/calendar.ics (留空不显示)">
    </div>
    ```
*   **同步脚本 (`tools/sync_preview.py`)**：添加对 `{{WEBCAL_URL}}` 占位符的匹配和检查。
*   **后端保存 (`src/config/setup_page.cpp`)**：提取并填入该值，更新 `ConfigStore` 的加载/保存流程。

### 2.2 12px 农历字库制作
通过 `generate_device_font.py` 生成 12px 农历/节日通用字库：
*   **字体变量**：`kDeviceLunarFontVlw` / `kDeviceLunar`
*   **字号大小**：`12px`
*   **字体源文件**：`fonts/misans/MiSans-Normal.ttf`
*   **字符集**：GB2312 字符集 + Extra Text（涵盖常见的中文字、星期、节假日、农历词汇等）。

### 2.3 Webcal ics 流式解析与缓存
当系统连接 WiFi 触发 Webcal 同步时，使用 `HTTPClient` 以流形式拉取内容：
1.  **流式读取**：获取 `WiFiClientSecure` 流指针，使用 `readStringUntil('\n')` 逐行扫描。
2.  **事件过滤**：
    *   当行内容为 `BEGIN:VEVENT` 时，开始记录状态；
    *   提取并暂存 `DTSTART` (转换为 YYYY-MM-DD) 以及 `SUMMARY`；
    *   当行内容为 `END:VEVENT` 时，如果 `DTSTART` 对应的日期与当前本地时间相差在 **[-40, 40]** 天内，则存入节假日内存 Map 中。
3.  **JSON 缓存存盘**：
    解析结束后，将 Map 序列化成精简的 JSON 文件存盘：`/webcal_cache.json`。格式如下：
    ```json
    {
      "2026-06-01": "儿童节",
      "2026-06-06": "芒种"
    }
    ```

### 2.4 日历逻辑重构 (`CalendarView` & `CalendarData`)
*   **数据结构扩展**：
    `CalendarData` 新增周数据结构，移除不再使用的未来节日预告字段。
*   **渲染逻辑重写**：
    *   计算当前 offset（周单位）下，当前周 of the 7 天日期范围（周一至周日）。
    *   为这 7 天中的每一天提取：星期（一至日）、日期（几号）、农历日期（年鉴数据）、Webcal 缓存中是否存在对应的节日。
    *   使用 20px 字体显示第一排星期、第二排日期；
    *   使用 12px 的 `kDeviceLunarFontVlw` 字体绘制第三排农历和第四排节日。

### 2.5 按键重构与长按强制刷新 (`BootController`)
*   **按周翻页偏移**：
    用 `calendarWeekOffset_` 代替 `calendarMonthOffset_`。当处于日历视图时，BtnA / BtnB 的点击改写为 `calendarWeekOffset_` 增减。
    在范围限制上：`-5 <= calendarWeekOffset_ <= 5`。
*   **长按 3 秒强制刷新检测**：
    在 `app_runtime.cpp` 的 `makeBootDeps()` 中通过 `M5.BtnC.isPressed()` 配合 `M5.BtnC.pressedFor(3000)` 实现长按 3 秒的事件拦截。
    长按触发后：
    1.  显示临时“正在强制刷新中...”界面；
    2.  调用 `connectWifi`，成功后依次执行：天气联网同步、NTP 时间同步、Webcal 下载并解析更新本地缓存；
    3.  `disconnectWifi` 断开连接以省电；
    4.  重新渲染当前视图，若有 offset，则重置为 0；
    5.  重置休眠倒计时。

---

## 3. 验证方案

### 3.1 自动化测试
*   运行本机桩测试 `pio test -e native`。需要在 `test/native` 下补充或更新对应的 fake stub 和测试用例（如周时间计算、流式 ICS 解析核心模块的测试）。

### 3.2 编译与刷写
*   通过 `pio run -e m5stack-papercolor` 进行工程编译，确保无编译错误。
*   刷写设备并查看墨水屏实际显示效果，测试 Webcal 在 Portal 网页上的输入保存。

### 3.3 手动验证
*   **Web Portal 验证**：在配置网页配置 Webcal url 看看是否能成功解析并写入 LittleFS。
*   **刷新验证**：在天气/日历视图下长按 G1 键 3 秒，看是否能够正常联网重新拉取并重绘画面。
*   **翻页验证**：点击 A/B 键向前后翻页，确保在 ±5 周范围后不再允许翻页，双击 C 键能立即归零并刷新。
