# HomeDeck 配置页面位置获取功能实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 HomeDeck 配置模式的 Web 配置页面中添加「获取当前位置」按钮，通过浏览器 Geolocation API 获取经纬度，与配置一同保存到 NVS，为天气功能提供位置数据。

**Architecture:** 采用最小改动方案（方案 A）。将经纬度作为 `SetupConfig` 的字符串字段，与 WiFi/时区同等级别配置项，随表单一起保存。不新增 HTTP endpoint 或存储抽象。

**Tech Stack:** ESP32 Arduino 框架, PlatformIO, C++17, Unity 测试框架, 浏览器 Geolocation API

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `src/config/config_types.h` | 修改 | `SetupConfig` 新增 `latitude`/`longitude`；`ConfigValidationError` 新增两个错误类型 |
| `src/config/config_validator.h` | 修改 | 新增 `parseLatitude`/`parseLongitude` 声明 |
| `src/config/config_validator.cpp` | 修改 | 实现坐标解析函数；在 `validateSetupSubmission` 中调用验证 |
| `src/config/config_store.cpp` | 修改 | NVS 读写坐标字段（key: `lat`/`lon`，默认上海黄浦区坐标） |
| `src/config/setup_page.cpp` | 修改 | HTML 添加位置输入框、获取按钮、Geolocation JS、状态样式 |
| `src/config/config_portal.cpp` | 修改 | `readConfigFromRequest()` 读取表单中的 `latitude`/`longitude` |
| `test/native/test_config_validation/test_main.cpp` | 修改 | 新增坐标验证测试（有效、无效、空值、格式错误） |
| `test/native/test_config_store/test_main.cpp` | 修改 | 更新默认坐标测试和保存读取测试 |
| `test/native/test_setup_page/test_main.cpp` | 修改 | 新增页面元素存在性测试 |

---

## Task 1: 数据模型与坐标验证

**Files:**
- Modify: `src/config/config_types.h`
- Modify: `src/config/config_validator.h`
- Modify: `src/config/config_validator.cpp`
- Test: `test/native/test_config_validation/test_main.cpp`

---

- [ ] **Step 1: 在 config_types.h 的 SetupConfig 中添加坐标字段**

```cpp
struct SetupConfig {
  std::string wifiSsid;
  std::string wifiPassword;
  std::string timezoneIana = "Asia/Shanghai";
  bool autoRtcCorrection = false;
  std::string ntpServer = "pool.ntp.org";
  std::string latitude;   // 新增
  std::string longitude;  // 新增
};
```

- [ ] **Step 2: 在 config_types.h 的枚举中添加坐标错误类型**

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

- [ ] **Step 3: 在 config_validator.h 中添加坐标解析函数声明**

```cpp
bool parseLatitude(std::string_view value, double* out);
bool parseLongitude(std::string_view value, double* out);
```

- [ ] **Step 4: 写失败测试 —— test_config_validation 新增坐标验证测试**

在 `test/native/test_config_validation/test_main.cpp` 中，在 `test_timezone_catalog_maps_asia_shanghai` 之前添加以下测试函数，并在 `main()` 的 `RUN_TEST` 列表末尾添加对应的 `RUN_TEST` 调用：

```cpp
void test_valid_latitude_longitude() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.wifiSsid = "Home";
  config.ntpServer = "pool.ntp.org";
  config.latitude = "31.2304";
  config.longitude = "121.4737";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::None, result.error);
}

void test_invalid_latitude_rejected() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.wifiSsid = "Home";
  config.ntpServer = "pool.ntp.org";
  config.latitude = "91.0";
  config.longitude = "121.4737";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::InvalidLatitude, result.error);
}

void test_invalid_longitude_rejected() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.wifiSsid = "Home";
  config.ntpServer = "pool.ntp.org";
  config.latitude = "31.2304";
  config.longitude = "181.0";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::InvalidLongitude, result.error);
}

void test_empty_latitude_longitude_allowed() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.wifiSsid = "Home";
  config.ntpServer = "pool.ntp.org";
  config.latitude = "";
  config.longitude = "";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::None, result.error);
}

void test_malformed_latitude_rejected() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.wifiSsid = "Home";
  config.ntpServer = "pool.ntp.org";
  config.latitude = "not-a-number";
  config.longitude = "121.4737";
  homedeck::ManualDateTime manual{true, 2026, 5, 24, 12, 0, 0};

  const auto result = homedeck::validateSetupSubmission(config, manual);

  TEST_ASSERT_EQUAL(homedeck::ConfigValidationError::InvalidLatitude, result.error);
}
```

