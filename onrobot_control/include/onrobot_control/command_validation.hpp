#pragma once

#include <cmath>
#include <optional>

namespace onrobot_control
{

struct GripperLimits
{
  double max_position_m;
  double max_effort_n;
};

struct GripperGoal
{
  double position_m;
  double max_effort_n;
};

enum class GoalValidationError
{
  none,
  invalid_limits,
  non_finite_position,
  position_out_of_range,
  non_finite_effort,
  effort_out_of_range
};

struct GoalValidationResult
{
  std::optional<GripperGoal> goal;
  GoalValidationError error;

  [[nodiscard]] bool valid() const noexcept
  {
    return error == GoalValidationError::none;
  }
};

[[nodiscard]] inline GoalValidationResult validate_goal(const GripperGoal& goal, const GripperLimits& limits) noexcept
{
  if (!std::isfinite(limits.max_position_m) || limits.max_position_m <= 0.0 || !std::isfinite(limits.max_effort_n) ||
      limits.max_effort_n <= 0.0)
  {
    return { std::nullopt, GoalValidationError::invalid_limits };
  }

  if (!std::isfinite(goal.position_m))
  {
    return { std::nullopt, GoalValidationError::non_finite_position };
  }
  if (goal.position_m < 0.0 || goal.position_m > limits.max_position_m)
  {
    return { std::nullopt, GoalValidationError::position_out_of_range };
  }

  if (!std::isfinite(goal.max_effort_n))
  {
    return { std::nullopt, GoalValidationError::non_finite_effort };
  }
  if (goal.max_effort_n <= 0.0 || goal.max_effort_n > limits.max_effort_n)
  {
    return { std::nullopt, GoalValidationError::effort_out_of_range };
  }

  return { goal, GoalValidationError::none };
}

}  // namespace onrobot_control