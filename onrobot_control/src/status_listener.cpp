#include <memory>
#include <sstream>

#include <rclcpp/rclcpp.hpp>

#include "onrobot_control/status_decoder.hpp"
#include "onrobot_msgs/msg/on_robot_rg_input.hpp"
#include "onrobot_msgs/msg/on_robot_vg_input.hpp"

class StatusListener final : public rclcpp::Node
{
public:
  StatusListener() : Node("onrobot_status_listener")
  {
    rg_subscription_ = create_subscription<onrobot_msgs::msg::OnRobotRGInput>(
      "OnRobotRGInput", 10,
      [this](onrobot_msgs::msg::OnRobotRGInput::ConstSharedPtr status) { report_rg(*status); });
    vg_subscription_ = create_subscription<onrobot_msgs::msg::OnRobotVGInput>(
      "OnRobotVGInput", 10,
      [this](onrobot_msgs::msg::OnRobotVGInput::ConstSharedPtr status) { report_vg(*status); });
  }

private:
  void report_rg(const onrobot_msgs::msg::OnRobotRGInput& status)
  {
    const auto decoded = onrobot_control::decode_rg_status(
      status.g_fof, status.g_gwd, status.g_sta, status.g_wdf);
    std::ostringstream text;
    text << "RG width=" << decoded.width_tenths_mm / 10.0 << " mm, width_with_offset="
         << decoded.width_with_offset_tenths_mm / 10.0 << " mm, fingertip_offset="
         << decoded.fingertip_offset_tenths_mm / 10.0 << " mm"
         << ", busy=" << decoded.busy
         << ", grip_detected=" << decoded.grip_detected
         << ", safety_switch_1=" << decoded.safety_switch_1
         << ", safety_circuit_1=" << decoded.safety_circuit_1
         << ", safety_switch_2=" << decoded.safety_switch_2
         << ", safety_circuit_2=" << decoded.safety_circuit_2
         << ", safety_error=" << decoded.safety_error;
    RCLCPP_INFO(get_logger(), "%s", text.str().c_str());
  }

  void report_vg(const onrobot_msgs::msg::OnRobotVGInput& status)
  {
    RCLCPP_INFO(get_logger(), "VG vacuum: channel A=%u, channel B=%u", status.g_vca, status.g_vcb);
  }

  rclcpp::Subscription<onrobot_msgs::msg::OnRobotRGInput>::SharedPtr rg_subscription_;
  rclcpp::Subscription<onrobot_msgs::msg::OnRobotVGInput>::SharedPtr vg_subscription_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<StatusListener>());
  rclcpp::shutdown();
  return 0;
}