在 `main()` 中添加：
```cpp
  RUN_TEST(test_valid_latitude_longitude);
  RUN_TEST(test_invalid_latitude_rejected);
  RUN_TEST(test_invalid_longitude_rejected);
  RUN_TEST(test_empty_latitude_longitude_allowed);
  RUN_TEST(test_malformed_latitude_rejected);
```

- [ ] **Step 5: 运行测试确认失败**

Run: `pio test -e native -f test_config_validation`

Expected: 编译失败或测试失败，因为 `parseLatitude`/`parseLongitude` 和 `InvalidLatitude`/`InvalidLongitude` 未定义。

- [ ] **Step 6: 在 config_validator.cpp 中实现坐标解析函数**

在匿名命名空间中的 `makeError` 函数之后、`parseManualDateTime` 之前，添加：

```cpp
bool parseLatitude(std::string_view value, double* out) {
  if (value.empty()) return true;
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

在 `validateSetupSubmission()` 的 `if (hasWifi && !config.autoRtcCorrection && !manualDateTime.present)` 块之后、`return ConfigValidationResult{};` 之前，添加：

```cpp
  if (!parseLatitude(config.latitude, nullptr)) {
    return makeError(ConfigValidationError::InvalidLatitude, "纬度格式无效（范围：-90 ~ 90）。");
  }
  if (!parseLongitude(config.longitude, nullptr)) {
    return makeError(ConfigValidationError::InvalidLongitude, "经度格式无效（范围：-180 ~ 180）。");
  }
```

- [ ] **Step 7: 运行测试确认通过**

Run: `pio test -e native -f test_config_validation`

Expected: 所有 18 个测试通过。

- [ ] **Step 8: 提交**

```bash
git add src/config/config_types.h src/config/config_validator.h src/config/config_validator.cpp test/native/test_config_validation/test_main.cpp
git commit -m "feat: add latitude/longitude to SetupConfig with validation

- Add latitude/longitude fields to SetupConfig
- Add InvalidLatitude/InvalidLongitude validation errors
- Implement parseLatitude/parseLongitude with range checking
- Add unit tests for valid, invalid, empty, and malformed coordinates"
```

---

## Task 2: 持久化存储

**Files:**
- Modify: `src/config/config_store.cpp`
- Test: `test/native/test_config_store/test_main.cpp`

---

- [ ] **Step 1: 在 config_store.cpp 中添加 NVS key 常量**

在匿名命名空间中的 `kForceConfig` 之后添加：

```cpp
constexpr const char* kLatitude = "lat";
constexpr const char* kLongitude = "lon";
```

- [ ] **Step 2: 在 loadSetupConfig 中添加坐标读取**

在 `config.ntpServer = ...` 之后、`return config;` 之前添加：

```cpp
  config.latitude = prefs_.getString(kLatitude, "31.2304").c_str();
  config.longitude = prefs_.getString(kLongitude, "121.4737").c_str();
```

- [ ] **Step 3: 在 saveSetupConfig 中添加坐标写入**

在 `const bool boolOk = ...` 之后、`return ...` 之前添加：

```cpp
  const bool latOk = prefs_.putString(kLatitude, config.latitude.c_str()) > 0 || config.latitude.empty();
  const bool lonOk = prefs_.putString(kLongitude, config.longitude.c_str()) > 0 || config.longitude.empty();
```

并将 return 语句改为：

```cpp
  return stringsOk && passwordOk && timezoneOk && ntpOk && boolOk && latOk && lonOk;
```

- [ ] **Step 4: 更新 test_config_store 测试 —— 验证默认坐标**

修改 `test_load_defaults_when_empty`：

在 `TEST_ASSERT_EQUAL_STRING("pool.ntp.org", config.ntpServer.c_str());` 之后添加：

```cpp
  TEST_ASSERT_EQUAL_STRING("31.2304", config.latitude.c_str());
  TEST_ASSERT_EQUAL_STRING("121.4737", config.longitude.c_str());
```

- [ ] **Step 5: 更新 test_config_store 测试 —— 验证坐标保存读取**

修改 `test_save_and_load_config_and_flags`：

在 `config.ntpServer = "time.cloudflare.com";` 之后添加：

```cpp
  config.latitude = "39.9042";
  config.longitude = "116.4074";
```

在 `TEST_ASSERT_EQUAL_STRING("time.cloudflare.com", loaded.ntpServer.c_str());` 之后添加：

```cpp
  TEST_ASSERT_EQUAL_STRING("39.9042", loaded.latitude.c_str());
  TEST_ASSERT_EQUAL_STRING("116.4074", loaded.longitude.c_str());
