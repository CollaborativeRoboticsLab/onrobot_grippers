#include <charconv>
#include <memory>
#include <string>
#include <system_error>

#include <rclcpp/rclcpp.hpp>

#include "onrobot_msgs/msg/on_robot_vg_output.hpp"
#include "onrobot_msgs/srv/set_command.hpp"

class VgCommandServer final : public rclcpp::Node
{
public:
  VgCommandServer() : Node("onrobot_vg_command_server")
  {
    publisher_ = create_publisher<onrobot_msgs::msg::OnRobotVGOutput>("OnRobotVGOutput", 1);
    service_ = create_service<onrobot_msgs::srv::SetCommand>(
      "onrobot_vg/set_command",
      [this](const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request> request,
             std::shared_ptr<onrobot_msgs::srv::SetCommand::Response> response) {
        handle_command(request, response);
      });
    RCLCPP_INFO(get_logger(), "Publishing VG command messages only; hardware transport is not enabled");
  }

private:
  void handle_command(
    const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request>& request,
    const std::shared_ptr<onrobot_msgs::srv::SetCommand::Response>& response)
  {
    auto next = command_;
    const auto& text = request->command;
    if (text == "g")
    {
      set_channel(next.r_mca, next.r_vca, true, 80);
      set_channel(next.r_mcb, next.r_vcb, true, 80);
    }
    else if (text == "r")
    {
      set_channel(next.r_mca, next.r_vca, false, 0);
      set_channel(next.r_mcb, next.r_vcb, false, 0);
    }
    else if (text == "ga" || text == "ra")
    {
      const bool grip = text == "ga";
      set_channel(next.r_mca, next.r_vca, grip, grip ? 80 : 0);
    }
    else if (text == "gb" || text == "rb")
    {
      const bool grip = text == "gb";
      set_channel(next.r_mcb, next.r_vcb, grip, grip ? 80 : 0);
    }
    else
    {
      unsigned int vacuum = 0;
      const auto result = std::from_chars(text.data(), text.data() + text.size(), vacuum);
      if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size() || vacuum > 80)
      {
        response->success = false;
        response->message = "Use g, r, ga, ra, gb, rb, or a vacuum value from 0 to 80";
        return;
      }
      const bool grip = vacuum != 0;
      set_channel(next.r_mca, next.r_vca, grip, vacuum);
      set_channel(next.r_mcb, next.r_vcb, grip, vacuum);
    }

    command_ = next;
    publisher_->publish(command_);
    response->success = true;
    response->message = "VG command published; no hardware write was performed";
  }

  static void set_channel(std::uint16_t& mode, std::uint16_t& vacuum, bool grip, std::uint16_t value)
  {
    mode = grip ? 0x0100 : 0x0000;
    vacuum = value;
  }

  onrobot_msgs::msg::OnRobotVGOutput command_;
  rclcpp::Publisher<onrobot_msgs::msg::OnRobotVGOutput>::SharedPtr publisher_;
  rclcpp::Service<onrobot_msgs::srv::SetCommand>::SharedPtr service_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VgCommandServer>());
  rclcpp::shutdown();
  return 0;
}