#include "providers/webcal_provider.h"
#include <ArduinoJson.h>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <vector>

#if defined(ARDUINO)
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#else
#include <LittleFS.h>
#endif

namespace homedeck {

namespace {

// 平台通用的行读取函数，以确保不管是 native 还是 Arduino 都是 std::string
std::string readLine(Stream& stream) {
#if defined(ARDUINO)
  String s = stream.readStringUntil('\n');
  return std::string(s.c_str());
#else
  return stream.readStringUntil('\n');
#endif
}

// 辅助函数：除去末尾 \r
void trimRightInPlace(std::string& s) {
  while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
    s.pop_back();
  }
}

// 辅助函数：反转义
std::string unescapeSummary(const std::string& val) {
  std::string res;
  res.reserve(val.size());
  for (size_t i = 0; i < val.size(); ++i) {
    if (val[i] == '\\' && i + 1 < val.size()) {
      char next = val[i + 1];
      if (next == 'n' || next == 'N') {
        res += '\n';
      } else if (next == 't' || next == 'T') {
        res += '\t';
      } else {
        res += next;
      }
      i++;
    } else {
      res += val[i];
    }
  }
  return res;
}

// 辅助函数：安全转换 YYYYMMDD
bool parseDate(const std::string& val, int& outYear, int& outMonth, int& outDay) {
  if (val.size() < 8) return false;
  for (int i = 0; i < 8; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(val[i]))) return false;
  }
  outYear = (val[0] - '0') * 1000 + (val[1] - '0') * 100 + (val[2] - '0') * 10 + (val[3] - '0');
  outMonth = (val[4] - '0') * 10 + (val[5] - '0');
  outDay = (val[6] - '0') * 10 + (val[7] - '0');
  return true;
}

// 辅助函数：Rata Die 算法计算公元以来的天数
int daysFromCivil(int year, int month, int day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
  const unsigned dayOfYear = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  return era * 146097 + static_cast<int>(dayOfEra) - 719468;
}

// 格式化 YYYY-MM-DD
std::string formatDate(int year, int month, int day) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
  return std::string(buf);
}

// 拆分属性 键[:参数]: 值
bool parseIcsProperty(const std::string& line, std::string& outKey, std::string& outValue) {
  size_t colonPos = line.find(':');
  if (colonPos == std::string::npos) {
    return false;
  }
  outKey = line.substr(0, colonPos);
  outValue = line.substr(colonPos + 1);
  return true;
}

// 过滤 URL 协议
std::string normalizeUrl(const std::string& url) {
  if (url.rfind("webcal://", 0) == 0) {
    return "https://" + url.substr(9);
  }
  return url;
}

// 单行处理与过滤的核心逻辑
void handleEvent(const std::string& startVal, const std::string& summaryVal, const std::tm& localNow, std::map<std::string, std::string>& outFestivals) {
  int eventYear = 0, eventMonth = 0, eventDay = 0;
  if (!parseDate(startVal, eventYear, eventMonth, eventDay)) {
    return;
  }

  int nowDays = daysFromCivil(localNow.tm_year + 1900, localNow.tm_mon + 1, localNow.tm_mday);
  int eventDays = daysFromCivil(eventYear, eventMonth, eventDay);
  int diff = eventDays - nowDays;

  if (std::abs(diff) <= 40) {
    std::string dateKey = formatDate(eventYear, eventMonth, eventDay);
    std::string cleanSummary = unescapeSummary(summaryVal);
    if (outFestivals.find(dateKey) != outFestivals.end()) {
      outFestivals[dateKey] += " " + cleanSummary;
    } else {
      outFestivals[dateKey] = cleanSummary;
    }
  }
}

}  // namespace