```

- [ ] **Step 6: 运行测试确认通过**

Run: `pio test -e native -f test_config_store`

Expected: 所有 3 个测试通过。

- [ ] **Step 7: 提交**

```bash
git add src/config/config_store.cpp test/native/test_config_store/test_main.cpp
git commit -m "feat: persist latitude/longitude to NVS

- Add 'lat' and 'lon' NVS keys
- Default coordinates: Shanghai Huangpu (31.2304, 121.4737)
- Update store tests for default and round-trip coordinate persistence"
```

---

## Task 3: 前端配置页面

**Files:**
- Modify: `src/config/setup_page.cpp`
- Test: `test/native/test_setup_page/test_main.cpp`

---

- [ ] **Step 1: 在 setup_page.cpp 的 style 标签中添加位置状态样式**

将：
```cpp
  html << "<style>body{font-family:sans-serif;margin:24px;max-width:680px}label{display:block;margin-top:14px}input,select,button{font-size:16px;padding:8px;width:100%;box-sizing:border-box}.wifi button{margin:4px 0}.msg{color:#b00020}</style>";
```

替换为：
```cpp
  html << "<style>body{font-family:sans-serif;margin:24px;max-width:680px}label{display:block;margin-top:14px}input,select,button{font-size:16px;padding:8px;width:100%;box-sizing:border-box}.wifi button{margin:4px 0}.msg{color:#b00020}#location_status{display:block;margin-top:4px;font-size:14px}#location_status.ok{color:#2e7d32}#location_status.err{color:#b00020}</style>";
```

- [ ] **Step 2: 在 setup_page.cpp 的表单中添加位置输入框和按钮**

将：
```cpp
  html << "<label>手动日期时间<input name=\"manual_datetime\" type=\"datetime-local\"></label>";
  html << "<button type=\"submit\">保存</button></form>";
```

替换为：
```cpp
  html << "<label>手动日期时间<input name=\"manual_datetime\" type=\"datetime-local\"></label>";
  html << "<label>纬度 <input id=\"latitude\" name=\"latitude\" value=\"" << htmlEscape(values.latitude) << "\"></label>";
  html << "<label>经度 <input id=\"longitude\" name=\"longitude\" value=\"" << htmlEscape(values.longitude) << "\"></label>";
  html << "<button type=\"button\" id=\"get_location\">获取当前位置</button><span id=\"location_status\"></span>";
  html << "<button type=\"submit\">保存</button></form>";
```

- [ ] **Step 3: 在 setup_page.cpp 的 script 标签中添加 Geolocation JS**

将：
```cpp
  html << "<script>const ssid=document.getElementById('wifi_ssid');const auto=document.getElementById('auto_rtc');function sync(){auto.disabled=ssid.value.trim()==='';if(auto.disabled)auto.checked=false;}function pickSsid(v){ssid.value=v;sync();}document.querySelectorAll('.wifi button[data-ssid]').forEach(b=>b.addEventListener('click',()=>pickSsid(b.dataset.ssid)));ssid.addEventListener('input',sync);sync();</script>";
```

替换为：
```cpp
  html << "<script>const ssid=document.getElementById('wifi_ssid');const auto=document.getElementById('auto_rtc');function sync(){auto.disabled=ssid.value.trim()==='';if(auto.disabled)auto.checked=false;}function pickSsid(v){ssid.value=v;sync();}document.querySelectorAll('.wifi button[data-ssid]').forEach(b=>b.addEventListener('click',()=>pickSsid(b.dataset.ssid)));ssid.addEventListener('input',sync);sync();const latInput=document.getElementById('latitude');const lonInput=document.getElementById('longitude');const locStatus=document.getElementById('location_status');document.getElementById('get_location').addEventListener('click',function(){if(!navigator.geolocation){locStatus.textContent='浏览器不支持地理定位';locStatus.className='err';return;}locStatus.textContent='正在获取位置...';locStatus.className='';navigator.geolocation.getCurrentPosition(function(pos){latInput.value=pos.coords.latitude.toFixed(6);lonInput.value=pos.coords.longitude.toFixed(6);locStatus.textContent='位置已获取';locStatus.className='ok';},function(err){var msgs={1:'权限被拒绝',2:'位置不可用',3:'获取超时'};locStatus.textContent=msgs[err.code]||'获取失败';locStatus.className='err';},{timeout:10000,enableHighAccuracy:false});});</script>";
