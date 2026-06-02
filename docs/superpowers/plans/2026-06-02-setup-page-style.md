# HomeDeck 配置网页样式现代化重构实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 重构 HomeDeck 在配置模式下的 Web 页面样式，将其提升为响应式、采用 CSS Tokens 驱动的精致温和明亮风（Warm & Clean Light Mode），在不依赖外部 CDN 的前提下实现高水平的可用性与微交互视效，并确保 Native 单元测试 100% 通过。

**Architecture:** 
1. 在 `src/config/setup_page.cpp` 的 `buildSetupPageHtml` 函数中，对 HTML 进行语义化重构，引入 `.container` 容器与三张功能性 `.card`（头部卡片、网络扫描卡片、主配置卡片）。
2. 在内联 `<style>` 开头声明基于 `:root` 的 CSS 变量（CSS Tokens），并完全用变量替代硬编码的样式值。
3. 使用轻量级 CSS 动画（如 `highlight-flash`）以及原生 CSS 信号格图标，提升可用性。
4. 增强内联 JavaScript，实现 Wi-Fi 选中的 `.active` 状态变化、OSM 提取成功的“呼吸闪烁特效”、RTC 状态联动以及提交按钮防抖和前端基本非空校验。

**Tech Stack:** C++ std::ostringstream, Modern self-contained Vanilla HTML5 / CSS3 / JavaScript (ES6+).

---

### Task 1: 准备静态预览与环境验证 (Setup Static Preview & Verification)

本任务旨在建立一个独立预览沙箱，以便在不烧录硬件的情况下，直接在主机浏览器中通过拉伸视口验证最新的 CSS 响应式与 CSS Tokens 的渲染表现。

**Files:**
- Create: `/Users/kenn/PROJECTS/m5stack/HomeDeck/scratch/preview.html`

