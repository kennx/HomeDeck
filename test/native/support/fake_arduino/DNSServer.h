#pragma once

#include <cstdint>
#include <string>

#include <WiFi.h>

enum class DNSReplyCode {
  NoError = 0,
};

class DNSServer {
 public:
  DNSServer() {
    DNSServer::gLastDnsServer = this;
  }

  bool start(const std::uint16_t& port, const char* domainName, const IPAddress& resolvedIP) {
    started = true;
    portValue = port;
    domain = domainName != nullptr ? domainName : "";
    resolvedIp = resolvedIP.toString();
    return startSucceeds;
  }

  void processNextRequest() {
    ++processCount;
  }

  void stop() {
    started = false;
  }

  void setErrorReplyCode(const DNSReplyCode&) {}
  void setTTL(const std::uint32_t&) {}

  bool startSucceeds = true;
  bool started = false;
  std::uint16_t portValue = 0;
  std::string domain;
  std::string resolvedIp;
  int processCount = 0;

  static DNSServer* gLastDnsServer;
};

inline DNSServer* DNSServer::gLastDnsServer = nullptr;
inline DNSServer*& gLastDnsServer = DNSServer::gLastDnsServer;
