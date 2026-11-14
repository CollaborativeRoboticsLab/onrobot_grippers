#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <string>

#include "onrobot_control/command_codec.hpp"
#include "onrobot_msgs/msg/on_robot_vg_output.hpp"
#include "onrobot_msgs/srv/set_command.hpp"

class VgCommandServer final : public rclcpp::Node
{
public:
  VgCommandServer() : Node("onrobot_vg_command_server")
  {
    publisher_ = create_publisher<onrobot_msgs::msg::OnRobotVGOutput>("OnRobotVGOutput", 1);
    service_ = create_service<onrobot_msgs::srv::SetCommand>(
        "onrobot_vg/set_command", [this](const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request> request,
                                         std::shared_ptr<onrobot_msgs::srv::SetCommand::Response> response) {
          handle_command(request, response);
        });
    RCLCPP_INFO(get_logger(), "Publishing VG command messages only; hardware transport is not enabled");
  }

private:
  void handle_command(const std::shared_ptr<onrobot_msgs::srv::SetCommand::Request>& request,
                      const std::shared_ptr<onrobot_msgs::srv::SetCommand::Response>& response)
  {
    const auto parsed = onrobot_control::parse_vg_command(
        request->command, { command_.r_mca, command_.r_vca, command_.r_mcb, command_.r_vcb });
    if (!parsed.valid())
    {
      response->success = false;
      response->message = parsed.error == onrobot_control::CommandError::out_of_range ? "Vacuum must be between 0 and "
                                                                                        "80" :
                                                                                        "Use g, r, ga, ra, gb, rb, or "
                                                                                        "a numeric vacuum value";
      return;
    }

    command_.r_mca = parsed.command->mode_a;
    command_.r_vca = parsed.command->vacuum_a;
    command_.r_mcb = parsed.command->mode_b;
    command_.r_vcb = parsed.command->vacuum_b;
    publisher_->publish(command_);
    response->success = true;
    response->message = "VG command published; no hardware write was performed";
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