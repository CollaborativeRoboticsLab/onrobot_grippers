cmake_minimum_required(VERSION 3.16)
project(onrobot_vg_description)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY config images meshes urdf
  DESTINATION share/${PROJECT_NAME}
)

install(FILES README.md LICENSE DESTINATION share/${PROJECT_NAME})

ament_package()