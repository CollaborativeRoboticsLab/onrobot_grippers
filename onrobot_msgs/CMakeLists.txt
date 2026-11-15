cmake_minimum_required(VERSION 3.16)
project(onrobot_msgs)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RawRegisterSnapshot.msg"
  "msg/OnRobotRGInput.msg"
  "msg/OnRobotRGOutput.msg"
  "msg/OnRobotVGInput.msg"
  "msg/OnRobotVGOutput.msg"
  "action/VGCommand.action"
  "srv/SetCommand.srv"
  DEPENDENCIES std_msgs
)

ament_export_dependencies(rosidl_default_runtime)

ament_package()