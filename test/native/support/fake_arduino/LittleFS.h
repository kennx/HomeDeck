#pragma once

class LittleFSClass {
 public:
  bool begin() {
    began = true;
    return beginSucceeds;
  }

  void end() {
    ended = true;
  }

  bool beginSucceeds = true;
  bool began = false;
  bool ended = false;
};

inline LittleFSClass LittleFS;
