#pragma once

#include <functional>
#include <map>
#include <string>

constexpr int HTTP_GET = 0;
constexpr int HTTP_POST = 1;
constexpr int HTTP_ANY = 255;

class WebServer {
 public:
  struct Handler {
    int method = HTTP_GET;
    std::function<void()> handler;
  };

  explicit WebServer(int port) : port_(port) {
    WebServer::gLastWebServer = this;
  }

  void on(const char* path, int method, std::function<void()> handler) {
    handlers_[path] = Handler{method, std::move(handler)};
  }

  void onNotFound(std::function<void()> handler) {
    notFoundHandler_ = std::move(handler);
  }

  void begin() { started_ = true; }

  void handleClient() { ++handleClientCount; }

  std::string arg(const char* name) const {
    const auto found = args_.find(name);
    return found == args_.end() ? std::string{} : found->second;
  }

  void sendHeader(const char* name, const char* value, bool = false) {
    headers_[name != nullptr ? name : ""] = value != nullptr ? value : "";
  }

  void send(int status, const char* type, const char* body) {
    lastStatus = status;
    lastType = type != nullptr ? type : "";
    lastBody = body != nullptr ? body : "";
  }

  bool invoke(const char* path, int method = HTTP_GET) {
    const std::string key = path != nullptr ? path : "";
    const auto found = handlers_.find(key);
    if (found != handlers_.end() && (found->second.method == method || found->second.method == HTTP_ANY)) {
      found->second.handler();
      return true;
    }
    if (notFoundHandler_) {
      notFoundHandler_();
      return true;
    }
    return false;
  }

  int port_ = 80;
  bool started_ = false;
  int handleClientCount = 0;
  int lastStatus = 0;
  std::string lastType;
  std::string lastBody;
  std::map<std::string, std::string> args_;
  std::map<std::string, std::string> headers_;
  std::map<std::string, Handler> handlers_;
  std::function<void()> notFoundHandler_;

  static WebServer* gLastWebServer;
};

inline WebServer* WebServer::gLastWebServer = nullptr;
inline WebServer*& gLastWebServer = WebServer::gLastWebServer;
