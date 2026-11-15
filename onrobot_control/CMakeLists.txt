cmake_minimum_required(VERSION 3.16)
project(onrobot_control LANGUAGES CXX)

find_package(ament_cmake REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(control_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(onrobot_msgs REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED IMPORTED_TARGET libmodbus)

include(CTest)
include(GNUInstallDirs)

add_library(onrobot_control_core INTERFACE)
target_include_directories(onrobot_control_core
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(onrobot_control_core INTERFACE cxx_std_17)
target_link_libraries(onrobot_control_core INTERFACE modbus)

add_executable(onrobot_control_node src/control.cpp)
target_link_libraries(onrobot_control_node onrobot_control_core)
ament_target_dependencies(onrobot_control_node rclcpp)

add_library(onrobot_read_only_status_adapter src/read_only_status_adapter.cpp)
target_link_libraries(onrobot_read_only_status_adapter onrobot_control_core Threads::Threads)
ament_target_dependencies(onrobot_read_only_status_adapter
  rclcpp rcl_interfaces onrobot_msgs diagnostic_msgs)

add_executable(onrobot_read_only_status_adapter_node src/read_only_status_adapter_main.cpp)
set_target_properties(onrobot_read_only_status_adapter_node PROPERTIES
  OUTPUT_NAME onrobot_read_only_status_adapter)
target_link_libraries(onrobot_read_only_status_adapter_node onrobot_read_only_status_adapter)
ament_target_dependencies(onrobot_read_only_status_adapter_node rclcpp)

add_executable(onrobot_rg_command_server src/rg_command_server.cpp)
target_link_libraries(onrobot_rg_command_server onrobot_control_core)
ament_target_dependencies(onrobot_rg_command_server rclcpp rclcpp_action rcl_interfaces control_msgs)

add_executable(onrobot_simulated_rg_action_server src/simulated_rg_action_server.cpp)
target_link_libraries(onrobot_simulated_rg_action_server onrobot_control_core)
ament_target_dependencies(onrobot_simulated_rg_action_server rclcpp rclcpp_action control_msgs sensor_msgs std_msgs)

add_executable(onrobot_vg_command_server src/vg_command_server.cpp)
target_link_libraries(onrobot_vg_command_server onrobot_control_core)
ament_target_dependencies(onrobot_vg_command_server rclcpp rclcpp_action onrobot_msgs)

add_executable(onrobot_status_listener src/status_listener.cpp)
target_link_libraries(onrobot_status_listener onrobot_control_core)
ament_target_dependencies(onrobot_status_listener rclcpp onrobot_msgs sensor_msgs)

add_executable(onrobot_rg_dual_command_server src/rg_dual_command_server.cpp)
target_link_libraries(onrobot_rg_dual_command_server onrobot_control_core)
ament_target_dependencies(onrobot_rg_dual_command_server rclcpp rclcpp_action rcl_interfaces control_msgs)

add_executable(onrobot_dual_status_listener src/dual_status_listener.cpp)
target_link_libraries(onrobot_dual_status_listener onrobot_control_core)
ament_target_dependencies(onrobot_dual_status_listener rclcpp onrobot_msgs sensor_msgs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES LICENSE DESTINATION share/${PROJECT_NAME})
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../onrobot_ros/config/"
  DESTINATION share/${PROJECT_NAME}/config
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../onrobot_ros/launch/"
  DESTINATION share/${PROJECT_NAME}/launch
)
install(TARGETS onrobot_control_core EXPORT export_onrobot_control)
install(TARGETS
  onrobot_control_node
  onrobot_read_only_status_adapter
  onrobot_read_only_status_adapter_node
  onrobot_rg_command_server
  onrobot_simulated_rg_action_server
  onrobot_vg_command_server
  onrobot_status_listener
  onrobot_rg_dual_command_server
  onrobot_dual_status_listener
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME control_api_and_joint_state
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/rg_model_parameters_test.py
      $<TARGET_FILE:onrobot_rg_command_server> $<TARGET_FILE:onrobot_rg_dual_command_server>
      $<TARGET_FILE:onrobot_vg_command_server> $<TARGET_FILE:onrobot_status_listener>
      $<TARGET_FILE:onrobot_dual_status_listener> $<TARGET_FILE:onrobot_simulated_rg_action_server>)
  set_tests_properties(control_api_and_joint_state PROPERTIES TIMEOUT 90)

  add_executable(test_command_validation test/command_validation_test.cpp)
  target_link_libraries(test_command_validation PRIVATE onrobot_control_core)
  target_compile_options(test_command_validation PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME command_validation COMMAND test_command_validation)

  add_executable(test_command_codec test/command_codec_test.cpp)
  target_link_libraries(test_command_codec PRIVATE onrobot_control_core)
  target_compile_options(test_command_codec PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME command_codec COMMAND test_command_codec)

  add_executable(test_status_decoder test/status_decoder_test.cpp)
  target_link_libraries(test_status_decoder PRIVATE onrobot_control_core)
  target_compile_options(test_status_decoder PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME status_decoder COMMAND test_status_decoder)

  add_executable(test_modbus_tcp_transport test/modbus_tcp_transport_test.cpp)
  target_link_libraries(test_modbus_tcp_transport PRIVATE onrobot_control_core)
  target_compile_options(test_modbus_tcp_transport PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME modbus_tcp_transport COMMAND test_modbus_tcp_transport)

  add_executable(test_read_only_register_poller test/read_only_register_poller_test.cpp)
  target_link_libraries(test_read_only_register_poller PRIVATE onrobot_control_core Threads::Threads)
  target_compile_options(test_read_only_register_poller PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME read_only_register_poller COMMAND test_read_only_register_poller)
  set_tests_properties(read_only_register_poller PROPERTIES TIMEOUT 5)

  add_executable(test_read_only_status_adapter test/read_only_status_adapter_test.cpp)
  target_link_libraries(test_read_only_status_adapter
    onrobot_read_only_status_adapter Threads::Threads)
  ament_target_dependencies(test_read_only_status_adapter
    rclcpp onrobot_msgs diagnostic_msgs)
  target_compile_options(test_read_only_status_adapter PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME read_only_status_adapter COMMAND test_read_only_status_adapter)
  set_tests_properties(read_only_status_adapter PROPERTIES TIMEOUT 10)
endif()

ament_export_include_directories(include)
ament_export_libraries(modbus)
ament_export_targets(export_onrobot_control HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp rcl_interfaces onrobot_msgs diagnostic_msgs)

ament_package()