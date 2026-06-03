#pragma once

#include <string_view>

#include "config/config_types.h"

namespace homedeck {

std::string_view trim(std::string_view value);
bool parseManualDateTime(std::string_view value, ManualDateTime* out);
bool isManualDateTimeValid(const ManualDateTime& value);
bool parseLatitude(std::string_view value, double* out);
bool parseLongitude(std::string_view value, double* out);
ConfigValidationResult validateSetupSubmission(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime);

}  // namespace homedeck
