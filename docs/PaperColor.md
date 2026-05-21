# M5Stack PaperColor 开发板 - 详细文档

> **SKU**: C151
> **参考链接**: [M5Stack PaperColor 官方文档](https://docs.m5stack.com/zh_CN/core/PaperColor)

---

## 📋 概述

PaperColor 是一款配备 **4 英寸 E Ink Spectra 6 全彩墨水屏** 的开发板，屏幕分辨率为 **400×600**，兼具低功耗与高可视性优势。内置集成 **ESP32-S3R8** 核心主控，配备 **16MB Flash** 与 **8MB PSRAM**，支持 **2.4GHz Wi-Fi** 无线通信。

开发板集成了完整的人机交互系统，包括 **3 个可编程按键**、基于 **ES8311** 音频编解码器的音频系统、具备 **AEC 回声消除** 功能的 MEMS 麦克风以及 **AW8737A** 功放驱动的扬声器，支持高质量语音拾取与音频播放。

扩展功能方面，板载 **SHT40 温湿度传感器**、**RX8130CE 实时时钟**、**microSD 卡槽**、**红外发射管**、**2 个 RGB LED** 及 **HY2.0-4P 扩展接口**，配合 **M5PM1 多级电源管理系统** 与 **1250mAh 锂电池**，可实现持久的低功耗运行。

**适用场景**：电子标牌、语音交互终端、环境监测仪表、艺术展示装置等物联网与嵌入式场景。

---

## 🔧 核心规格参数

| 规格 | 参数 |
|------|------|
| **SoC** | ESP32-S3R8 @ 双核 Xtensa LX7 处理器，主频高达 **240MHz** |
| **Flash** | **16MB** |
| **PSRAM** | **8MB** |
| **Wi-Fi** | 2.4 GHz |
| **屏幕** | 4" E-Paper (E6 Full-Color) ED2208-DOA (EL040EF1)，分辨率 **400×600** |
| **输入电源** | USB Type-C DC 5V |
| **电池容量** | **1250mAh** |
| **音频编解码器** | **ES8311** |
| **麦克风** | MEMS 麦克风，ES7210 音频 ADC，集成 **AEC 电路**（回声消除） |
| **扬声器** | **1W** @ 8Ω 2520 扬声器，AW8737A 音频功放 |
| **温湿度传感器** | **SHT40** |
| **拓展存储** | **microSD** 卡槽 |
| **RTC** | **RX8130CE** 实时时钟 |
| **用户按键** | **3× 用户按键** + **1× 电源按键** (ON/OFF/RESET/BOOT) |
| **红外** | 红外发射管 |
| **LED** | **2× RGB LED** |
| **功耗 - 待机状态** | **92.53μA** |
| **功耗 - 满载状态** | **211.97mA** |
| **产品尺寸** | **70.8 × 103.9 × 8.5mm** |
| **产品重量** | **73.3g** |
| **包装尺寸** | 120.0 × 80.0 × 23.0mm |
| **毛重** | 93.4g |

---

## 🎯 产品特性

- ✅ **ESP32-S3R8** 核心主控：16MB Flash + 8MB PSRAM
- ✅ **2.4GHz Wi-Fi** 无线通信
- ✅ **4 英寸 E Ink Spectra 6 全彩墨水屏** — 低功耗、高可视性
- ✅ **内置 1250mAh 锂电池** — 持久续航
- ✅ **microSD 卡槽** — 扩展存储
- ✅ **音频交互系统**：
  - ES8311 音频编解码器
  - 1W 扬声器 + MEMS 麦克风（带 AEC 回声消除）
- ✅ **人机交互**：
  - 3× 用户按键（Button A/B/C）
  - 1× 电源按键
  - 红外发射管
  - 2× RGB LED
- ✅ **环境监测**：板载 **SHT40** 温湿度传感器
- ✅ **实时时钟**：**RX8130CE** RTC
- ✅ **扩展接口**：HY2.0-4P（Grove 接口）
- ✅ **电源管理**：M5PM1 多级电源管理系统

---

## 🕹️ 操作说明

### 开关机

| 操作 | 方式 |
|------|------|
| **开机 / 重启** | 短按一次电源按钮 |
| **关机** | 连续按两次电源按钮 |

### 进入下载模式（烧录固件）

> 如需烧录固件，请将设备通过 USB Type-C 数据线连接至电脑，**长按侧面的复位按键**即可进入下载模式，等待烧录。

---

## 🔌 管脚映射（Pinout）

### E-Paper 墨水屏

| ESP32-S3R8 | 信号 | 功能 |
|------------|------|------|
| G15 | SPI_CLK | SPI 时钟 |
| G13 | SPI_MOSI | SPI 数据输出 |
| G44 | EINK_CS | 墨水屏片选 |
| G43 | EINK_DC | 墨水屏数据/命令选择 |
| G11 | EINK_BUSY | 墨水屏忙信号 |
| G12 | EINK_RST | 墨水屏复位 |
| M5PM1 PYG0 | PY_EPD_EN | 墨水屏供电使能 |

### 按键（KEY）

| ESP32-S3R8 | 信号 | 功能 |
|------------|------|------|
| G1 | USER_KEY1 | Button C |
| G9 | USER_KEY2 | Button B |
| G10 | USER_KEY3 | Button A |

### 红外（IR）

| ESP32-S3R8 | 信号 | 功能 |
|------------|------|------|
| G48 | IR_TX | 红外发射 |

### RGB LED

| ESP32-S3R8 | 信号 | 功能 |
|------------|------|------|
| G21 | RGB | RGB LED 控制 |

### 音频系统

#### I2C 控制

| ESP32-S3R8 | 信号 | 芯片 | 地址 |
|------------|------|------|------|
| G2 | AUDIO_I2C_SCL | ES8311 | 0x18 |
| G3 | AUDIO_I2C_SDA | ES8311 | 0x18 |
| G2 | AUDIO_I2C_SCL | ES7210 | 0x40 |
| G3 | AUDIO_I2C_SDA | ES7210 | 0x40 |

#### I2S 音频

| ESP32-S3R8 | 信号 | 功能 |
|------------|------|------|
| G42 | I2S_MCLK | 主时钟 |
| G41 | I2S_LRCK | 左右声道时钟 |
| G40 | I2S_BCLK | 位时钟 |
| G39 | I2S_DSDIN | DAC 数据输入（ES8311） |
| G38 | I2S_SDOUT | ADC 数据输出（ES7210） |

#### 电源控制

| ESP32-S3R8 | 信号 | 功能 |
|------------|------|------|
| G45 | AUDIO_PWR_EN | 音频编解码芯片与麦克风供电 |
| G46 | SPK_EN | 扬声器功放使能 |

### 实时时钟（RTC）

| ESP32-S3R8 | 信号 | 芯片 | 地址 |
|------------|------|------|------|
| G7 | RTC_IRQ | RX8130CE | 0x32 |
| G3 | SYS_SDA | RX8130CE | 0x32 |
| G2 | SYS_SCL | RX8130CE | 0x32 |

### 温湿度传感器（SHT40）

| ESP32-S3R8 | 信号 | 芯片 | 地址 |
|------------|------|------|------|
| G3 | SYS_SDA | SHT40 | 0x44 |
| G2 | SYS_SCL | SHT40 | 0x44 |

### microSD 卡槽

| ESP32-S3R8 | 信号 | 功能 |
|------------|------|------|
| G47 | CS | 片选 |
| G15 | SPI_CLK | SPI 时钟 |
| G13 | SPI_MOSI | SPI 数据输出 |
| G14 | SPI_MISO | SPI 数据输入 |

### HY2.0-4P 扩展接口（PORT.A / Grove）

| 颜色 | 信号 | ESP32-S3R8 |
|------|------|------------|
| 黑色 | GND | 地线 |
| 红色 | 5V | 5V 电源 |
| 黄色 | G4 | GPIO4 |
| 白色 | G5 | GPIO5 |

### M5PM1 电源管理芯片

#### I2C 控制

| ESP32-S3R8 | 信号 | 芯片 | 地址 |
|------------|------|------|------|
| G3 | SYS_SDA | M5PM1 | 0x6e |
| G2 | SYS_SCL | M5PM1 | 0x6e |

#### 电源控制信号

| M5PM1 信号 | 功能 |
|------------|------|
| DCDC3V3_EN_PP (PY_MPWR_EN) | 3V3_L2 层电源开关 |
| LDO3V3_EN_PP (RTCPY_RGB_PWR_EN_IRQ) | RGB LED 供电开关 |
| BOOST5V_EN_PP (PY_GROVE_OUT_EN) | Grove 拓展接口供电方向控制 |

#### M5PM1 扩展控制

| M5PM1 | 信号 | 功能 |
|-------|------|------|
| PYG0 | PY_EPD_EN | 开启墨水屏供电 |
| PYG2 | RTC_IRQ | RTC 中断信号 |
| PYG3 | PY_SD_PWR_EN | microSD 模块供电 |
| PYG4 | PY_SD_DET_EN | microSD 检测功能启用，使能上拉 |
| PYG1 | CARD_DEC | microSD 插入检测 |

---

## 📐 原理图与尺寸图

| 资源 | 链接 |
|------|------|
| **PaperColor 原理图 PDF** | [下载](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1239/C151-SCH_PaperColor_V0.5_SCH_PDF_20260424_EN_2026_04_24_11_01_17.pdf) |
| **PaperColor 模型尺寸 PDF** | [下载](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1239/C151PaperColor_model_size.pdf) |

---

## 📚 数据手册（Datasheet）

| 芯片 | 手册链接 |
|------|----------|
| **ESP32-S3** | [技术参考手册](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/477/esp32-s3_technical_reference_manual_cn.pdf) |
| **ES8311** | [音频编解码器](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/atom/Atomic%20Echo%20Base/ES8311.pdf) |
| **BMI270** | [6轴IMU](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/K128%20CoreS3/BMI270.PDF) |

---

### 通信协议

| 资源 | 链接 |
|------|------|
| M5PM1 电源管理芯片数据手册 | [下载](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1207/M5PM1_Datasheet_CN.pdf) |

### Easyloader（出厂固件）

| 资源 | 链接 | 备注 |
|------|------|------|
| PaperColor 出厂固件 Easyloader | [下载](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1239/C151-papercolor-factory-fw.exe) | 烧录工具 |

### 其他教程

| 资源 | 链接 |
|------|------|
| PaperColor 烧录出厂固件教程 | [教程](https://docs.m5stack.com/zh_CN/guide/restore_factory/papercolor) |
| PaperColor 出厂固件使用教程 | [教程](https://docs.m5stack.com/zh_CN/guide/display_device/papercolor/usage) |


## 🔗 参考链接汇总

| 类型 | 链接 |
|------|------|
| **官方文档** | https://docs.m5stack.com/zh_CN/core/PaperColor |
| **原理图 PDF** | https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1239/C151-SCH_PaperColor_V0.5_SCH_PDF_20260424_EN_2026_04_24_11_01_17.pdf |
| **尺寸图 PDF** | https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1239/C151PaperColor_model_size.pdf |
| **出厂固件源码** | https://github.com/m5stack/M5PaperColor-UserDemo |
| **M5PM1 Library** | https://github.com/m5stack/M5PM1 |
| **M5Unified Library** | https://github.com/m5stack/M5Unified |
| **M5GFX Library** | https://github.com/m5stack/M5GFX |
| **出厂固件 Easyloader** | https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1239/C151-papercolor-factory-fw.exe |