bool parseIcsStream(Stream& stream, const std::tm& localNow, std::map<std::string, std::string>& outFestivals) {
  std::string currentLine = "";
  bool hasCurrent = false;

  bool inEvent = false;
  std::string eventStart = "";
  std::string eventSummary = "";

  auto processLine = [&](const std::string& line) {
    std::string key, val;
    if (!parseIcsProperty(line, key, val)) {
      return;
    }

    if (key == "BEGIN" && val == "VEVENT") {
      inEvent = true;
      eventStart = "";
      eventSummary = "";
    } else if (key == "END" && val == "VEVENT") {
      if (inEvent && !eventStart.empty() && !eventSummary.empty()) {
        handleEvent(eventStart, eventSummary, localNow, outFestivals);
      }
      inEvent = false;
    } else if (inEvent) {
      if (key == "DTSTART" || key.rfind("DTSTART;", 0) == 0) {
        eventStart = val;
      } else if (key == "SUMMARY" || key.rfind("SUMMARY;", 0) == 0) {
        eventSummary = val;
      }
    }
  };

  while (stream.available()) {
    std::string rawLine = readLine(stream);
    trimRightInPlace(rawLine);
    if (rawLine.empty()) {
      continue;
    }

    if (rawLine[0] == ' ' || rawLine[0] == '\t') {
      if (hasCurrent) {
        currentLine += rawLine.substr(1);
      }
    } else {
      if (hasCurrent) {
        processLine(currentLine);
      }
      currentLine = rawLine;
      hasCurrent = true;
    }
  }

  if (hasCurrent) {
    processLine(currentLine);
  }

  return true;
}

bool syncWebcalFestivals(const std::string& webcalUrl, const std::string& wifiSsid, const std::string& wifiPassword) {
#if defined(ARDUINO)
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  std::string targetUrl = normalizeUrl(webcalUrl);
  if (!http.begin(client, targetUrl.c_str())) {
    WiFi.disconnect(true);
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    WiFi.disconnect(true);
    return false;
  }

  Stream& stream = http.getStream();
  std::time_t now = std::time(nullptr);
  std::tm* localNow = std::localtime(&now);
  if (localNow == nullptr) {
    http.end();
    WiFi.disconnect(true);
    return false;
  }

  std::map<std::string, std::string> outFestivals;
  bool parseOk = parseIcsStream(stream, *localNow, outFestivals);
  http.end();
  WiFi.disconnect(true);

  if (!parseOk) {
    return false;
  }

  if (!LittleFS.begin()) {
    LittleFS.end();
    return false;
  }
  File file = LittleFS.open("/webcal_cache.json", "w");
  if (!file) {
    LittleFS.end();
    return false;
  }

  JsonDocument doc;
  for (const auto& pair : outFestivals) {
    doc[pair.first] = pair.second;
  }

  std::string jsonStr;
  serializeJson(doc, jsonStr);
  size_t bytesWritten = file.write(reinterpret_cast<const uint8_t*>(jsonStr.c_str()), jsonStr.size());
  file.close();
  LittleFS.end();

  return bytesWritten > 0;
#else
  (void)webcalUrl;
  (void)wifiSsid;
  (void)wifiPassword;
  return false;
#endif
}

bool loadCachedFestivals(std::map<std::string, std::string>& outFestivals) {
  if (!LittleFS.begin()) {
    LittleFS.end();
    return false;
  }
  if (!LittleFS.exists("/webcal_cache.json")) {
    LittleFS.end();
    return false;
  }
  File file = LittleFS.open("/webcal_cache.json", "r");
  if (!file) {
    LittleFS.end();
    return false;
  }
  size_t size = file.size();
  std::vector<char> buf(size + 1, '\0');
  size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(buf.data()), size);
  file.close();
  LittleFS.end();

  if (bytesRead == 0) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buf.data());
  if (error) {
    return false;
  }

  JsonObject root = doc.as<JsonObject>();
  for (JsonPair p : root) {
    outFestivals[p.key().c_str()] = p.value().as<std::string>();
  }
  return true;
}

}  // namespace homedeck
