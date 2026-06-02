# HomeDeck 配置页面位置获取功能设计

## 概述

在 HomeDeck 配置模式的 Web 配置页面中，添加一个「获取当前位置」按钮。用户点击后，通过浏览器 Geolocation API 获取设备经纬度坐标，填入表单中的可编辑输入框，随配置一同保存到 ESP32 NVS 中，为后续天气显示功能提供位置数据。

## 背景和目标

- **目的**：为天气显示功能准备位置数据（经纬度），用于调用天气 API。
- **场景**：用户在配置模式下连接 HomeDeck 的 AP，打开配置页面，点击按钮获取当前位置或手动输入坐标。
- **约束**：ESP32 墨水屏设备，深度睡眠架构，配置页面为服务器端渲染的 HTML（非 SPA）。

## 设计决策

采用**方案 A（最小改动）**：将经纬度作为 `SetupConfig` 的字符串字段，与 WiFi/时区同等级别配置项，随表单一起保存。不新增 HTTP endpoint 或存储抽象。

**理由**：
- 改动量最小，与现有配置系统完全兼容
- 坐标天然属于"设备配置"范畴，无需独立存储层
- 用户交互简单直观：点击按钮 → 获取坐标 → 填入输入框 → 随表单保存

## 详细设计

### 4.1 数据模型（`src/config/config_types.h`）

`SetupConfig` 新增两个字段：

```cpp
struct SetupConfig {
  std::string wifiSsid;
  std::string wifiPassword;
  std::string timezoneIana = "Asia/Shanghai";
  bool autoRtcCorrection = false;
  std::string ntpServer = "pool.ntp.org";
  std::string latitude;      // 新增：纬度
  std::string longitude;     // 新增：经度
};
```

使用 `std::string` 而非 `double`，与现有字段风格保持一致，HTML 表单直接透传无需转换。

`ConfigValidationError` 枚举新增两个值：

```cpp
enum class ConfigValidationError {
  None,
  MissingManualDateTime,
  MissingNtpServer,
  InvalidManualDateTime,
  InvalidTimezone,
  InvalidLatitude,    // 新增
  InvalidLongitude,   // 新增
};
```

### 4.2 前端 UI 与交互（`src/config/setup_page.cpp`）

在表单「手动日期时间」之后、「保存」按钮之前，新增位置区域：

**HTML：**

```html
<label>纬度 <input id="latitude" name="latitude" value="{lat}"></label>
<label>经度 <input id="longitude" name="longitude" value="{lon}"></label>
<button type="button" id="get_location">获取当前位置</button>
<span id="location_status"></span>
```

输入框预填充逻辑：
- 若 `SetupConfig` 中已保存坐标 → 显示保存值
- 若未保存（空字符串）→ 显示上海黄浦区默认值 `31.2304` / `121.4737`

**CSS 新增：**

```css
#location_status{display:block;margin-top:4px;font-size:14px}
#location_status.ok{color:#2e7d32}
#location_status.err{color:#b00020}
```

**JavaScript 新增**（嵌入现有 `<script>` 标签）：

```javascript
const latInput = document.getElementById('latitude');
const lonInput = document.getElementById('longitude');
const locStatus = document.getElementById('location_status');
document.getElementById('get_location').addEventListener('click', function() {
    if (!navigator.geolocation) {
        locStatus.textContent = '浏览器不支持地理定位';
        locStatus.className = 'err';
        return;
    }
    locStatus.textContent = '正在获取位置...';
    locStatus.className = '';
    navigator.geolocation.getCurrentPosition(
        function(pos) {
            latInput.value = pos.coords.latitude.toFixed(6);
            lonInput.value = pos.coords.longitude.toFixed(6);
            locStatus.textContent = '位置已获取';
            locStatus.className = 'ok';
        },
        function(err) {
            var msgs = {1:'权限被拒绝', 2:'位置不可用', 3:'获取超时'};
            locStatus.textContent = msgs[err.code] || '获取失败';
            locStatus.className = 'err';
        },
        {timeout: 10000, enableHighAccuracy: false}
    );
});
```

参数说明：
- `toFixed(6)`：精度约 0.1 米，对天气 API 足够
- `timeout: 10000`：10 秒超时
- `enableHighAccuracy: false`：降低精度要求，加快响应速度

### 4.3 后端请求处理（`src/config/config_portal.cpp`）

`readConfigFromRequest()` 新增坐标字段读取：

```cpp
SetupConfig ConfigPortal::readConfigFromRequest() {
  SetupConfig config{};
  // ... 现有字段读取 ...
  config.latitude = server_.arg("latitude").c_str();
  config.longitude = server_.arg("longitude").c_str();
  return config;
}
```

