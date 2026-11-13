#include <rclcpp/rclcpp.hpp>
#include <string>

#include "onrobot_control/command_validation.hpp"

class ControlNode final : public rclcpp::Node
{
public:
  ControlNode() : Node("onrobot_control")
  {
    const auto model = declare_parameter<std::string>("model", "rg2");
    RCLCPP_WARN(get_logger(), "Model '%s' selected; hardware transport is not enabled", model.c_str());
  }
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ControlNode>());
  rclcpp::shutdown();
  return 0;
}