#pragma once

#include <sys/time.h>

using sntp_sync_time_cb_t = void (*)(struct timeval*);

inline sntp_sync_time_cb_t gFakeSntpCallback = nullptr;

inline void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t callback) {
  gFakeSntpCallback = callback;
}

inline void fakeSntpNotifySync() {
  if (gFakeSntpCallback == nullptr) {
    return;
  }

  timeval value{};
  gFakeSntpCallback(&value);
}
