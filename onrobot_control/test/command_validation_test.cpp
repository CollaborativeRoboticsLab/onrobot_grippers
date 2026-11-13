#include "onrobot_control/command_validation.hpp"

#include <iostream>
#include <limits>

namespace
{

int failures = 0;

void expect(bool condition, const char * description)
{
  if (!condition) {
    std::cerr << "FAIL: " << description << '\n';
    ++failures;
  }
}

void expect_valid(
  const onrobot_control::GripperGoal & goal,
  const onrobot_control::GripperLimits & limits,
  const char * description)
{
  const auto result = onrobot_control::validate_goal(goal, limits);
  expect(result.valid(), description);
  expect(result.goal.has_value(), "valid result contains the original goal");
  if (result.goal) {
    expect(result.goal->position_m == goal.position_m, "position is unchanged");
    expect(result.goal->max_effort_n == goal.max_effort_n, "effort is unchanged");
  }
}

void expect_error(
  const onrobot_control::GripperGoal & goal,
  const onrobot_control::GripperLimits & limits,
  onrobot_control::GoalValidationError expected_error,
  const char * description)
{
  const auto result = onrobot_control::validate_goal(goal, limits);
  expect(!result.valid(), description);
  expect(result.error == expected_error, "validation returns the expected error");
  expect(!result.goal.has_value(), "invalid result contains no goal");
}

}  // namespace

int main()
{
  using onrobot_control::GoalValidationError;
  using onrobot_control::GripperGoal;
  using onrobot_control::GripperLimits;

  const GripperLimits limits{0.1, 100.0};
  expect_valid({0.0, 1.0}, limits, "minimum position and positive effort are valid");
  expect_valid({0.1, 100.0}, limits, "maximum position and effort are inclusive");

  expect_error({-0.001, 10.0}, limits, GoalValidationError::position_out_of_range,
    "negative position is rejected");
  expect_error({0.101, 10.0}, limits, GoalValidationError::position_out_of_range,
    "position above the limit is rejected");
  expect_error({std::numeric_limits<double>::quiet_NaN(), 10.0}, limits,
    GoalValidationError::non_finite_position, "NaN position is rejected");
  expect_error({0.05, std::numeric_limits<double>::infinity()}, limits,
    GoalValidationError::non_finite_effort, "infinite effort is rejected");
  expect_error({0.05, std::numeric_limits<double>::quiet_NaN()}, limits,
    GoalValidationError::non_finite_effort, "NaN effort is rejected");
  expect_error({0.05, 0.0}, limits, GoalValidationError::effort_out_of_range,
    "zero effort is rejected");
  expect_error({0.05, 100.1}, limits, GoalValidationError::effort_out_of_range,
    "effort above the limit is rejected");
  expect_error({0.05, 10.0}, {0.0, 100.0}, GoalValidationError::invalid_limits,
    "zero position limit is rejected");
  expect_error({0.05, 10.0}, {0.1, std::numeric_limits<double>::infinity()},
    GoalValidationError::invalid_limits, "non-finite effort limit is rejected");

  return failures == 0 ? 0 : 1;
}