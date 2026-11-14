#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include "onrobot_control/command_codec.hpp"
#include "onrobot_msgs/msg/on_robot_rg_output.hpp"
#include "onrobot_msgs/srv/set_command.hpp"

class RgCommandServer final : public rclcpp::Node
{
public:
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

    publisher_ = create_publisher<onrobot_msgs::msg::OnRobotRGOutput>("OnRobotRGOutput", 1);
    service_ = create_service<onrobot_msgs::srv::SetCommand>(
      "onrobot_rg/set_command",
      [this](const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request> request,
             std::shared_ptr<onrobot_msgs::srv::SetCommand::Response> response) {
        handle_command(request, response);
      });
    RCLCPP_INFO(get_logger(), "Publishing RG command messages only; hardware transport is not enabled");
  }

private:
  void handle_command(
    const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request>& request,
    const std::shared_ptr<onrobot_msgs::srv::SetCommand::Response>& response)
  {
    const auto parsed = onrobot_control::parse_rg_command(
      request->command, { command_.r_gfr, command_.r_gwd, command_.r_ctr }, limits_);
    if (!parsed.valid())
    {
      response->success = false;
      response->message = parsed.error == onrobot_control::CommandError::out_of_range
        ? "Width exceeds model maximum of " + std::to_string(limits_.max_width)
        : "Use c, o, i, d, or a numeric gripper width";
      return;
    }

    command_.r_gfr = parsed.command->force;
    command_.r_gwd = parsed.command->width;
    command_.r_ctr = parsed.command->control;
    publisher_->publish(command_);
    response->success = true;
    response->message = "RG command published; no hardware write was performed";
  }

  onrobot_control::RgLimits limits_{ 0, 0 };
  onrobot_msgs::msg::OnRobotRGOutput command_;
  rclcpp::Publisher<onrobot_msgs::msg::OnRobotRGOutput>::SharedPtr publisher_;
  rclcpp::Service<onrobot_msgs::srv::SetCommand>::SharedPtr service_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RgCommandServer>());
  rclcpp::shutdown();
  return 0;
}