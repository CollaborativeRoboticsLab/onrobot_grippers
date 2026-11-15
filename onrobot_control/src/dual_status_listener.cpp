#include <memory>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <sstream>
#include <string>

#include "onrobot_control/status_decoder.hpp"
#include "onrobot_msgs/msg/on_robot_rg_input.hpp"

class DualStatusListener final : public rclcpp::Node
{
public:
  DualStatusListener() : Node("onrobot_rg_dual_status_listener")
  {
    rcl_interfaces::msg::ParameterDescriptor joint_name_descriptor;
    joint_name_descriptor.description = "Name of the virtual prismatic RG aperture coordinate";
    joint_name_descriptor.read_only = true;
    primary_aperture_joint_name_ =
        declare_parameter<std::string>("primary_aperture_joint_name", "primary_rg_aperture", joint_name_descriptor);
    secondary_aperture_joint_name_ =
        declare_parameter<std::string>("secondary_aperture_joint_name", "secondary_rg_aperture", joint_name_descriptor);
    joint_state_publisher_ = create_publisher<sensor_msgs::msg::JointState>("rg_aperture/joint_states", 10);
    primary_subscription_ = create_subscription<onrobot_msgs::msg::OnRobotRGInput>(
        "OnRobotRGInput_A", 10, [this](onrobot_msgs::msg::OnRobotRGInput::ConstSharedPtr status) {
          report("primary", primary_aperture_joint_name_, *status);
        });
    secondary_subscription_ = create_subscription<onrobot_msgs::msg::OnRobotRGInput>(
        "OnRobotRGInput_B", 10, [this](onrobot_msgs::msg::OnRobotRGInput::ConstSharedPtr status) {
          report("secondary", secondary_aperture_joint_name_, *status);
        });
  }

private:
  void report(const char* side, const std::string& aperture_joint_name, const onrobot_msgs::msg::OnRobotRGInput& status)
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

    sensor_msgs::msg::JointState joint_state;
    joint_state.header.stamp = now();
    joint_state.name = { aperture_joint_name };
    joint_state.position = { static_cast<double>(decoded.width_with_offset_tenths_mm) * 1.0e-4 };
    joint_state_publisher_->publish(joint_state);
  }

  rclcpp::Subscription<onrobot_msgs::msg::OnRobotRGInput>::SharedPtr primary_subscription_;
  rclcpp::Subscription<onrobot_msgs::msg::OnRobotRGInput>::SharedPtr secondary_subscription_;
  std::string primary_aperture_joint_name_;
  std::string secondary_aperture_joint_name_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_publisher_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<DualStatusListener>());
  rclcpp::shutdown();
  return 0;
}