- [ ] **Step 1: 创建本地静态预览 HTML 文件**
  
  在 scratch 目录下编写一个完全自包含的静态测试页面，模拟编译后渲染出的最终 DOM。包含完整的 CSS Tokens、响应式样式、测试用的 Wi-Fi 药丸按钮、提示框（Callout）以及增强的交互 JavaScript 脚本。

  写入以下完整代码至 `/Users/kenn/PROJECTS/m5stack/HomeDeck/scratch/preview.html`：
  ```html
  <!doctype html>
  <html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>HomeDeck Setup Preview</title>
    <style>
      :root {
        /* Colors */
        --primary-color: #4F46E5;
        --primary-hover: #4338CA;
        --primary-light: #EEF2FF;
        --bg-page: #F4F5F7;
        --bg-card: #FFFFFF;
        --bg-input: #F9FAFB;
        --bg-input-focus: #FFFFFF;
        --color-success: #10B981;
        --color-success-bg: #ECFDF5;
        --color-error: #EF4444;
        --color-error-bg: #FEF2F2;
        --color-error-border: #FCA5A5;
        --color-info: #3B82F6;
        --color-info-bg: #EFF6FF;
        --color-info-border: #BFDBFE;
        --text-primary: #1F2937;
        --text-secondary: #4B5563;
        --text-tertiary: #9CA3AF;
        --text-on-primary: #FFFFFF;
        /* Radius & Spacing */
        --radius-card: 16px;
        --radius-input: 8px;
        --radius-badge: 6px;
        --border-color: #E5E7EB;
        --border-color-focus: #4F46E5;
        --border-width: 1px;
        --space-xs: 4px;
        --space-sm: 8px;
        --space-md: 16px;
        --space-lg: 24px;
        --space-xl: 32px;
        /* Shadows & Transitions */
        --shadow-card: 0 4px 6px -1px rgba(0, 0, 0, 0.03), 0 2px 4px -1px rgba(0, 0, 0, 0.02), 0 16px 24px -4px rgba(0, 0, 0, 0.04);
        --shadow-focus: 0 0 0 3px rgba(79, 70, 229, 0.15);
        --transition-fast: 0.15s ease;
      }

      * { box-sizing: border-box; margin: 0; padding: 0; }
      body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
        background-color: var(--bg-page);
        color: var(--text-primary);
        padding: var(--space-xl) var(--space-md);
        display: flex;
        justify-content: center;
        min-height: 100vh;
      }
      .container {
        width: 100%;
        max-width: 600px;
        display: flex;
        flex-direction: column;
        gap: var(--space-md);
      }
      .card {
        background-color: var(--bg-card);
        border-radius: var(--radius-card);
        padding: var(--space-lg);
        box-shadow: var(--shadow-card);
        border: var(--border-width) solid var(--border-color);
      }
      .header-card {
        display: flex;
        flex-direction: column;
        gap: var(--space-sm);
      }
      .logo-area {
        display: flex;
        align-items: center;
        gap: var(--space-sm);
      }
      .logo-icon {
        width: 32px;
        height: 32px;
        color: var(--primary-color);
      }
      h1 {
        font-size: 24px;
        font-weight: 700;
        color: var(--text-primary);
        letter-spacing: -0.5px;
      }
      .ap-status {
        display: flex;
        align-items: center;
        gap: var(--space-sm);
        flex-wrap: wrap;
        margin-top: var(--space-xs);
      }
      .badge {
        font-size: 12px;
        font-weight: 600;
        padding: 3px 8px;
        border-radius: var(--radius-badge);
        text-transform: uppercase;
      }
      .badge-success {
        background-color: var(--color-success-bg);
        color: var(--color-success);
        border: 1px solid rgba(16, 185, 129, 0.2);
      }
      .ap-info {
        font-size: 14px;
        color: var(--text-secondary);
      }
      .callout {
        padding: var(--space-md);
        border-radius: var(--radius-input);
        display: flex;
        gap: var(--space-sm);
        align-items: flex-start;
      }
      .callout-error {
        background-color: var(--color-error-bg);
        border: var(--border-width) solid var(--color-error-border);
        color: var(--color-error);
      }
      .callout-info {
        background-color: var(--color-info-bg);
        border: var(--border-width) solid var(--color-info-border);
        color: var(--color-info);
      }
      .callout-icon {
        flex-shrink: 0;
        margin-top: 2px;
      }
      .callout p {
        font-size: 13px;
        line-height: 1.5;
      }
      .callout-info p {
        color: var(--text-secondary);
      }
      .callout-error p {
        font-weight: 500;
      }
      h2 {
        font-size: 16px;
        font-weight: 600;
        margin-bottom: var(--space-md);
        display: flex;
        align-items: center;
        gap: var(--space-sm);
        color: var(--text-primary);
      }
      .section-icon {
        width: 18px;
        height: 18px;
        color: var(--text-secondary);
      }
      .wifi-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(130px, 1fr));
        gap: var(--space-sm);
      }
      .wifi-pill {
        border: var(--border-width) solid var(--border-color);
        background-color: var(--bg-card);
        border-radius: var(--radius-badge);
        padding: var(--space-sm) var(--space-md);
        font-size: 13px;
        font-weight: 500;
        cursor: pointer;
        display: flex;
        flex-direction: column;
        align-items: flex-start;
        gap: 2px;
        text-align: left;
        transition: all var(--transition-fast);
        color: var(--text-secondary);
      }
      .wifi-pill:hover {
        border-color: var(--primary-color);
        background-color: var(--primary-light);
        color: var(--primary-color);
      }
      .wifi-pill.active {
        border-color: var(--primary-color);
        background-color: var(--primary-light);
        color: var(--primary-color);
        box-shadow: var(--shadow-focus);
      }
      .wifi-meta {
        font-size: 11px;
        color: var(--text-tertiary);
        display: flex;
        align-items: center;
        gap: 4px;
      }
      .wifi-pill:hover .wifi-meta, .wifi-pill.active .wifi-meta {
        color: var(--primary-color);
      }
      /* Custom pure CSS Signal Icon */
      .sig-icon {
        display: inline-flex;
        align-items: flex-end;
        gap: 2px;
        width: 12px;
        height: 8px;
      }
      .sig-bar {
        width: 2px;
        background-color: #D1D5DB;
        border-radius: 1px;
      }
      .sig-bar-1 { height: 30%; }
      .sig-bar-2 { height: 65%; }
      .sig-bar-3 { height: 100%; }
      .wifi-pill:hover .sig-bar, .wifi-pill.active .sig-bar {
        background-color: var(--primary-color);
      }
      .wifi-pill[data-sig="good"] .sig-bar-1,
      .wifi-pill[data-sig="good"] .sig-bar-2,
      .wifi-pill[data-sig="good"] .sig-bar-3 {
        background-color: var(--color-success);
      }
      .wifi-pill[data-sig="mid"] .sig-bar-1,
      .wifi-pill[data-sig="mid"] .sig-bar-2 {
        background-color: #F59E0B;
      }
      .wifi-pill[data-sig="weak"] .sig-bar-1 {
        background-color: var(--color-error);
      }
      form {
        display: flex;
        flex-direction: column;
        gap: var(--space-md);
      }
      .form-group {
        margin-bottom: var(--space-md);
        display: flex;
        flex-direction: column;
        gap: var(--space-xs);
      }
      .form-group:last-child {
        margin-bottom: 0;
      }
      label {
        font-size: 13px;
        font-weight: 600;
        color: var(--text-secondary);
        display: flex;
        align-items: center;
      }
      .label-checkbox {
        flex-direction: row;
        gap: var(--space-sm);
        cursor: pointer;
        padding: var(--space-sm) 0;
        font-weight: 500;
      }
      input[type="text"], input[type="password"], input[type="datetime-local"], select {
        width: 100%;
        padding: 10px 12px;
        font-size: 14px;
        border: var(--border-width) solid var(--border-color);
        background-color: var(--bg-input);
        border-radius: var(--radius-input);
        color: var(--text-primary);
        outline: none;
        transition: all var(--transition-fast);
      }
      input[type="checkbox"] {
        width: 16px;
        height: 16px;
        accent-color: var(--primary-color);
        cursor: pointer;
      }
      input[type="checkbox"]:disabled + span {
        color: var(--text-tertiary);
        cursor: not-allowed;
      }
      input:focus, select:focus {
        border-color: var(--border-color-focus);
        background-color: var(--bg-input-focus);
        box-shadow: var(--shadow-focus);
      }
      input:disabled, select:disabled {
        background-color: #E5E7EB;
        color: var(--text-tertiary);
        cursor: not-allowed;
      }
      .btn-submit {
        background-color: var(--primary-color);
        color: var(--text-on-primary);
        font-size: 15px;
        font-weight: 600;
        padding: var(--space-md);
        border: none;
        border-radius: var(--radius-input);
        cursor: pointer;
        transition: all var(--transition-fast);
        box-shadow: 0 4px 6px -1px rgba(79, 70, 229, 0.2);
        width: 100%;
      }
      .btn-submit:hover:not(:disabled) {
        background-color: var(--primary-hover);
        box-shadow: 0 6px 12px -2px rgba(79, 70, 229, 0.3);
      }
      .btn-submit:active:not(:disabled) {
        transform: scale(0.98);
      }
      .btn-submit:disabled {
        background-color: var(--text-tertiary);
        cursor: not-allowed;
        box-shadow: none;
      }

      /* OSM Flash Animation */
      @keyframes flash-green {
        0% { background-color: var(--color-success-bg); border-color: var(--color-success); }
        100% { background-color: var(--bg-input-focus); border-color: var(--border-color-focus); }
      }
      .highlight-flash {
        animation: flash-green 0.8s ease-out;
      }

      /* Responsive Rules */
      @media (max-width: 640px) {
        body { padding: var(--space-md) var(--space-sm); }
        .card { padding: var(--space-md); border-radius: 12px; }
        h1 { font-size: 20px; }
        .wifi-grid { grid-template-columns: repeat(auto-fill, minmax(110px, 1fr)); }
        .wifi-pill { padding: var(--space-sm); font-size: 12px; }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <header class="card header-card">
        <div class="logo-area">
          <svg class="logo-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" />
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
          </svg>
          <h1>HomeDeck Setup</h1>
        </div>
        <div class="ap-status">
          <span class="badge badge-success">AP 运行中</span>
          <span class="ap-info">热点: <strong>HomeDeck-ABCD</strong> / 192.168.4.1</span>
        </div>
      </header>

      <div id="error_container" class="callout callout-error" style="display:none;">
        <svg class="callout-icon" width="16" height="16" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
        </svg>
        <p id="error_msg" class="msg"></p>
      </div>

      <section class="card">
        <h2>
          <svg class="section-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071a9 9 0 0112.14 0M1.93 9.143a13 13 0 0118.14 0" />
          </svg>
          可用 Wi-Fi 列表
        </h2>
        <div class="wifi-grid">
          <button type="button" class="wifi-pill" data-ssid="SmartHome_5G" data-sig="good">
            <span>SmartHome_5G</span>
            <div class="wifi-meta">
              <div class="sig-icon" data-sig="good"><span class="sig-bar sig-bar-1"></span><span class="sig-bar sig-bar-2"></span><span class="sig-bar sig-bar-3"></span></div>
              -45 dBm
            </div>
          </button>
          <button type="button" class="wifi-pill" data-ssid="Office-Guest" data-sig="mid">
            <span>Office-Guest</span>
            <div class="wifi-meta">
              <div class="sig-icon" data-sig="mid"><span class="sig-bar sig-bar-1"></span><span class="sig-bar sig-bar-2"></span><span class="sig-bar sig-bar-3"></span></div>
              -68 dBm
            </div>
          </button>
          <button type="button" class="wifi-pill" data-ssid="TP-LINK_Old" data-sig="weak">
            <span>TP-LINK_Old</span>
            <div class="wifi-meta">
              <div class="sig-icon" data-sig="weak"><span class="sig-bar sig-bar-1"></span><span class="sig-bar sig-bar-2"></span><span class="sig-bar sig-bar-3"></span></div>
              -82 dBm
            </div>
          </button>
        </div>
      </section>

      <form id="setup_form" method="post" action="/save">
        <section class="card">
          <h2>
            <svg class="section-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
            </svg>
            网络与系统配置
          </h2>
          <div class="form-group">
            <label for="wifi_ssid">Wi-Fi SSID</label>
            <input id="wifi_ssid" name="wifi_ssid" value="" placeholder="请输入 SSID 或从上方选择">
          </div>
          <div class="form-group">
            <label for="wifi_password">Wi-Fi 密码</label>
            <input id="wifi_password" name="wifi_password" type="password" value="" placeholder="请输入无线密码">
          </div>
          <div class="form-group">
            <label for="timezone">时区</label>
            <select id="timezone" name="timezone">
              <option value="Asia/Shanghai">Asia/Shanghai (上海)</option>
              <option value="Asia/Tokyo">Asia/Tokyo (东京)</option>
              <option value="America/New_York">America/New_York (纽约)</option>
            </select>
          </div>
          <div class="form-group">
            <label class="label-checkbox">
              <input id="auto_rtc" name="auto_rtc" type="checkbox" value="1" disabled>
              <span>自动纠正 RTC</span>
            </label>
          </div>
          <div class="form-group">
            <label for="ntp_server">NTP 服务器</label>
            <input id="ntp_server" name="ntp_server" value="ntp.aliyun.com">
          </div>
          <div class="form-group">
            <label for="manual_datetime">手动日期时间</label>
            <input id="manual_datetime" name="manual_datetime" type="datetime-local">
          </div>
        </section>

        <section class="card">
          <h2>
            <svg class="section-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z" />
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 11a3 3 0 11-6 0 3 3 0 016 0z" />
            </svg>
            地理位置坐标
          </h2>
          <div class="form-group">
            <label for="latitude">纬度</label>
            <input id="latitude" name="latitude" value="" placeholder="例如: 31.2304">
          </div>
          <div class="form-group">
            <label for="longitude">经度</label>
            <input id="longitude" name="longitude" value="" placeholder="例如: 121.4737">
          </div>
          <div class="form-group">
            <label for="osm_link">OpenStreetMap 地图链接</label>
            <input id="osm_link" placeholder="在此粘贴 OpenStreetMap 链接以提取坐标">
          </div>
          <div class="callout callout-info hint">
            <svg class="callout-icon" width="16" height="16" fill="none" viewBox="0 0 24 24" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
            <p>打开 openstreetmap.org，定位当前位置并复制浏览器地址栏的链接粘贴至上方，可自动提取经纬度。</p>
          </div>
        </section>

        <button type="submit" id="submit_btn" class="btn-submit">保存配置</button>
      </form>
    </div>

    <script>
      const ssidInput = document.getElementById('wifi_ssid');
      const autoRtc = document.getElementById('auto_rtc');
      const errorContainer = document.getElementById('error_container');
      const errorMsg = document.getElementById('error_msg');
      const form = document.getElementById('setup_form');
      const submitBtn = document.getElementById('submit_btn');

      function syncRtcState() {
        autoRtc.disabled = ssidInput.value.trim() === '';
        if (autoRtc.disabled) {
          autoRtc.checked = false;
        }
      }

      function selectWifi(pill, ssidValue) {
        document.querySelectorAll('.wifi-pill').forEach(p => p.classList.remove('active'));
        pill.classList.add('active');
        ssidInput.value = ssidValue;
        syncRtcState();
      }

      document.querySelectorAll('.wifi-pill').forEach(pill => {
        pill.addEventListener('click', () => selectWifi(pill, pill.dataset.ssid));
      });

      ssidInput.addEventListener('input', () => {
        syncRtcState();
        // 如果输入框手动输入的值不匹配任何已选择的药丸，则取消药丸的高亮
        document.querySelectorAll('.wifi-pill').forEach(p => {
          if (p.dataset.ssid === ssidInput.value) {
            p.classList.add('active');
          } else {
            p.classList.remove('active');
          }
        });
      });

      // OSM Link Parsing & Highlight Flash Animation
      document.getElementById('osm_link').addEventListener('input', function() {
        const val = this.value.trim();
        let latVal = '', lonVal = '';
        
        // Match Hash pattern
        const hashMatch = val.match(/#map=[0-9.]+\/([0-9.-]+)\/([0-9.-]+)/);
        if (hashMatch) {
          latVal = hashMatch[1];
          lonVal = hashMatch[2];
        } else {
          // Match Query param pattern
          const latMatch = val.match(/[?&](?:mlat|lat)=([0-9.-]+)/);
          const lonMatch = val.match(/[?&](?:mlon|lon)=([0-9.-]+)/);
          if (latMatch && lonMatch) {
            latVal = latMatch[1];
            lonVal = lonMatch[1]; // Keep standard behavior but allow correct matches
          }
        }

        if (latVal && lonVal) {
          const latInput = document.getElementById('latitude');
          const lonInput = document.getElementById('longitude');
          latInput.value = latVal;
          lonInput.value = lonVal;

          // Add flash animations
          latInput.classList.remove('highlight-flash');
          lonInput.classList.remove('highlight-flash');
          void latInput.offsetWidth; // Trigger reflow to restart animation
          latInput.classList.add('highlight-flash');
          lonInput.classList.add('highlight-flash');
        }
      });

      // Client-side Validation & Defending duplicate submits
      form.addEventListener('submit', function(e) {
        errorContainer.style.display = 'none';
        let errors = [];

        const latVal = document.getElementById('latitude').value.trim();
        const lonVal = document.getElementById('longitude').value.trim();

        if (latVal !== "" && isNaN(Number(latVal))) {
          errors.push("纬度必须为有效的数字。");
        }
        if (lonVal !== "" && isNaN(Number(lonVal))) {
          errors.push("经度必须为有效的数字。");
        }

        if (errors.length > 0) {
          e.preventDefault();
          errorMsg.textContent = errors.join(" ");
          errorContainer.style.display = 'flex';
          window.scrollTo({ top: 0, behavior: 'smooth' });
          return;
        }

        // Disable button to prevent duplicate clicks
        submitBtn.disabled = true;
        submitBtn.textContent = "正在保存...";
      });

      syncRtcState();
    </script>
  </body>
  </html>
  ```

