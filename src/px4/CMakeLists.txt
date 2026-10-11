
cmake_minimum_required(VERSION 3.20)
project(px4)

find_package(ament_cmake REQUIRED)
find_program(MAKE_EXECUTABLE make REQUIRED)

set(PX4_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PX4-Autopilot")

if(NOT EXISTS "${PX4_SOURCE_DIR}/Makefile")
    message(FATAL_ERROR
        "PX4-Autopilot not found: ${PX4_SOURCE_DIR}"
    )
endif()

add_custom_target(px4_sitl ALL
    COMMAND "${MAKE_EXECUTABLE}"
            px4_sitl_zenoh
            "CMAKE_ARGS=-DDDS_TOPICS_FILE=ifc_dds_topics.yaml"
    WORKING_DIRECTORY "${PX4_SOURCE_DIR}"
    USES_TERMINAL
    VERBATIM
    COMMENT "Building PX4 SITL"
)

install(
    DIRECTORY launch
    DESTINATION share/${PROJECT_NAME}
)

ament_package()
