#include "onrobot_control/read_only_status_adapter.hpp"

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<onrobot_control::ReadOnlyStatusAdapter>());
  rclcpp::shutdown();
  return 0;
}