- [ ] **Step 2: 在浏览器中手动预览验证样式**
  
  运行或双击打开 `/Users/kenn/PROJECTS/m5stack/HomeDeck/scratch/preview.html`：
  1. 验证在宽屏和窄屏下的响应式切换，确保无任何元素溢出。
  2. 点击 “Office-Guest” 药丸，验证输入框是否填入 "Office-Guest"，药丸是否高亮，以及“自动纠正 RTC”是否变为可点击状态。
  3. 在 OSM 链接框粘贴 `https://www.openstreetmap.org/#map=19/22.5431/113.9782`，检查经度、纬度输入框是否自动填充，且是否有绿色的闪烁反馈。

---

### Task 2: 重新设计并替换 C++ 端的 HTML 骨架与 CSS Tokens (Implement CSS & HTML Skeleton)

本任务将在固件源代码中集成我们在 Task 1 中完美通过验证的 UI 界面和 CSS Tokens。

**Files:**
- Modify: `/Users/kenn/PROJECTS/m5stack/HomeDeck/src/config/setup_page.cpp:50-83` (修改头部、CSS Tokens、错误消息和 Wi-Fi 部分)
- Modify: `/Users/kenn/PROJECTS/m5stack/HomeDeck/src/config/setup_page.cpp:84-102` (修改表单控件与 DOM 闭合)

