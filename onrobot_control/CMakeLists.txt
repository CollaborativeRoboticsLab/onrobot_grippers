cmake_minimum_required(VERSION 3.16)
project(onrobot_control LANGUAGES CXX)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

include(CTest)
include(GNUInstallDirs)

add_library(onrobot_control_core INTERFACE)
target_include_directories(onrobot_control_core
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(onrobot_control_core INTERFACE cxx_std_17)

add_executable(onrobot_control_node src/control.cpp)
target_link_libraries(onrobot_control_node onrobot_control_core)
ament_target_dependencies(onrobot_control_node rclcpp)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS onrobot_control_core EXPORT onrobot_control_coreTargets)
install(TARGETS onrobot_control_node DESTINATION lib/${PROJECT_NAME})
install(EXPORT onrobot_control_coreTargets
  FILE onrobot_control_coreTargets.cmake
  NAMESPACE onrobot_control::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/onrobot_control
)

if(BUILD_TESTING)
  add_executable(test_command_validation test/command_validation_test.cpp)
  target_link_libraries(test_command_validation PRIVATE onrobot_control_core)
  target_compile_options(test_command_validation PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME command_validation COMMAND test_command_validation)
endif()