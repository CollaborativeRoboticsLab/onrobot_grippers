cmake_minimum_required(VERSION 3.16)
project(onrobot_rg_description)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY config images meshes urdf
  DESTINATION share/${PROJECT_NAME}
)

install(FILES README.md LICENSE DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_program(XACRO_EXECUTABLE xacro REQUIRED)
  find_program(CHECK_URDF_EXECUTABLE check_urdf REQUIRED)

  foreach(MODEL_FILE IN ITEMS onrobot_rg2_model.xacro onrobot_rg6_model.xacro)
    get_filename_component(MODEL_NAME "${MODEL_FILE}" NAME_WE)
    add_test(
      NAME "${PROJECT_NAME}_${MODEL_NAME}"
      COMMAND "${CMAKE_COMMAND}"
        "-DXACRO_EXECUTABLE=${XACRO_EXECUTABLE}"
        "-DCHECK_URDF_EXECUTABLE=${CHECK_URDF_EXECUTABLE}"
        "-DMODEL_FILE=${CMAKE_CURRENT_SOURCE_DIR}/urdf/${MODEL_FILE}"
        "-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${MODEL_NAME}.urdf"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/../test/validate_xacro.cmake"
    )
  endforeach()
endif()

ament_package()