- [ ] **Step 1: 重构 `src/config/setup_page.cpp` 的头、CSS Tokens 以及 Wi-Fi 渲染区**
  
  将 `buildSetupPageHtml` 中的 CSS 定义彻底更换为基于 CSS Tokens 驱动的设计体系，重构头部 Banner 卡片以及 Wi-Fi 网格，并在每个药丸标签中使用内联 CSS 渲染精美的信号格。

  将 `src/config/setup_page.cpp` 中对应部分替换为以下内容：
  ```cpp
  std::string buildSetupPageHtml(
      const std::string& apSsid,
      const SetupConfig& values,
      const std::vector<WifiNetwork>& networks,
      const std::string& message) {
    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\">";
    html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html << "<title>HomeDeck Setup</title>";
    html << "<style>";
    html << ":root {";
    html << "  --primary-color: #4F46E5;";
    html << "  --primary-hover: #4338CA;";
    html << "  --primary-light: #EEF2FF;";
    html << "  --bg-page: #F4F5F7;";
    html << "  --bg-card: #FFFFFF;";
    html << "  --bg-input: #F9FAFB;";
    html << "  --bg-input-focus: #FFFFFF;";
    html << "  --color-success: #10B981;";
    html << "  --color-success-bg: #ECFDF5;";
    html << "  --color-error: #EF4444;";
    html << "  --color-error-bg: #FEF2F2;";
    html << "  --color-error-border: #FCA5A5;";
    html << "  --color-info: #3B82F6;";
    html << "  --color-info-bg: #EFF6FF;";
    html << "  --color-info-border: #BFDBFE;";
    html << "  --text-primary: #1F2937;";
    html << "  --text-secondary: #4B5563;";
    html << "  --text-tertiary: #9CA3AF;";
    html << "  --text-on-primary: #FFFFFF;";
    html << "  --radius-card: 16px;";
    html << "  --radius-input: 8px;";
    html << "  --radius-badge: 6px;";
    html << "  --border-color: #E5E7EB;";
    html << "  --border-color-focus: #4F46E5;";
    html << "  --border-width: 1px;";
    html << "  --space-xs: 4px;";
    html << "  --space-sm: 8px;";
    html << "  --space-md: 16px;";
    html << "  --space-lg: 24px;";
    html << "  --space-xl: 32px;";
    html << "  --shadow-card: 0 4px 6px -1px rgba(0, 0, 0, 0.03), 0 2px 4px -1px rgba(0, 0, 0, 0.02), 0 16px 24px -4px rgba(0, 0, 0, 0.04);";
    html << "  --shadow-focus: 0 0 0 3px rgba(79, 70, 229, 0.15);";
    html << "  --transition-fast: 0.15s ease;";
    html << "}";
    html << "* { box-sizing: border-box; margin: 0; padding: 0; }";
    html << "body { font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, sans-serif; background-color: var(--bg-page); color: var(--text-primary); padding: var(--space-xl) var(--space-md); display: flex; justify-content: center; min-height: 100vh; }";
    html << ".container { width: 100%; max-width: 600px; display: flex; flex-direction: column; gap: var(--space-md); }";
    html << ".card { background-color: var(--bg-card); border-radius: var(--radius-card); padding: var(--space-lg); box-shadow: var(--shadow-card); border: var(--border-width) solid var(--border-color); }";
    html << ".header-card { display: flex; flex-direction: column; gap: var(--space-sm); }";
    html << ".logo-area { display: flex; align-items: center; gap: var(--space-sm); }";
    html << ".logo-icon { width: 32px; height: 32px; color: var(--primary-color); }";
    html << "h1 { font-size: 24px; font-weight: 700; color: var(--text-primary); letter-spacing: -0.5px; }";
    html << ".ap-status { display: flex; align-items: center; gap: var(--space-sm); flex-wrap: wrap; margin-top: var(--space-xs); }";
    html << ".badge { font-size: 12px; font-weight: 600; padding: 3px 8px; border-radius: var(--radius-badge); text-transform: uppercase; }";
    html << ".badge-success { background-color: var(--color-success-bg); color: var(--color-success); border: 1px solid rgba(16, 185, 129, 0.2); }";
    html << ".ap-info { font-size: 14px; color: var(--text-secondary); }";
    html << ".callout { padding: var(--space-md); border-radius: var(--radius-input); display: flex; gap: var(--space-sm); align-items: flex-start; }";
    html << ".callout-error { background-color: var(--color-error-bg); border: var(--border-width) solid var(--color-error-border); color: var(--color-error); }";
    html << ".callout-info { background-color: var(--color-info-bg); border: var(--border-width) solid var(--color-info-border); color: var(--color-info); }";
    html << ".callout-icon { flex-shrink: 0; margin-top: 2px; }";
    html << ".callout p { font-size: 13px; line-height: 1.5; }";
    html << ".callout-info p { color: var(--text-secondary); }";
    html << ".callout-error p { font-weight: 500; }";
    html << "h2 { font-size: 16px; font-weight: 600; margin-bottom: var(--space-md); display: flex; align-items: center; gap: var(--space-sm); color: var(--text-primary); }";
    html << ".section-icon { width: 18px; height: 18px; color: var(--text-secondary); }";
    html << ".wifi-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(130px, 1fr)); gap: var(--space-sm); }";
    html << ".wifi-pill { border: var(--border-width) solid var(--border-color); background-color: var(--bg-card); border-radius: var(--radius-badge); padding: var(--space-sm) var(--space-md); font-size: 13px; font-weight: 500; cursor: pointer; display: flex; flex-direction: column; align-items: flex-start; gap: 2px; text-align: left; transition: all var(--transition-fast); color: var(--text-secondary); }";
    html << ".wifi-pill:hover { border-color: var(--primary-color); background-color: var(--primary-light); color: var(--primary-color); }";
    html << ".wifi-pill.active { border-color: var(--primary-color); background-color: var(--primary-light); color: var(--primary-color); box-shadow: var(--shadow-focus); }";
    html << ".wifi-meta { font-size: 11px; color: var(--text-tertiary); display: flex; align-items: center; gap: 4px; }";
    html << ".wifi-pill:hover .wifi-meta, .wifi-pill.active .wifi-meta { color: var(--primary-color); }";
    html << ".sig-icon { display: inline-flex; align-items: flex-end; gap: 2px; width: 12px; height: 8px; }";
    html << ".sig-bar { width: 2px; background-color: #D1D5DB; border-radius: 1px; }";
    html << ".sig-bar-1 { height: 30%; }";
    html << ".sig-bar-2 { height: 65%; }";
    html << ".sig-bar-3 { height: 100%; }";
    html << ".wifi-pill:hover .sig-bar, .wifi-pill.active .sig-bar { background-color: var(--primary-color); }";
    html << "form { display: flex; flex-direction: column; gap: var(--space-md); }";
    html << ".form-group { margin-bottom: var(--space-md); display: flex; flex-direction: column; gap: var(--space-xs); }";
    html << "label { font-size: 13px; font-weight: 600; color: var(--text-secondary); display: flex; align-items: center; }";
    html << ".label-checkbox { flex-direction: row; gap: var(--space-sm); cursor: pointer; padding: var(--space-sm) 0; font-weight: 500; }";
    html << "input[type=\"text\"], input[type=\"password\"], input[type=\"datetime-local\"], select { width: 100%; padding: 10px 12px; font-size: 14px; border: var(--border-width) solid var(--border-color); background-color: var(--bg-input); border-radius: var(--radius-input); color: var(--text-primary); outline: none; transition: all var(--transition-fast); }";
    html << "input[type=\"checkbox\"] { width: 16px; height: 16px; accent-color: var(--primary-color); cursor: pointer; }";
    html << "input:focus, select:focus { border-color: var(--border-color-focus); background-color: var(--bg-input-focus); box-shadow: var(--shadow-focus); }";
    html << "input:disabled, select:disabled { background-color: #E5E7EB; color: var(--text-tertiary); cursor: not-allowed; }";
    html << ".btn-submit { background-color: var(--primary-color); color: var(--text-on-primary); font-size: 15px; font-weight: 600; padding: var(--space-md); border: none; border-radius: var(--radius-input); cursor: pointer; transition: all var(--transition-fast); box-shadow: 0 4px 6px -1px rgba(79, 70, 229, 0.2); width: 100%; }";
    html << ".btn-submit:hover:not(:disabled) { background-color: var(--primary-hover); box-shadow: 0 6px 12px -2px rgba(79, 70, 229, 0.3); }";
    html << ".btn-submit:active:not(:disabled) { transform: scale(0.98); }";
    html << ".btn-submit:disabled { background-color: var(--text-tertiary); cursor: not-allowed; box-shadow: none; }";
    html << "@keyframes flash-green { 0% { background-color: var(--color-success-bg); border-color: var(--color-success); } 100% { background-color: var(--bg-input-focus); border-color: var(--border-color-focus); } }";
    html << ".highlight-flash { animation: flash-green 0.8s ease-out; }";
    html << "@media (max-width: 640px) {";
    html << "  body { padding: var(--space-md) var(--space-sm); }";
    html << "  .card { padding: var(--space-md); border-radius: 12px; }";
    html << "  h1 { font-size: 20px; }";
    html << "  .wifi-grid { grid-template-columns: repeat(auto-fill, minmax(110px, 1fr)); }";
    html << "  .wifi-pill { padding: var(--space-sm); font-size: 12px; }";
    html << "}";
    html << "</style></head><body>";
    html << "<div class=\"container\">";
    html << "<header class=\"card header-card\">";
    html << "<div class=\"logo-area\">";
    // Inline SVG gear/gear icon
    html << "<svg class=\"logo-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z\" /><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M15 12a3 3 0 11-6 0 3 3 0 016 0z\" /></svg>";
    html << "<h1>HomeDeck Setup</h1>";
    html << "</div>";
    html << "<div class=\"ap-status\">";
    html << "<span class=\"badge badge-success\">AP 运行中</span>";
    html << "<span class=\"ap-info\">热点: <strong>" << htmlEscape(apSsid) << "</strong> / 192.168.4.1</span>";
    html << "</div>";
    html << "</header>";

    // Callout message
    if (!message.empty()) {
      html << "<div id=\"error_container\" class=\"callout callout-error\">";
      html << "<svg class=\"callout-icon\" width=\"16\" height=\"16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z\" /></svg>";
      html << "<p class=\"msg\">" << htmlEscape(message) << "</p>";
      html << "</div>";
    } else {
      html << "<div id=\"error_container\" class=\"callout callout-error\" style=\"display:none;\">";
      html << "<svg class=\"callout-icon\" width=\"16\" height=\"16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z\" /></svg>";
      html << "<p id=\"error_msg\" class=\"msg\"></p>";
      html << "</div>";
    }

    // WiFi List Card
    html << "<section class=\"card\">";
    html << "<h2>";
    html << "<svg class=\"section-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071a9 9 0 0112.14 0M1.93 9.143a13 13 0 0118.14 0\" /></svg>";
    html << "Wi-Fi 列表</h2>";
    html << "<div class=\"wifi-grid\">";
    for (const auto& network : networks) {
      std::string signalQuality = "good";
      if (network.rssi < -80) signalQuality = "weak";
      else if (network.rssi < -65) signalQuality = "mid";

      html << "<button type=\"button\" class=\"wifi-pill\" data-ssid=\"" << htmlEscape(network.ssid) << "\" data-sig=\"" << signalQuality << "\">";
      html << "<span>" << htmlEscape(network.ssid) << "</span>";
      html << "<div class=\"wifi-meta\">";
      // Pure CSS Signal Icons
      html << "<div class=\"sig-icon\"><span class=\"sig-bar sig-bar-1\"></span><span class=\"sig-bar sig-bar-2\"></span><span class=\"sig-bar sig-bar-3\"></span></div>";
      html << network.rssi << " dBm</div></button>";
    }
    html << "</div></section>";
  ```

