#pragma once

#include <cstdint>
#include <vector>

struct FakeI2C;

using m5pm1_err_t = int;

inline constexpr m5pm1_err_t M5PM1_OK = 0;
inline constexpr std::uint8_t M5PM1_DEFAULT_ADDR = 0x6e;
inline constexpr std::uint32_t M5PM1_I2C_FREQ_100K = 100000;

inline int gFakePm1BeginCount = 0;
inline bool gFakePm1BeginShouldFail = false;
inline std::vector<bool> gFakePm1LdoEnableCalls;

inline void fakeM5Pm1Reset() {
  gFakePm1BeginCount = 0;
  gFakePm1BeginShouldFail = false;
  gFakePm1LdoEnableCalls.clear();
}

class M5PM1 {
 public:
  m5pm1_err_t begin(FakeI2C*, std::uint8_t, std::uint32_t) {
    ++gFakePm1BeginCount;
    return gFakePm1BeginShouldFail ? -1 : M5PM1_OK;
  }

  m5pm1_err_t setLdoEnable(bool enable) {
    gFakePm1LdoEnableCalls.push_back(enable);
    return M5PM1_OK;
  }
};
