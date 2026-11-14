#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <sstream>

#include "onrobot_control/status_decoder.hpp"
#include "onrobot_msgs/msg/on_robot_rg_input.hpp"

class DualStatusListener final : public rclcpp::Node
{
public:
  DualStatusListener() : Node("onrobot_rg_dual_status_listener")
  {
    primary_subscription_ = create_subscription<onrobot_msgs::msg::OnRobotRGInput>(
        "OnRobotRGInput_A", 10,
        [this](onrobot_msgs::msg::OnRobotRGInput::ConstSharedPtr status) { report("primary", *status); });
    secondary_subscription_ = create_subscription<onrobot_msgs::msg::OnRobotRGInput>(
        "OnRobotRGInput_B", 10,
        [this](onrobot_msgs::msg::OnRobotRGInput::ConstSharedPtr status) { report("secondary", *status); });
  }

private:
  void report(const char* side, const onrobot_msgs::msg::OnRobotRGInput& status)
  {
    const auto decoded = onrobot_control::decode_rg_status(status.g_fof, status.g_gwd, status.g_sta, status.g_wdf);
    std::ostringstream text;
    text << side << " RG: width=" << decoded.width_tenths_mm / 10.0
         << " mm, width_with_offset=" << decoded.width_with_offset_tenths_mm / 10.0
         << " mm, fingertip_offset=" << decoded.fingertip_offset_tenths_mm / 10.0 << " mm, busy=" << decoded.busy
         << ", grip_detected=" << decoded.grip_detected << ", safety_switch_1=" << decoded.safety_switch_1
         << ", safety_circuit_1=" << decoded.safety_circuit_1 << ", safety_switch_2=" << decoded.safety_switch_2
         << ", safety_circuit_2=" << decoded.safety_circuit_2 << ", safety_error=" << decoded.safety_error;
    RCLCPP_INFO(get_logger(), "%s", text.str().c_str());
  }

  rclcpp::Subscription<onrobot_msgs::msg::OnRobotRGInput>::SharedPtr primary_subscription_;
  rclcpp::Subscription<onrobot_msgs::msg::OnRobotRGInput>::SharedPtr secondary_subscription_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DualStatusListener>());
  rclcpp::shutdown();
  return 0;
}