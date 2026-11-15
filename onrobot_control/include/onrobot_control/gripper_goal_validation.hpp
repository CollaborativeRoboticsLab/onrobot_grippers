#pragma once

#include <cmath>

#include "onrobot_control/command_codec.hpp"

namespace onrobot_control
{

[[nodiscard]] inline bool rg_gripper_goal_within_limits(double position_m, double max_effort_n,
                                                        RgLimits limits) noexcept
{
  return std::isfinite(position_m) && std::isfinite(max_effort_n) && position_m >= 0.0 &&
         position_m <= static_cast<double>(limits.max_width) * 1.0e-4 && max_effort_n >= 0.0 &&
         max_effort_n <= static_cast<double>(limits.max_force) * 0.1;
}

}  // namespace onrobot_control