无需新增 endpoint，坐标随 `/save` POST 请求一起提交、验证、保存。

### 4.4 持久化存储（`src/config/config_store.cpp`）

**NVS key 常量：**

```cpp
constexpr const char* kLatitude = "lat";
constexpr const char* kLongitude = "lon";
```

**读取（默认值与前端一致）：**

```cpp
config.latitude = prefs_.getString(kLatitude, "31.2304").c_str();
config.longitude = prefs_.getString(kLongitude, "121.4737").c_str();
```

**写入：**

```cpp
const bool latOk = prefs_.putString(kLatitude, config.latitude.c_str()) > 0 || config.latitude.empty();
const bool lonOk = prefs_.putString(kLongitude, config.longitude.c_str()) > 0 || config.longitude.empty();
return stringsOk && passwordOk && timezoneOk && ntpOk && boolOk && latOk && lonOk;
```

### 4.5 输入验证（`src/config/config_validator.cpp/h`）

**新增辅助函数：**

```cpp
bool parseLatitude(std::string_view value, double* out) {
  if (value.empty()) return true;  // 空值允许（使用默认）
  char* end = nullptr;
  const double d = std::strtod(std::string(value).c_str(), &end);
  if (*end != '\0') return false;
  if (d < -90.0 || d > 90.0) return false;
  if (out) *out = d;
  return true;
}

bool parseLongitude(std::string_view value, double* out) {
  if (value.empty()) return true;
  char* end = nullptr;
  const double d = std::strtod(std::string(value).c_str(), &end);
  if (*end != '\0') return false;
  if (d < -180.0 || d > 180.0) return false;
  if (out) *out = d;
  return true;
}
```

**`validateSetupSubmission()` 新增验证：**

```cpp
if (!parseLatitude(config.latitude, nullptr)) {
  return makeError(ConfigValidationError::InvalidLatitude, "纬度格式无效（范围：-90 ~ 90）。");
}
if (!parseLongitude(config.longitude, nullptr)) {
  return makeError(ConfigValidationError::InvalidLongitude, "经度格式无效（范围：-180 ~ 180）。");
}
```

验证规则：
- 空值：允许（使用设备默认值）
- 非空值：必须是有效数字，且在规定范围内
- 坐标为可选配置项，不强制填写

## 测试计划

### 单元测试更新

1. **`test/native/test_config_validation/test_main.cpp`**
   - 新增 `test_valid_latitude_longitude`：有效坐标通过验证
   - 新增 `test_invalid_latitude_rejected`：超出范围的纬度被拒绝
   - 新增 `test_invalid_longitude_rejected`：超出范围的经度被拒绝
   - 新增 `test_empty_latitude_longitude_allowed`：空坐标允许通过

2. **`test/native/test_config_store/test_main.cpp`**
   - 更新 `test_load_defaults_when_empty`：验证默认坐标为上海黄浦区
   - 更新 `test_save_and_load_config_and_flags`：验证坐标保存和读取

3. **`test/native/test_setup_page/test_main.cpp`**
   - 新增 `test_setup_page_contains_location_fields`：验证 HTML 包含经纬度输入框和获取按钮

### 集成验证

- `pio run -e m5stack-papercolor`：确保固件编译通过
- `pio test -e native`：确保所有本机单元测试通过

## 涉及文件清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `src/config/config_types.h` | 修改 | SetupConfig 新增 latitude/longitude；枚举新增错误类型 |
| `src/config/setup_page.cpp` | 修改 | HTML 添加位置输入框、按钮、JS、CSS |
| `src/config/config_portal.cpp` | 修改 | readConfigFromRequest 读取坐标字段 |
| `src/config/config_store.cpp` | 修改 | NVS 读写坐标字段 |
| `src/config/config_validator.h` | 修改 | 新增 parseLatitude/parseLongitude 声明 |
| `src/config/config_validator.cpp` | 修改 | 实现坐标解析和验证逻辑 |
| `test/native/test_config_validation/test_main.cpp` | 修改 | 新增坐标验证测试 |
| `test/native/test_config_store/test_main.cpp` | 修改 | 更新默认坐标和保存读取测试 |
| `test/native/test_setup_page/test_main.cpp` | 修改 | 新增页面元素存在性测试 |

## 默认坐标

- **纬度**：`31.2304`
- **经度**：`121.4737`
- **位置**：上海市黄浦区（人民广场附近）
- **说明**：设备首次启动或用户未配置位置时，使用此默认值确保天气功能可用。
