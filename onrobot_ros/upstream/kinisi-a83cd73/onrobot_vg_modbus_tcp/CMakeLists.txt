cmake_minimum_required(VERSION 3.16)
project(onrobot_vg_modbus_tcp)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclpy REQUIRED)

ament_python_install_package(${PROJECT_NAME})

ament_package()
