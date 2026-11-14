#include "onrobot_control/command_codec.hpp"

#include <iostream>

namespace
{

int failures = 0;

void expect(bool condition, const char* description)
{
  if (!condition) {
    std::cerr << "FAIL: " << description << '\n';
    ++failures;
  }
}

}  // namespace

int main()
{
  using onrobot_control::CommandError;
  using onrobot_control::RgCommand;
  using onrobot_control::VgCommand;

  const auto rg2 = onrobot_control::rg_limits_for_model("rg2");
  const auto rg6 = onrobot_control::rg_limits_for_model("rg6");
  expect(rg2 && rg2->max_force == 400 && rg2->max_width == 1100, "RG2 limits match the legacy controller");
  expect(rg6 && rg6->max_force == 1200 && rg6->max_width == 1600, "RG6 limits match the legacy controller");
  expect(!onrobot_control::rg_limits_for_model("unknown"), "unsupported RG model has no limits");

  const RgCommand empty_rg{};
  const auto close = onrobot_control::parse_rg_command("c", empty_rg, *rg2);
  expect(close.valid() && close.command->force == 400 && close.command->width == 0 && close.command->control == 16,
    "RG close command uses maximum force and zero width");
  const auto open = onrobot_control::parse_rg_command("o", empty_rg, *rg2);
  expect(open.valid() && open.command->width == 1100, "RG open command uses model maximum width");
  const auto width = onrobot_control::parse_rg_command("1100", empty_rg, *rg2);
  expect(width.valid() && width.command->width == 1100, "RG numeric maximum width is accepted");
  expect(onrobot_control::parse_rg_command("1101", empty_rg, *rg2).error == CommandError::out_of_range,
    "RG width beyond model limit is rejected");
  expect(onrobot_control::parse_rg_command("x", empty_rg, *rg2).error == CommandError::invalid_syntax,
    "unknown RG command is rejected");
  const auto increased = onrobot_control::parse_rg_command("i", {390, 500, 0}, *rg2);
  expect(increased.valid() && increased.command->force == 400, "RG force increment is capped at model limit");
  const auto decreased = onrobot_control::parse_rg_command("d", {10, 500, 0}, *rg2);
  expect(decreased.valid() && decreased.command->force == 0, "RG force decrement is floored at zero");

  const VgCommand empty_vg{};
  const auto grip = onrobot_control::parse_vg_command("g", empty_vg);
  expect(grip.valid() && grip.command->mode_a == 0x0100 && grip.command->vacuum_a == 80 &&
    grip.command->mode_b == 0x0100 && grip.command->vacuum_b == 80, "VG grip sets both channels to 80 percent");
  const auto one_channel = onrobot_control::parse_vg_command("ra", {0x0100, 60, 0x0100, 70});
  expect(one_channel.valid() && one_channel.command->mode_a == 0 && one_channel.command->vacuum_a == 0 &&
    one_channel.command->mode_b == 0x0100 && one_channel.command->vacuum_b == 70,
    "VG channel A release leaves channel B unchanged");
  const auto numeric = onrobot_control::parse_vg_command("80", empty_vg);
  expect(numeric.valid() && numeric.command->vacuum_a == 80 && numeric.command->vacuum_b == 80,
    "VG maximum vacuum is accepted");
  expect(onrobot_control::parse_vg_command("81", empty_vg).error == CommandError::out_of_range,
    "VG vacuum above documented limit is rejected");
  expect(onrobot_control::parse_vg_command("nope", empty_vg).error == CommandError::invalid_syntax,
    "unknown VG command is rejected");

  return failures == 0 ? 0 : 1;
}