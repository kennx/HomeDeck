# HomeDeck 配置网页 UI/UX 现代化样式设计方案

本文档详细记录了关于 M5Stack PaperColor 设备在 SoftAP 模式下托管的“Web 配置网页”的现代化样式重构设计与 CSS Tokens 规范。

---

## 1. 目标描述 (Goal Description)

目前的配置页面（托管在 ESP32 的 192.168.4.1）采用极其简陋的默认无样式排版，可用性与视觉美观度较低。

本方案旨在**不依赖任何外部 CDN 资源（如 CSS 框架、Google Fonts 等网络文件）的前提下**，通过手写原生现代化 CSS 和轻量级 JavaScript，将配置页面重构为**精致温和明亮风（Warm & Clean Light Mode）**的响应式布局。它不仅大幅提升了视觉档次，更通过结构化卡片排版和微交互动画显著增强了设备配网的成功率与用户体验。

---

## 2. 核心设计原则 (Design Principles)

1. **完全自包含 (Self-contained)**：由于用户连接 ESP32 热点时没有外网，所有样式、交互和图标必须使用纯粹的 CSS/SVG 实现，确保零外部依赖。
2. **极其轻量化 (Ultra Lightweight)**：HTML+CSS 字符串体积控制在 12KB 以内，以最小化 ESP32 内存（RAM）和闪存（Flash）负担。
3. **视觉一致性与美观度 (Aesthetic Excellence)**：通过物理软阴影与大圆角的白色卡片，配合深邃的靛蓝强调色，打造出符合现代高端智能硬件的居家科技感。
4. **移动端优先的响应式 (Mobile-First Responsive)**：完美契合手机和电脑端，特别是通过规整的“Wi-Fi 药丸标签”优化长 Wi-Fi 列表，避免提交按钮被无限挤压出屏幕。

---

## 3. CSS 设计系统与 Tokens 规范 (CSS Design Tokens)

为方便后续开发人员快速修改或扩展网页样式，所有的核心视觉属性均已抽象为 CSS 变量。

### 🎨 颜色 Tokens (Color Palette)

| 变量名 | 默认值 | 视觉用途 |
| :--- | :--- | :--- |
| `--primary-color` | `#4F46E5` | **主品牌色 (靛蓝)**：用于核心按钮背景、聚焦框发光边框 |
| `--primary-hover` | `#4338CA` | **主品牌色悬停态**：略微加深，提供按钮悬浮反馈 |
| `--primary-light` | `#EEF2FF` | **轻度高亮底色**：用于选中状态下的轻度背景填充 |
| `--bg-page` | `#F4F5F7` | **页面底色 (温润灰蓝)**：作为大背景，呈现温和高档质感 |
| `--bg-card` | `#FFFFFF` | **卡片背景**：纯白色，提供清晰的内容承载面 |
| `--bg-input` | `#F9FAFB` | **输入框背景**：极浅灰色，降低默认视觉噪音 |
| `--bg-input-focus`| `#FFFFFF` | **输入框聚焦背景**：聚焦时转为纯白，增强关注度 |

### 📢 状态反馈 Tokens (Feedback Status)

| 变量名 | 默认值 | 视觉用途 |
| :--- | :--- | :--- |
| `--color-success` | `#10B981` | **成功色 (翠绿)**：用于运行状态徽章、强信号指示 |
| `--color-success-bg`| `#ECFDF5` | **成功状态框背景**：轻柔翠绿 |
| `--color-error` | `#EF4444` | **错误色 (柔红)**：用于表单报错、输入非法提示 |
| `--color-error-bg` | `#FEF2F2` | **错误框背景**：轻柔粉红 Callout |
| `--color-error-border`| `#FCA5A5`| **错误框边框** |
| `--color-info` | `#3B82F6` | **信息色 (天蓝)**：用于操作向导和常规提示 |
| `--color-info-bg` | `#EFF6FF` | **信息框背景**：轻柔浅蓝 Callout |
| `--color-info-border`| `#BFDBFE` | **信息框边框** |

### ✍️ 文本色阶 Tokens (Typography Colors)

| 变量名 | 默认值 | 视觉用途 |
| :--- | :--- | :--- |
| `--text-primary` | `#1F2937` | **主文本色 (炭灰)**：代替死板的纯黑，降低眼部疲劳 |
| `--text-secondary`| `#4B5563` | **次文本色 (中灰)**：用于 Form Label、AP 说明文本 |
| `--text-tertiary` | `#9CA3AF` | **弱辅助色 (浅灰)**：用于 Placeholder、输入提示注脚 |
| `--text-on-primary`| `#FFFFFF` | **按钮前景文本 (纯白)**：用于靛蓝按钮上的文字 |

### 📐 布局、间距与圆角 Tokens (Layout & Spacing)

