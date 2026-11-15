#include <control_msgs/action/gripper_command.hpp>
#include <memory>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <stdexcept>
#include <string>

#include "onrobot_control/gripper_goal_validation.hpp"

class RgDualCommandServer final : public rclcpp::Node
{
public:
  using Action = control_msgs::action::GripperCommand;
  using GoalHandle = rclcpp_action::ServerGoalHandle<Action>;

  RgDualCommandServer() : Node("onrobot_rg_dual_command_server")
  {
    primary_limits_ = declare_limits("gripper_primary", "rg2");
    secondary_limits_ = declare_limits("gripper_secondary", "rg6");
    primary_action_server_ = create_action_server("onrobot_rg/gripper_command_A", primary_limits_, "primary");
    secondary_action_server_ = create_action_server("onrobot_rg/gripper_command_B", secondary_limits_, "secondary");
    RCLCPP_WARN(get_logger(), "Dual RG actions are offline-only; goals are rejected until a verified execution and "
                              "feedback backend exists");
  }

private:
  onrobot_control::RgLimits declare_limits(const std::string& parameter, const std::string& default_model)
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.description = "Gripper model; change requires restarting the node";
    descriptor.read_only = true;
    const auto model = declare_parameter<std::string>(parameter, default_model, descriptor);
    const auto limits = onrobot_control::rg_limits_for_model(model);
    if (!limits)
    {
      throw std::invalid_argument(parameter + " must be 'rg2' or 'rg6'");
    }
    return *limits;
  }

  rclcpp_action::Server<Action>::SharedPtr create_action_server(const std::string& action_name,
                                                                const onrobot_control::RgLimits& limits,
                                                                const std::string& side)
  {
    return rclcpp_action::create_server<Action>(
        this, action_name,
        [this, limits, side](const rclcpp_action::GoalUUID&, std::shared_ptr<const Action::Goal> goal) {
          if (!onrobot_control::rg_gripper_goal_within_limits(goal->command.position, goal->command.max_effort, limits))
          {
            RCLCPP_WARN(get_logger(), "Rejecting %s RG goal outside configured position or effort limits",
                        side.c_str());
            return rclcpp_action::GoalResponse::REJECT;
          }

          RCLCPP_WARN(get_logger(),
                      "Rejecting %s RG goal because no verified execution and feedback backend is "
                      "configured",
                      side.c_str());
          return rclcpp_action::GoalResponse::REJECT;
        },
        [](const std::shared_ptr<GoalHandle>&) { return rclcpp_action::CancelResponse::REJECT; },
        [](const std::shared_ptr<GoalHandle>&) {});
  }

  onrobot_control::RgLimits primary_limits_{ 0, 0 };
  onrobot_control::RgLimits secondary_limits_{ 0, 0 };
  rclcpp_action::Server<Action>::SharedPtr primary_action_server_;
  rclcpp_action::Server<Action>::SharedPtr secondary_action_server_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RgDualCommandServer>());
  rclcpp::shutdown();
  return 0;
}