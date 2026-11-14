#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>

namespace onrobot_control
{

struct RgLimits
{
  std::uint16_t max_force;
  std::uint16_t max_width;
};

struct RgCommand
{
  std::uint16_t force{ 0 };
  std::uint16_t width{ 0 };
  std::uint8_t control{ 0 };
};

struct VgCommand
{
  std::uint16_t mode_a{ 0 };
  std::uint16_t vacuum_a{ 0 };
  std::uint16_t mode_b{ 0 };
  std::uint16_t vacuum_b{ 0 };
};

enum class CommandError
{
  none,
  invalid_syntax,
  out_of_range
};

template<typename Command>
struct CommandParseResult
{
  std::optional<Command> command;
  CommandError error;

  [[nodiscard]] bool valid() const noexcept
  {
    return command.has_value();
  }
};

[[nodiscard]] inline std::optional<RgLimits> rg_limits_for_model(std::string_view model) noexcept
{
  if (model == "rg2") {
    return RgLimits{ 400, 1100 };
  }
  if (model == "rg6") {
    return RgLimits{ 1200, 1600 };
  }
  return std::nullopt;
}

[[nodiscard]] inline CommandParseResult<RgCommand> parse_rg_command(
  std::string_view text, const RgCommand& current, RgLimits limits)
{
  auto next = current;
  if (text == "c") {
    next.force = limits.max_force;
    next.width = 0;
    next.control = 0x10;
  } else if (text == "o") {
    next.force = limits.max_force;
    next.width = limits.max_width;
    next.control = 0x10;
  } else if (text == "i" || text == "d") {
    const auto force = static_cast<int>(next.force) + (text == "i" ? 25 : -25);
    next.force = static_cast<std::uint16_t>(std::clamp(force, 0, static_cast<int>(limits.max_force)));
    next.control = 0x10;
  } else {
    unsigned int width = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), width);
    if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
      return { std::nullopt, CommandError::invalid_syntax };
    }
    if (width > limits.max_width) {
      return { std::nullopt, CommandError::out_of_range };
    }
    next.force = limits.max_force;
    next.width = static_cast<std::uint16_t>(width);
    next.control = 0x10;
  }
  return { next, CommandError::none };
}

[[nodiscard]] inline CommandParseResult<VgCommand> parse_vg_command(
  std::string_view text, const VgCommand& current)
{
  auto next = current;
  if (text == "g") {
    next.mode_a = next.mode_b = 0x0100;
    next.vacuum_a = next.vacuum_b = 80;
  } else if (text == "r") {
    next.mode_a = next.mode_b = 0;
    next.vacuum_a = next.vacuum_b = 0;
  } else if (text == "ga" || text == "ra") {
    next.mode_a = text == "ga" ? 0x0100 : 0;
    next.vacuum_a = text == "ga" ? 80 : 0;
  } else if (text == "gb" || text == "rb") {
    next.mode_b = text == "gb" ? 0x0100 : 0;
    next.vacuum_b = text == "gb" ? 80 : 0;
  } else {
    unsigned int vacuum = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), vacuum);
    if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
      return { std::nullopt, CommandError::invalid_syntax };
    }
    if (vacuum > 80) {
      return { std::nullopt, CommandError::out_of_range };
    }
    const auto mode = vacuum == 0 ? 0 : 0x0100;
    next.mode_a = next.mode_b = static_cast<std::uint16_t>(mode);
    next.vacuum_a = next.vacuum_b = static_cast<std::uint16_t>(vacuum);
  }
  return { next, CommandError::none };
}

}  // namespace onrobot_control