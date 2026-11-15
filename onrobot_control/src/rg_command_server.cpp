#include <control_msgs/action/gripper_command.hpp>
#include <cstdint>
#include <memory>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <stdexcept>
#include <string>

#include "onrobot_control/gripper_goal_validation.hpp"

class RgCommandServer final : public rclcpp::Node
{
public:
  using Action = control_msgs::action::GripperCommand;
  using GoalHandle = rclcpp_action::ServerGoalHandle<Action>;

  RgCommandServer() : Node("onrobot_rg_command_server")
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.description = "Gripper model; change requires restarting the node";
    descriptor.read_only = true;
    const auto model = declare_parameter<std::string>("gripper_model", "rg6", descriptor);
    const auto limits = onrobot_control::rg_limits_for_model(model);
    if (!limits)
    {
      throw std::invalid_argument("gripper_model must be 'rg2' or 'rg6'");
    }
    limits_ = *limits;

    action_server_ = rclcpp_action::create_server<Action>(
        this, "onrobot_rg/gripper_command",
        [this](const rclcpp_action::GoalUUID&, std::shared_ptr<const Action::Goal> goal) { return handle_goal(goal); },
        [](const std::shared_ptr<GoalHandle>&) { return rclcpp_action::CancelResponse::REJECT; },
        [](const std::shared_ptr<GoalHandle>&) {});
    RCLCPP_WARN(get_logger(), "RG action is offline-only; goals are rejected until a verified execution backend "
                              "exists");
  }

private:
  rclcpp_action::GoalResponse handle_goal(const std::shared_ptr<const Action::Goal>& goal) const
  {
    if (!onrobot_control::rg_gripper_goal_within_limits(goal->command.position, goal->command.max_effort, limits_))
    {
      RCLCPP_WARN(get_logger(), "Rejecting RG goal outside the configured position or effort limits");
      return rclcpp_action::GoalResponse::REJECT;
    }

    RCLCPP_WARN(get_logger(), "Rejecting RG goal because no verified execution and feedback backend is configured");
    return rclcpp_action::GoalResponse::REJECT;
  }

  onrobot_control::RgLimits limits_{ 0, 0 };
  rclcpp_action::Server<Action>::SharedPtr action_server_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RgCommandServer>());
  rclcpp::shutdown();
  return 0;
}