- [ ] **Step 2: 重构 `src/config/setup_page.cpp` 的表单卡片与 DOM 闭合**
  
  将后续的 input 项按照 `.form-group` 包裹起来，并以两个独立的 `.card`（一个为主配置，一个为地理位置）进行模块化包装。

  将 `src/config/setup_page.cpp` 中对应部分替换为以下内容：
  ```cpp
    html << "<form id=\"setup_form\" method=\"post\" action=\"/save\">";
    
    // Card 2: Network Configuration
    html << "<section class=\"card\">";
    html << "<h2><svg class=\"section-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4\" /></svg>";
    html << "网络与系统配置</h2>";
    
    html << "<div class=\"form-group\"><label for=\"wifi_ssid\">Wi-Fi SSID</label>";
    html << "<input id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"" << htmlEscape(values.wifiSsid) << "\" placeholder=\"请输入 SSID 或从上方选择\"></div>";
    
    html << "<div class=\"form-group\"><label for=\"wifi_password\">Wi-Fi 密码</label>";
    html << "<input id=\"wifi_password\" name=\"wifi_password\" type=\"password\" value=\"" << htmlEscape(values.wifiPassword) << "\" placeholder=\"请输入密码\"></div>";
    
    html << "<div class=\"form-group\"><label for=\"timezone\">时区</label><select id=\"timezone\" name=\"timezone\">";
    std::size_t timezoneCount = 0;
    const auto* timezones = timezoneCatalog(&timezoneCount);
    for (std::size_t i = 0; i < timezoneCount; ++i) {
      html << "<option value=\"" << timezones[i].iana << "\"";
      if (values.timezoneIana == timezones[i].iana) {
        html << " selected";
      }
      html << ">" << timezones[i].label << "</option>";
    }
    html << "</select></div>";
    
    html << "<div class=\"form-group\"><label class=\"label-checkbox\">";
    html << "<input id=\"auto_rtc\" name=\"auto_rtc\" type=\"checkbox\" value=\"1\"";
    if (values.autoRtcCorrection) {
      html << " checked";
    }
    if (values.wifiSsid.empty()) {
      html << " disabled";
    }
    html << "> <span>自动纠正 RTC</span></label></div>";
    
    html << "<div class=\"form-group\"><label for=\"ntp_server\">NTP 服务器</label>";
    html << "<input id=\"ntp_server\" name=\"ntp_server\" value=\"" << htmlEscape(values.ntpServer) << "\"></div>";
    
    html << "<div class=\"form-group\"><label for=\"manual_datetime\">手动日期时间</label>";
    html << "<input id=\"manual_datetime\" name=\"manual_datetime\" type=\"datetime-local\"></div>";
    html << "</section>";

    // Card 3: Location Configuration
    html << "<section class=\"card\">";
    html << "<h2><svg class=\"section-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z\" /><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M15 11a3 3 0 11-6 0 3 3 0 016 0z\" /></svg>";
    html << "地理位置坐标</h2>";
    
    html << "<div class=\"form-group\"><label for=\"latitude\">纬度</label>";
    html << "<input id=\"latitude\" name=\"latitude\" value=\"" << htmlEscape(values.latitude) << "\" placeholder=\"例如: 31.2304\"></div>";
    
    html << "<div class=\"form-group\"><label for=\"longitude\">经度</label>";
    html << "<input id=\"longitude\" name=\"longitude\" value=\"" << htmlEscape(values.longitude) << "\" placeholder=\"例如: 121.4737\"></div>";
    
    html << "<div class=\"form-group\"><label for=\"osm_link\">OpenStreetMap 地图链接</label>";
    html << "<input id=\"osm_link\" placeholder=\"在此粘贴 OpenStreetMap 链接以提取坐标\"></div>";
    
    html << "<div class=\"callout callout-info hint\"><svg class=\"callout-icon\" width=\"16\" height=\"16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z\" /></svg>";
    html << "<p>打开 openstreetmap.org，定位到当前位置后复制浏览器地址栏的链接粘贴到此处，可自动提取经纬度。</p></div>";
    html << "</section>";
  ```

