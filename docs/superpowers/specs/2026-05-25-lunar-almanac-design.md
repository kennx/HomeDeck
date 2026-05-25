# 老黄历完整功能设计

## 背景

HomeDeck 当前 `feat/lunar-calendar` 分支已经实现了接近 Figma 的老黄历首页渲染：400x600 竖屏、红/绿主题、大日期、农历信息、干支信息、黄历表格和动态宜忌行高度。现状缺口是黄历数据仍为占位值，`makeHomeCalendarData()` 只根据本地时间生成公历年、月、日和星期。

本设计把老黄历做成运行期完全离线的功能：生成期使用固定版本的 `6tail/lunar-python` 作为 golden source 生成 1900-2100 的黄历数据包，设备端从 LittleFS 读取当天记录并填充首页字段。

## 目标

- 以 Figma 节点 `18:2` 的字段为完整功能边界。
- 覆盖公历日期 `1900-01-01` 到 `2100-12-31`。
- 设备运行期不依赖网络、Python、外部 API 或动态计算完整黄历规则。
- 黄历数据通过 LittleFS 数据文件提供。
- 数据缺失或损坏时，首页仍显示 Figma 版式和公历日期，黄历字段显示明确占位。
- 保留当前渲染层已有的 Figma 坐标、字体、动态宜忌高度和红/绿主题行为。

## 非目标

- 不新增 Figma 之外的首页字段，例如节假日名称、今日提示、运势说明或天气。
- 不改变换日刷新、deep sleep、NTP 节流或配置流程。
- 不在设备端实现完整黄历规则引擎。
- 不把 `lunar-python` 或 Python 运行时放进固件。
- 不把 1900-2100 的每日黄历直接编译成巨型 C++ 常量数组。

## 字段边界

首页需要真实填充以下字段：

- 公历年：例如 `2026 年`。
- 公历月：例如 `十二月`。
- 星期：例如 `星期日`。
- 日号：例如 `21`。
- 农历日期：例如 `四月初六`。
- 节气：当天有节气时显示，例如 `小满`；没有节气时为空。
- 干支与生肖日：例如 `丙午年 癸巳月 丁酉日 鸡日`。
- 五行：例如 `五行炉中火`。
- 冲煞：例如 `冲猴煞北`。
- 值神：例如 `值神白虎`。
- 建除：例如 `建除成日`。
- 胎神：例如 `胎神厨灶炉外正南`。
- 宜：空格分隔的日宜词条。
- 忌：空格分隔的日忌词条。

`isHoliday` 仍先沿用当前行为：周六、周日使用红色主题，工作日使用绿色主题。本功能不扩展法定节假日主题。

## Golden Source

生成期固定使用 `6tail/lunar-python` 作为 golden source。选择理由：

- Context7 文档显示它提供农历、节气、精确干支、纳音、冲煞、值神、建除、宜忌等 API。
- GitHub 和 PyPI 均标注 MIT license。
- PyPI 当前可见版本包含 2025 年内更新，适合作为 2026 年语境下的生成期依赖候选。

生成器依赖锁定为 `lunar_python==1.4.8`，并在生成 manifest 中记录：

- generator format version。
- `lunar-python` package version。
- 数据覆盖起止日期。
- 记录数量。
- 数据文件校验值。
- 生成时间或生成脚本版本。
- MIT license 归属说明。

参考来源：

- `https://github.com/6tail/lunar-python`
- `https://pypi.org/project/lunar_python/`

## 架构

### 生成期数据层

新增生成脚本 `tools/generate_almanac_data.py`。脚本负责：

1. 使用锁定版本的 `lunar-python` 遍历 `1900-01-01` 到 `2100-12-31`。
2. 生成每天的 Figma 所需字段。
3. 对重复文本建立字符串表。
4. 输出紧凑二进制数据文件 `data/almanac.bin`。
5. 输出或内嵌 manifest/header，用于运行时验证。
6. 生成 golden snapshot 测试使用的样例数据。

### 设备端读取层

新增独立模块 `AlmanacProvider`：

- 只负责从 LittleFS 读取和解码 `almanac.bin`。
- 输入为公历年月日。
- 输出为当天黄历字段或明确的缺失状态。
- 不负责屏幕绘制。
- 不负责网络、NTP、RTC 或配置流程。

读取流程：

1. 校验文件头 magic、格式版本、日期范围、记录数量和校验值。
2. 将公历日期转换为从 `1900-01-01` 起的 `dayOffset`。
3. 按 `recordOffset = recordsStart + dayOffset * recordSize` 定位每日记录。
4. 解码字符串索引和宜忌词条索引。
5. 拼接 `HomeCalendarData` 所需字段。

### 首页组装层

`makeHomeCalendarData(const std::tm& localTime)` 继续作为首页数据组装入口：

