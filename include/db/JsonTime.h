// OOP: Utilities (JSON time conversions)
#pragma once
#include "json.hpp"
#include <chrono>
#include <cstdint>

namespace db {

// Reads an epoch-seconds timestamp written by toEpoch() without a narrowing
// conversion: time_t is long (64-bit) on MinGW, long long is also 64-bit but
// the compiler cannot prove they match, so the cast must be explicit.
inline std::chrono::system_clock::time_point epochToTime(const nlohmann::json& value) {
    const long long seconds = value.get<long long>();
    return std::chrono::system_clock::from_time_t(static_cast<std::time_t>(seconds));
}

// Writes a time_point as epoch seconds (integer, no fractional part).
inline nlohmann::json timeToEpoch(const std::chrono::system_clock::time_point& tp) {
    const std::time_t seconds = std::chrono::system_clock::to_time_t(tp);
    return nlohmann::json(static_cast<long long>(seconds));
}

} // namespace db