| 变量名 | 默认值 | 描述 |
| :--- | :--- | :--- |
| `--radius-card` | `16px` | 卡片超大圆角，符合现代 iOS 和智能家居风格 |
| `--radius-input` | `8px` | 输入框、按钮及中型容器的圆角 |
| `--radius-badge` | `6px` | 徽章和 Wi-Fi 药丸按钮的紧凑圆角 |
| `--border-color` | `#E5E7EB` | 精细边框线颜色 |
| `--space-xs` | `4px` | 超小边距 |
| `--space-sm` | `8px` | 小边距 |
| `--space-md` | `16px` | 中等边距，默认容器内 padding |
| `--space-lg` | `24px` | 大边距，宽屏卡片内 padding |
| `--space-xl` | `32px` | 超大边距 |

### 💫 阴影与过渡 Tokens (Shadows & Transitions)

| 变量名 | 默认值 | 描述 |
| :--- | :--- | :--- |
| `--shadow-card` | `0 4px 6px -1px rgba(0, 0, 0, 0.05), ...` | 卡片软阴影，创造高级的三维悬浮感 |
| `--shadow-focus` | `0 0 0 3px rgba(79, 70, 229, 0.15)` | 输入框聚焦时的浅靛蓝柔和呼吸光晕 |
| `--transition-fast`| `0.15s ease` | 按钮 Hover、Input 变色等微交互速度 |
| `--transition-normal`| `0.25s ease-in-out` | 卡片高亮切换等速度 |

---

## 4. 提议的更改 (Proposed Changes)

我们将对 `src/config/setup_page.cpp` 进行局部重构。

### DOM 结构与样式改进方案：

* **页面包裹**：所有卡片居中放置在 `.container` 内，最大宽度 `640px`。
* **头部 Banner 卡片**：顶部设计有精致的 HomeDeck 设备 logo 图标（内联 SVG），以及带绿色小呼吸灯的 `AP 运行中` 状态徽章。
* **Wi-Fi 药丸按钮 grid**：原本垂直平铺的大按钮，重构为 `.wifi-grid`，按钮采用紧凑的药丸按钮（Pill），左侧配有三格渐高的微型 CSS 信号格图标。
* **Notion 风格 Callout 框**：错误消息提示（`.msg`）与 OSM 提示（`.hint`）分别重构成浅红色和浅蓝色的 Callout 卡片，具有精美边框和图标。
* **输入框焦点动画**：在 `:focus` 时，通过 `transition` 实现背景转白、边框变靛蓝并向外扩散浅色光圈（`--shadow-focus`）的精致视效。

### JavaScript 交互增强：

1. **Wi-Fi 药丸点击激活态**：点击 Wi-Fi 药丸按钮后，通过 JS 为该按钮添加 `.active` 类，移除其他药丸的激活状态。
2. **OSM 经纬度提取闪烁特效**：当 OSM 链接解析并填充经纬度成功后，触发 `highlight-flash` CSS 关键帧动画，使经纬度输入框呈现一个短暂的（0.6s）浅绿色背景微呼吸，直观告知用户提取完成。
3. **RTC 复选框状态同步**：当且仅当 Wi-Fi SSID 不为空时可用；为空时自动置灰并取消勾选。
4. **前端校验与提交防抖**：保存时前端进行简单非空和数值类型检测，报错直接渲染在顶部的 Error Callout 中；点击提交后，保存按钮进入 `disabled` 状态并显示“正在保存...”，防止二次提交。

---

## 5. 验证计划 (Verification Plan)

### 5.1 编译与自动化测试

* **固件编译**：运行编译命令 `pio run -e m5stack-papercolor`，保证零警告，且整体固件体积无突增。
* **单元测试**：运行 `pio test -e native`。如果由于 HTML 重构导致现有的 `test_setup_page` 测试出现字符串断言不匹配，将同步更新测试用例，维持 100% 单元测试通过率。

### 5.2 静态页面预览（手动验证）

1. 我们将在本地临时目录下生成一个包含测试 Wi-Fi 列表的静态网页文件 `preview.html`。
2. 使用网页浏览器打开此文件。
3. 测试以下交互：
   * 点击 Wi-Fi 药丸按钮，查看 SSID 输入框是否填充，对应药丸是否变为高亮的 `.active` 状态。
   * 缩放浏览器视口大小，验证在手机屏幕宽度（375px 至 420px）和电脑屏幕宽度（> 640px）下的响应式排版是否完全正常、无内容溢出。
   * 粘贴合法的 OSM 链接（如 `https://www.openstreetmap.org/#map=19/25.7817/113.0199`），检查经纬度是否正确填充，并且输入框是否触发了绿色的“提取成功”呼吸闪烁特效。
