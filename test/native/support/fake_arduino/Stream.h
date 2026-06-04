#pragma once
#include <string>
#include <sstream>

class Stream {
 public:
  virtual ~Stream() = default;
  virtual std::string readStringUntil(char terminator) = 0;
  virtual bool available() = 0;
  virtual int peek() = 0;
};

class StringStream : public Stream {
 public:
  explicit StringStream(const std::string& data) : ss_(data) {}
  std::string readStringUntil(char terminator) override {
    std::string line;
    std::getline(ss_, line, terminator);
    return line;
  }
  bool available() override { return !ss_.eof(); }
  int peek() override { return ss_.peek(); }
 private:
  std::stringstream ss_;
};
