#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import re

def main():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(current_dir)
    preview_path = os.path.join(project_dir, 'scratch', 'preview.html')
    output_path = os.path.join(project_dir, 'src', 'generated', 'setup_page_html.h')

    if not os.path.exists(preview_path):
        print(f"Error: {preview_path} not found.")
        return

    with open(preview_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. 替换 Wi-Fi Grid 模拟项为占位符
    wifi_pattern = r'<!-- WIFI_GRID_ITEMS_TEMPLATE_START -->.*?<!-- WIFI_GRID_ITEMS_TEMPLATE_END -->'
    content, wifi_replaced = re.subn(wifi_pattern, '{{WIFI_GRID_ITEMS}}', content, flags=re.DOTALL)
    if wifi_replaced == 0:
        print("Warning: WIFI_GRID_ITEMS_TEMPLATE tags not found.")

    # 2. 替换 Timezone Options 模拟项为占位符
    tz_pattern = r'<!-- TIMEZONE_OPTION_ITEMS_TEMPLATE_START -->.*?<!-- TIMEZONE_OPTION_ITEMS_TEMPLATE_END -->'
    content, tz_replaced = re.subn(tz_pattern, '{{TIMEZONE_OPTION_ITEMS}}', content, flags=re.DOTALL)
    if tz_replaced == 0:
        print("Warning: TIMEZONE_OPTION_ITEMS_TEMPLATE tags not found.")

    # 3. 替换热点名、默认 values 占位符
    content = content.replace('HomeDeck-ABCD', '{{AP_SSID}}')
    
    # 替换错误容器样式和初始值
    content = content.replace('id="error_container" class="callout callout-error" style="display:none;"', 'id="error_container" class="callout callout-error" style="{{ERROR_CONTAINER_STYLE}}"')
    content = content.replace('<p id="error_msg" class="msg"></p>', '<p id="error_msg" class="msg">{{ERROR_MESSAGE}}</p>')
    
    # 替换表单控件 values 默认值（使用正则提高容错性）
    content = re.sub(r'id="wifi_ssid"\s+name="wifi_ssid"\s+value=""', 'id="wifi_ssid" name="wifi_ssid" value="{{WIFI_SSID}}"', content)
    content = re.sub(r'id="wifi_password"\s+name="wifi_password"\s+type="password"\s+value=""', 'id="wifi_password" name="wifi_password" type="password" value="{{WIFI_PASSWORD}}"', content)
    content = re.sub(r'id="auto_rtc"\s+name="auto_rtc"\s+type="checkbox"\s+value="1"\s+disabled', 'id="auto_rtc" name="auto_rtc" type="checkbox" value="1" {{AUTO_RTC_CHECKED}} {{AUTO_RTC_DISABLED}}', content)
    content = re.sub(r'id="ntp_server"\s+name="ntp_server"\s+value="ntp\.aliyun\.com"', 'id="ntp_server" name="ntp_server" value="{{NTP_SERVER}}"', content)
    content = re.sub(r'id="latitude"\s+name="latitude"\s+value=""', 'id="latitude" name="latitude" value="{{LATITUDE}}"', content)
    content = re.sub(r'id="longitude"\s+name="longitude"\s+value=""', 'id="longitude" name="longitude" value="{{LONGITUDE}}"', content)
    
    # 写入前断言所有占位符均已正确注入
    required_placeholders = [
        '{{AP_SSID}}', '{{WIFI_GRID_ITEMS}}', '{{TIMEZONE_OPTION_ITEMS}}',
        '{{ERROR_CONTAINER_STYLE}}', '{{ERROR_MESSAGE}}', '{{WIFI_SSID}}',
        '{{WIFI_PASSWORD}}', '{{NTP_SERVER}}', '{{LATITUDE}}', '{{LONGITUDE}}',
        '{{AUTO_RTC_CHECKED}}', '{{AUTO_RTC_DISABLED}}',
        '{{BATTERY_INFO_STYLE}}', '{{BATTERY_INFO}}'
    ]
    missing = [ph for ph in required_placeholders if ph not in content]
    if missing:
        raise RuntimeError(f"sync_preview.py: missing placeholders after replacement: {missing}")

    # 生成 C++ 原始字符串字面量头文件 (加入跨平台条件编译保护，支持宿主机 native 单元测试运行)
    header_content = f"""#pragma once

#ifdef ARDUINO
#include <pgmspace.h>
#else
#define PROGMEM
#endif

namespace homedeck {{

const char SETUP_PAGE_TEMPLATE[] PROGMEM = R"raw({content})raw";

}} // namespace homedeck
"""

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(header_content)

    print(f"Successfully synchronized {preview_path} to {output_path}")

if __name__ == '__main__':
    main()
