#pragma once

#include <cstdint>

namespace onrobot_control
{

struct RgStatus
{
  std::int16_t fingertip_offset_tenths_mm;
  std::uint16_t width_tenths_mm;
  std::uint16_t width_with_offset_tenths_mm;
  bool busy;
  bool grip_detected;
  bool safety_switch_1;
  bool safety_circuit_1;
  bool safety_switch_2;
  bool safety_circuit_2;
  bool safety_error;
};

[[nodiscard]] inline RgStatus decode_rg_status(std::uint16_t fingertip_offset, std::uint16_t width, std::uint8_t flags,
                                               std::uint16_t width_with_offset) noexcept
{
  const auto signed_offset = fingertip_offset <= 0x7fff ? static_cast<std::int32_t>(fingertip_offset) :
                                                          static_cast<std::int32_t>(fingertip_offset) - 0x10000;

  return { static_cast<std::int16_t>(signed_offset),
           width,
           width_with_offset,
           static_cast<bool>(flags & (1U << 0)),
           static_cast<bool>(flags & (1U << 1)),
           static_cast<bool>(flags & (1U << 2)),
           static_cast<bool>(flags & (1U << 3)),
           static_cast<bool>(flags & (1U << 4)),
           static_cast<bool>(flags & (1U << 5)),
           static_cast<bool>(flags & (1U << 6)) };
}

}  // namespace onrobot_control