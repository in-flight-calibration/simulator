
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

if("$ENV{PX4_CMD}" STREQUAL "")
    message(FATAL_ERROR "PX4_CMD is not set (see .env)")
endif()
separate_arguments(PX4_CMD_ARGS UNIX_COMMAND "$ENV{PX4_CMD}")

add_custom_target(px4_sitl ALL
    COMMAND ${PX4_CMD_ARGS}
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
