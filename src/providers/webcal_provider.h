#pragma once

#include <string>
#include <map>
#include <ctime>

#include <Stream.h>

namespace homedeck {

bool parseIcsStream(Stream& stream, const std::tm& localNow, std::map<std::string, std::string>& outFestivals);
bool syncWebcalFestivals(const std::string& webcalUrl, const std::string& wifiSsid, const std::string& wifiPassword);
bool loadCachedFestivals(std::map<std::string, std::string>& outFestivals);

}  // namespace homedeck
