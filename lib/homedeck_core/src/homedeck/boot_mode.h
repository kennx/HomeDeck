#pragma once

namespace homedeck {

enum class BootTarget {
  AccessPointSetup,
  ConnectAndHome,
};

struct BootInputs {
  bool configured;
  bool forceSetupButtons;
};

inline BootTarget decideBootTarget(const BootInputs& inputs) {
  if (!inputs.configured || inputs.forceSetupButtons) {
    return BootTarget::AccessPointSetup;
  }

  return BootTarget::ConnectAndHome;
}

}  // namespace homedeck
