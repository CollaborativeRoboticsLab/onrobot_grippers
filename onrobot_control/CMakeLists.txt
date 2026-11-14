cmake_minimum_required(VERSION 3.16)
project(onrobot_control LANGUAGES CXX)

find_package(ament_cmake REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(onrobot_msgs REQUIRED)
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

add_executable(onrobot_rg_command_server src/rg_command_server.cpp)
target_link_libraries(onrobot_rg_command_server onrobot_control_core)
ament_target_dependencies(onrobot_rg_command_server rclcpp rcl_interfaces onrobot_msgs)

add_executable(onrobot_vg_command_server src/vg_command_server.cpp)
target_link_libraries(onrobot_vg_command_server onrobot_control_core)
ament_target_dependencies(onrobot_vg_command_server rclcpp onrobot_msgs)

add_executable(onrobot_status_listener src/status_listener.cpp)
target_link_libraries(onrobot_status_listener onrobot_control_core)
ament_target_dependencies(onrobot_status_listener rclcpp onrobot_msgs)

add_executable(onrobot_rg_dual_command_server src/rg_dual_command_server.cpp)
target_link_libraries(onrobot_rg_dual_command_server onrobot_control_core)
ament_target_dependencies(onrobot_rg_dual_command_server rclcpp rcl_interfaces onrobot_msgs)

add_executable(onrobot_dual_status_listener src/dual_status_listener.cpp)
target_link_libraries(onrobot_dual_status_listener onrobot_control_core)
ament_target_dependencies(onrobot_dual_status_listener rclcpp onrobot_msgs)

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
  onrobot_rg_command_server
  onrobot_vg_command_server
  onrobot_status_listener
  onrobot_rg_dual_command_server
  onrobot_dual_status_listener
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME rg_model_parameters
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/rg_model_parameters_test.py
      $<TARGET_FILE:onrobot_rg_command_server> $<TARGET_FILE:onrobot_rg_dual_command_server>)
  set_tests_properties(rg_model_parameters PROPERTIES TIMEOUT 90)

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
endif()

ament_export_include_directories(include)
ament_export_libraries(modbus)
ament_export_targets(export_onrobot_control HAS_LIBRARY_TARGET)

ament_package()