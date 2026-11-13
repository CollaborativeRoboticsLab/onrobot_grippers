#include <charconv>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include <rclcpp/rclcpp.hpp>

#include "onrobot_msgs/msg/on_robot_rg_output.hpp"
#include "onrobot_msgs/srv/set_command.hpp"

class RgCommandServer final : public rclcpp::Node
{
public:
  RgCommandServer() : Node("onrobot_rg_command_server")
  {
    const auto model = declare_parameter<std::string>("gripper_model", "rg6");
    if (model == "rg2")
    {
      max_force_ = 400;
      max_width_ = 1100;
    }
    else if (model == "rg6")
    {
      max_force_ = 1200;
      max_width_ = 1600;
    }
    else
    {
      throw std::invalid_argument("gripper_model must be 'rg2' or 'rg6'");
    }

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
    auto next = command_;
    const auto& text = request->command;
    if (text == "c")
    {
      next.r_gfr = max_force_;
      next.r_gwd = 0;
      next.r_ctr = 0x10;
    }
    else if (text == "o")
    {
      next.r_gfr = max_force_;
      next.r_gwd = max_width_;
      next.r_ctr = 0x10;
    }
    else if (text == "i" || text == "d")
    {
      const auto force = static_cast<int>(next.r_gfr) + (text == "i" ? 25 : -25);
      next.r_gfr = static_cast<std::uint16_t>(std::clamp(force, 0, static_cast<int>(max_force_)));
      next.r_ctr = 0x10;
    }
    else
    {
      unsigned int width = 0;
      const auto result = std::from_chars(text.data(), text.data() + text.size(), width);
      if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size() || width > max_width_)
      {
        response->success = false;
        response->message = "Use c, o, i, d, or a width from 0 to " + std::to_string(max_width_);
        return;
      }
      next.r_gfr = max_force_;
      next.r_gwd = static_cast<std::uint16_t>(width);
      next.r_ctr = 0x10;
    }

    command_ = next;
    publisher_->publish(command_);
    response->success = true;
    response->message = "RG command published; no hardware write was performed";
  }

  std::uint16_t max_force_{ 0 };
  std::uint16_t max_width_{ 0 };
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