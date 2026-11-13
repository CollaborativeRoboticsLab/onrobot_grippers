#include <cstdint>
#include <memory>
#include <sstream>

#include <rclcpp/rclcpp.hpp>

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
    const auto flags = status.g_sta;
    std::ostringstream text;
    text << "RG width=" << status.g_gwd / 10.0 << " mm, width_with_offset=" << status.g_wdf / 10.0
         << " mm, fingertip_offset=" << static_cast<std::int16_t>(status.g_fof) / 10.0 << " mm"
         << ", busy=" << static_cast<bool>(flags & (1U << 0))
         << ", grip_detected=" << static_cast<bool>(flags & (1U << 1))
         << ", safety_switch_1=" << static_cast<bool>(flags & (1U << 2))
         << ", safety_circuit_1=" << static_cast<bool>(flags & (1U << 3))
         << ", safety_switch_2=" << static_cast<bool>(flags & (1U << 4))
         << ", safety_circuit_2=" << static_cast<bool>(flags & (1U << 5))
         << ", safety_error=" << static_cast<bool>(flags & (1U << 6));
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