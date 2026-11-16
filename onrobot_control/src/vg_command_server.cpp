#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

#include "onrobot_msgs/action/vg_command.hpp"

class VgCommandServer final : public rclcpp::Node
{
public:
  using Action = onrobot_msgs::action::VGCommand;
  using GoalHandle = rclcpp_action::ServerGoalHandle<Action>;

  VgCommandServer() : Node("onrobot_vg_command_server")
  {
    action_server_ = rclcpp_action::create_server<Action>(
        this, "gripper_command",
        [this](const rclcpp_action::GoalUUID&, std::shared_ptr<const Action::Goal> goal) { return handle_goal(goal); },
        [](const std::shared_ptr<GoalHandle>&) { return rclcpp_action::CancelResponse::REJECT; },
        [](const std::shared_ptr<GoalHandle>&) {});
    RCLCPP_WARN(get_logger(), "VG action is offline-only; goals are rejected until a verified execution backend "
                              "exists");
  }

private:
  rclcpp_action::GoalResponse handle_goal(const std::shared_ptr<const Action::Goal>& goal) const
  {
    if (goal->mode_a > 2 || goal->mode_b > 2 || goal->target_vacuum_a_percent > 80 ||
        goal->target_vacuum_b_percent > 80)
    {
      RCLCPP_WARN(get_logger(), "Rejecting VG goal outside supported mode or vacuum limits");
      return rclcpp_action::GoalResponse::REJECT;
    }

    RCLCPP_WARN(get_logger(), "Rejecting VG goal because no verified execution and feedback backend is configured");
    return rclcpp_action::GoalResponse::REJECT;
  }

  rclcpp_action::Server<Action>::SharedPtr action_server_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VgCommandServer>());
  rclcpp::shutdown();
  return 0;
}