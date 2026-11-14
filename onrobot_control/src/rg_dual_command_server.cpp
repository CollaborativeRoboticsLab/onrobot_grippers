#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <stdexcept>
#include <string>

#include "onrobot_control/command_codec.hpp"
#include "onrobot_msgs/msg/on_robot_rg_output.hpp"
#include "onrobot_msgs/srv/set_command.hpp"

class RgDualCommandServer final : public rclcpp::Node
{
public:
  RgDualCommandServer() : Node("onrobot_rg_dual_command_server")
  {
    primary_limits_ = declare_limits("gripper_primary", "rg2");
    secondary_limits_ = declare_limits("gripper_secondary", "rg6");
    primary_publisher_ = create_publisher<onrobot_msgs::msg::OnRobotRGOutput>("OnRobotRGOutput_A", 1);
    secondary_publisher_ = create_publisher<onrobot_msgs::msg::OnRobotRGOutput>("OnRobotRGOutput_B", 1);

    primary_service_ = create_service<onrobot_msgs::srv::SetCommand>(
        "onrobot_rg/set_command_A", [this](const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request> request,
                                           std::shared_ptr<onrobot_msgs::srv::SetCommand::Response> response) {
          handle_command(request->command, primary_command_, primary_limits_, primary_publisher_, "primary", response);
        });
    secondary_service_ = create_service<onrobot_msgs::srv::SetCommand>(
        "onrobot_rg/set_command_B", [this](const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request> request,
                                           std::shared_ptr<onrobot_msgs::srv::SetCommand::Response> response) {
          handle_command(request->command, secondary_command_, secondary_limits_, secondary_publisher_, "secondary",
                         response);
        });
    RCLCPP_INFO(get_logger(), "Publishing dual RG command messages only; hardware transport is not enabled");
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

  void handle_command(const std::string& text, onrobot_msgs::msg::OnRobotRGOutput& current,
                      const onrobot_control::RgLimits& limits,
                      const rclcpp::Publisher<onrobot_msgs::msg::OnRobotRGOutput>::SharedPtr& publisher,
                      const char* side, const std::shared_ptr<onrobot_msgs::srv::SetCommand::Response>& response)
  {
    const auto parsed =
        onrobot_control::parse_rg_command(text, { current.r_gfr, current.r_gwd, current.r_ctr }, limits);
    if (!parsed.valid())
    {
      response->success = false;
      response->message =
          parsed.error == onrobot_control::CommandError::out_of_range ?
              std::string(side) + " gripper width exceeds model maximum of " + std::to_string(limits.max_width) :
              std::string("Invalid ") + side + " gripper command; use c, o, i, d, or a numeric width";
      return;
    }

    current.r_gfr = parsed.command->force;
    current.r_gwd = parsed.command->width;
    current.r_ctr = parsed.command->control;
    publisher->publish(current);
    response->success = true;
    response->message = std::string(side) + " RG command published; no hardware write was performed";
  }

  onrobot_control::RgLimits primary_limits_{ 0, 0 };
  onrobot_control::RgLimits secondary_limits_{ 0, 0 };
  onrobot_msgs::msg::OnRobotRGOutput primary_command_;
  onrobot_msgs::msg::OnRobotRGOutput secondary_command_;
  rclcpp::Publisher<onrobot_msgs::msg::OnRobotRGOutput>::SharedPtr primary_publisher_;
  rclcpp::Publisher<onrobot_msgs::msg::OnRobotRGOutput>::SharedPtr secondary_publisher_;
  rclcpp::Service<onrobot_msgs::srv::SetCommand>::SharedPtr primary_service_;
  rclcpp::Service<onrobot_msgs::srv::SetCommand>::SharedPtr secondary_service_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RgDualCommandServer>());
  rclcpp::shutdown();
  return 0;
}