```

- [ ] **Step 4: 更新 test_setup_page 测试 —— 验证位置字段存在**

在 `test_setup_page_does_not_embed_ssid_in_inline_javascript` 之后添加：

```cpp
void test_setup_page_contains_location_fields() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.ntpServer = "pool.ntp.org";
  config.latitude = "31.2304";
  config.longitude = "121.4737";
  std::vector<homedeck::WifiNetwork> networks{};

  const std::string html = homedeck::buildSetupPageHtml("HomeDeck-ABCD", config, networks, "");

  TEST_ASSERT_NOT_EQUAL(-1, html.find("name=\"latitude\""));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("name=\"longitude\""));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("id=\"get_location\""));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("navigator.geolocation"));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("31.2304"));
  TEST_ASSERT_NOT_EQUAL(-1, html.find("121.4737"));
}
```

在 `main()` 的 `RUN_TEST` 列表末尾添加：

```cpp
  RUN_TEST(test_setup_page_contains_location_fields);
```

- [ ] **Step 5: 运行测试确认通过**

Run: `pio test -e native -f test_setup_page`

Expected: 所有 5 个测试通过。

- [ ] **Step 6: 提交**

```bash
git add src/config/setup_page.cpp test/native/test_setup_page/test_main.cpp
git commit -m "feat: add location acquisition UI to setup page

- Add latitude/longitude input fields with Shanghai default values
- Add '获取当前位置' button using browser Geolocation API
- Add inline JS for position acquisition with error handling
- Add CSS styles for location status messages
- Add unit test verifying location fields in generated HTML"
```

---

## Task 4: 后端请求处理

**Files:**
- Modify: `src/config/config_portal.cpp`

---

- [ ] **Step 1: 在 config_portal.cpp 的 readConfigFromRequest 中读取坐标**

将：
```cpp
  config.ntpServer = server_.arg("ntp_server").c_str();
  return config;
```

替换为：
```cpp
  config.ntpServer = server_.arg("ntp_server").c_str();
  config.latitude = server_.arg("latitude").c_str();
  config.longitude = server_.arg("longitude").c_str();
  return config;
```

- [ ] **Step 2: 编译验证**

Run: `pio run -e m5stack-papercolor`

Expected: 编译成功，无错误无警告。

- [ ] **Step 3: 提交**

```bash
git add src/config/config_portal.cpp
git commit -m "feat: read latitude/longitude from form submission

- Extend readConfigFromRequest to extract latitude and longitude fields
- Coordinates flow through validation and save pipeline unchanged"
```

---

## Task 5: 集成验证

---

- [ ] **Step 1: 运行所有本机单元测试**

Run: `pio test -e native`

Expected: 所有测试套件通过。

- [ ] **Step 2: 运行目标固件编译**

Run: `pio run -e m5stack-papercolor`

Expected: 编译成功，无错误无警告。

- [ ] **Step 3: 最终提交**

```bash
git commit --allow-empty -m "chore: verify location config feature passes all tests and builds

- All native unit tests pass
- m5stack-papercolor firmware builds successfully"
```

---

## Self-Review

### 1. Spec Coverage

| 设计文档要求 | 对应任务 |
|-------------|---------|
| SetupConfig 新增 latitude/longitude | Task 1, Step 1 |
| ConfigValidationError 新增 InvalidLatitude/InvalidLongitude | Task 1, Step 2 |
| parseLatitude/parseLongitude 实现 | Task 1, Step 6 |
| validateSetupSubmission 调用坐标验证 | Task 1, Step 6 |
| NVS 读写坐标（lat/lon key，默认上海坐标） | Task 2, Steps 1-3 |
| 前端 HTML 输入框、按钮、JS、CSS | Task 3, Steps 1-3 |
| readConfigFromRequest 读取坐标 | Task 4, Step 1 |
| 坐标验证测试（有效/无效/空值/格式错误） | Task 1, Step 4 |
| Store 默认坐标和保存读取测试 | Task 2, Steps 4-5 |
| Setup page 元素存在性测试 | Task 3, Step 4 |

**无遗漏。**

### 2. Placeholder Scan

- 无 "TBD" / "TODO" / "implement later" / "fill in details"
- 无 "add appropriate error handling" 等模糊描述
- 无 "similar to Task N" 引用
- 每个代码步骤都有完整代码块
- 每个运行步骤都有具体命令和预期输出

### 3. Type Consistency

- `latitude`/`longitude` 始终为 `std::string`（SetupConfig、表单参数、NVS 存储）
- `parseLatitude`/`parseLongitude` 的签名在声明和实现中一致
- `ConfigValidationError::InvalidLatitude`/`InvalidLongitude` 在 enum、测试、实现中命名一致
- 默认坐标值 "31.2304"/"121.4737" 在 NVS、测试、设计文档中一致

---

## Execution Handoff

**Plan complete and saved to `docs/superpowers/plans/2026-06-02-location-config.md`.**

**Two execution options:**

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**
