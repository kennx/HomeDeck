#pragma once

#include <Preferences.h>
#include <WString.h>

#include "homedeck/config_types.h"

struct CalendarCacheRecord {
  String payload;
  String updatedAt;
};

class ConfigStore {
 public:
  bool begin();
  homedeck::SetupConfig loadSetupConfig() const;
  bool saveSetupConfig(const homedeck::SetupConfig& config);
  CalendarCacheRecord loadPersonalCalendarCache() const;
  CalendarCacheRecord loadHolidayCalendarCache() const;
  bool savePersonalCalendarCache(const CalendarCacheRecord& record);
  bool saveHolidayCalendarCache(const CalendarCacheRecord& record);

 private:
  CalendarCacheRecord loadCalendarCache(const char* payloadKey, const char* updatedAtKey) const;
  bool saveCalendarCache(
      const char* payloadKey,
      const char* updatedAtKey,
      const CalendarCacheRecord& record);

  mutable Preferences prefs_;
  bool started_ = false;
};