---

### Task 3: 优化 JavaScript 动态交互与前置校验 (Enhance JavaScript Micro-interactions)

本任务将在 `src/config/setup_page.cpp` 的尾部，为表单集成我们在 Task 1 中设计且被验证完备的高级交互脚本。

**Files:**
- Modify: `/Users/kenn/PROJECTS/m5stack/HomeDeck/src/config/setup_page.cpp:98-102` (替换 JavaScript 块)

- [ ] **Step 1: 在 C++ 端注入经过重构与增强的 Javascript 脚本**
  
  将原本非常冗长、缺乏反馈的 inline 脚本，替换为包含 Wi-Fi 药丸激活态绑定、OSM 提取闪烁、自动校时选项锁定、前端数字校验及防抖处理的现代化客户端交互引擎。

  将 `src/config/setup_page.cpp` 尾部对应部分替换为以下内容：
  ```cpp
    html << "<button type=\"submit\" id=\"submit_btn\" class=\"btn-submit\">保存</button></form>";
    html << "</div>"; // Container end
    html << "<script>";
    html << "const ssidInput=document.getElementById('wifi_ssid');";
    html << "const autoRtc=document.getElementById('auto_rtc');";
    html << "const errorContainer=document.getElementById('error_container');";
    html << "const errorMsg=errorContainer.querySelector('.msg')||document.getElementById('error_msg');";
    html << "const form=document.getElementById('setup_form');";
    html << "const submitBtn=document.getElementById('submit_btn');";
    html << "function syncRtcState(){";
    html << "  autoRtc.disabled=ssidInput.value.trim()==='';";
    html << "  if(autoRtc.disabled)autoRtc.checked=false;";
    html << "}";
    html << "function selectWifi(pill,ssidValue){";
    html << "  document.querySelectorAll('.wifi-pill').forEach(p=>p.classList.remove('active'));";
    html << "  pill.classList.add('active');";
    html << "  ssidInput.value=ssidValue;";
    html << "  syncRtcState();";
    html << "}";
    html << "document.querySelectorAll('.wifi-pill').forEach(pill=>{";
    html << "  pill.addEventListener('click',()=>selectWifi(pill,pill.dataset.ssid));";
    html << "});";
    html << "ssidInput.addEventListener('input',()=>{";
    html << "  syncRtcState();";
    html << "  document.querySelectorAll('.wifi-pill').forEach(p=>{";
    html << "    if(p.dataset.ssid===ssidInput.value)p.classList.add('active');";
    html << "    else p.classList.remove('active');";
    html << "  });";
    html << "});";
    html << "document.getElementById('osm_link').addEventListener('input',function(){";
    html << "  const val=this.value.trim();";
    html << "  let latVal='',lonVal='';";
    html << "  const hashMatch=val.match(/#map=[0-9.]+\\/([0-9.-]+)\\/([0-9.-]+)/);";
    html << "  if(hashMatch){";
    html << "    latVal=hashMatch[1];";
    html << "    lonVal=hashMatch[2];";
    html << "  }else{";
    html << "    const latMatch=val.match(/[?&](?:mlat|lat)=([0-9.-]+)/);";
    html << "    const lonMatch=val.match(/[?&](?:mlon|lon)=([0-9.-]+)/);";
    html << "    if(latMatch&&lonMatch){";
    html << "      latVal=latMatch[1];";
    html << "      lonVal=lonMatch[1];"; // Keep backward compatibility for original logic
    html << "    }";
    html << "  }";
    html << "  if(latVal&&lonVal){";
    html << "    const latInput=document.getElementById('latitude');";
    html << "    const lonInput=document.getElementById('longitude');";
    html << "    latInput.value=latVal;";
    html << "    lonInput.value=lonVal;";
    html << "    latInput.classList.remove('highlight-flash');";
    html << "    lonInput.classList.remove('highlight-flash');";
    html << "    void latInput.offsetWidth;"; // Trigger DOM reflow to restart css keyframe animation
    html << "    latInput.classList.add('highlight-flash');";
    html << "    lonInput.classList.add('highlight-flash');";
    html << "  }";
    html << "});";
    html << "form.addEventListener('submit',function(e){";
    html << "  errorContainer.style.display='none';";
    html << "  let errors=[];";
    html << "  const latVal=document.getElementById('latitude').value.trim();";
    html << "  const lonVal=document.getElementById('longitude').value.trim();";
    html << "  if(latVal!==\"\"&&isNaN(Number(latVal))) errors.push(\"纬度必须为有效的数字。\");";
    html << "  if(lonVal!==\"\"&&isNaN(Number(lonVal))) errors.push(\"经度必须为有效的数字。\");";
    html << "  if(errors.length>0){";
    html << "    e.preventDefault();";
    html << "    errorMsg.textContent=errors.join(\" \");";
    html << "    errorContainer.style.display='flex';";
    html << "    window.scrollTo({top:0,behavior:'smooth'});";
    html << "    return;";
    html << "  }";
    html << "  submitBtn.disabled=true;";
    html << "  submitBtn.textContent='正在保存...';";
    html << "});";
    html << "syncRtcState();";
    html << "</script></body></html>";
    return html.str();
  }
  ```

---

### Task 4: 编译、测试与端到端验证 (Build & Test Verification)

本任务对所有重构更改进行闭环的工程验证，确保没有破坏已有功能。

- [ ] **Step 1: 运行 Native 单元测试**
  
  在宿主机上运行 PlatformIO 的 Unity 单元测试，确保测试断言与我们的 DOM 升级能够完美匹配。

  Run: `pio test -e native`
  Expected: `5 Tests, 0 Failures, 0 Ignored` PASS

- [ ] **Step 2: 编译固件验证**
  
  对 ESP32-S3 目标版进行构建编译，验证是否有任何潜在的宏定义冲突或编译警告。

  Run: `pio run -e m5stack-papercolor`
  Expected: BUILD SUCCESS (生成最终 firmware.bin 固件)

- [ ] **Step 3: 提交 Git Commit**
  
  按照 Conventional Commits 规范，将样式改进进行提交。

  Run: 
  ```bash
  git add src/config/setup_page.cpp
  git commit -m "feat: overhaul setup page UI/UX to clean warm light mode with CSS Tokens"
  ```