1. 先根据 `std::tm` 生成公历年、月、日、星期和周末主题。
2. 调用 `AlmanacProvider` 查询当天黄历。
3. 命中时填充 Figma 所有黄历字段。
4. 缺失时填充明确占位。

`HomeRenderer::render(const HomeCalendarData&)` 保持渲染职责，只消费数据，不读取 LittleFS 黄历文件。

## 数据格式

数据包采用紧凑二进制格式，并包含字符串表。

文件头包含：

- Magic：`HDALM001`。
- Format version。
- Start date：`1900-01-01`。
- End date：`2100-12-31`。
- Day count。
- Record size。
- Records offset。
- String table offset。
- String table size。
- CRC32 校验。

每日记录采用固定大小，字段以字符串表索引为主：

- 农历日期索引。
- 节气索引，空字符串代表无节气。
- 干支行索引。
- 五行索引。
- 冲煞索引。
- 值神索引。
- 建除索引。
- 胎神索引。
- 宜词条数量。
- 宜词条索引列表。
- 忌词条数量。
- 忌词条索引列表。

宜忌内容在设备端以空格拼接，继续复用当前按字符宽度换行的渲染算法。

如果实现时发现每日宜忌词条上限需要变化，生成器应先扫描全量数据并把最大值写入 header；设备端按 header 校验，避免静默截断。

## 字段映射

生成器使用以下口径生成显示文本：

- `lunarDate`：农历月中文名 + 农历日中文名；闰月加 `闰`。
- `solarTerm`：`lunar.getJieQi()`；无节气时为空。
- `ganzhi`：`年干支年 月干支月 日干支日 生肖日`。
- `wuxing`：`五行` + 日纳音，优先使用 `lunar.getDayNaYin()`。
- `chongsha`：`冲` + 日冲生肖 + `煞` + 日煞方向。
- `zhishen`：`值神` + 日天神。
- `jianchu`：`建除` + 建除十二值星 + `日`。
- `taishen`：`胎神` + `lunar.getDayPositionTai()`。
- `yi`：`lunar.getDayYi()` 返回词条以空格连接。
- `ji`：`lunar.getDayJi()` 返回词条以空格连接。

`lunar-python` 源码中 `getDayPositionTai()` 返回日胎神方位。若库 API 返回值已包含前缀，生成器负责归一化，确保设备端只拿到最终显示文本。

## 错误处理

黄历查询返回三类结果：

### 命中数据

首页显示真实黄历字段。公历字段仍由设备本地时间生成。

### 数据缺失或损坏

首页保留 Figma 版式，公历字段照常显示。黄历字段使用占位：

- 农历：`数据缺失`
- 干支：`黄历数据缺失`
- 五行：`五行暂无`
- 冲煞：`冲煞暂无`
- 值神：`值神暂无`
- 建除：`建除暂无`
- 胎神：`胎神暂无`
- 宜：`暂无`
- 忌：`暂无`

### 日期超出范围

同样走缺失状态，不进行越界 seek。

## 测试策略

### 生成器测试

- 验证 `almanac.bin` header 的 magic、版本、日期范围、记录数、offset 和校验值。
- 验证字符串表可以完整解码 UTF-8 文本。
- 验证全量记录没有越界字符串索引。
- 固定 golden dates：
  - Figma 日期：`2026-12-21`。
  - 一个农历新年。
  - 一个闰月日期。
  - 一个节气日。
  - 一个普通日。
- Golden snapshot 覆盖 Figma 所需所有字段。

### Native 测试

- `AlmanacProvider` 能读取已知日期。
- 文件缺失时返回缺失状态。
- magic 或 format version 不匹配时返回缺失状态。
- 日期早于 `1900-01-01` 或晚于 `2100-12-31` 时返回缺失状态。
- `makeHomeCalendarData()` 对固定日期能生成真实黄历字段。
- `makeHomeCalendarData()` 在数据缺失时保留公历字段并填充占位字段。

### 渲染测试

继续复用当前 `test_home_renderer` 对 Figma 坐标、字体、表格线、主题色和动态宜忌高度的测试。只在必要时调整样例数据，使它更接近真实黄历输出。

## 验收命令

```bash
python3 tools/generate_almanac_data.py
pio test -e native
pio run -e m5stack-papercolor
pio run -e m5stack-papercolor -t buildfs
git diff --check
```

## 实施顺序

1. 锁定生成期依赖和字段映射，写生成器 golden tests。
2. 实现 `tools/generate_almanac_data.py` 并生成 `data/almanac.bin`。
3. 为 native 环境扩展 LittleFS 文件读取 fake 能力。
4. 实现 `AlmanacProvider` 的 header 校验、日期定位和记录解码。
5. 将 `AlmanacProvider` 接入 `makeHomeCalendarData()`。
6. 扩展首页数据测试和缺失状态测试。
7. 复跑生成器、native 测试、设备构建、LittleFS 构建和 diff 检查。
