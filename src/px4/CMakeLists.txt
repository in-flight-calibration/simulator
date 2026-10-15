
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

set(PX4_ZENOH_DIR "${PX4_SOURCE_DIR}/src/modules/zenoh")

set(PX4_DDS_TOPICS_FILE "$ENV{DDS_TOPICS_FILE}")
if(PX4_DDS_TOPICS_FILE STREQUAL "")
    set(PX4_DDS_TOPICS_FILE "ifc_dds_topics.yaml")
endif()
set(PX4_DDS_TOPICS_PATH "${PX4_DDS_TOPICS_FILE}")
if(NOT IS_ABSOLUTE "${PX4_DDS_TOPICS_FILE}")
    set(PX4_DDS_TOPICS_PATH "${PX4_ZENOH_DIR}/${PX4_DDS_TOPICS_FILE}")
endif()
if(NOT EXISTS "${PX4_DDS_TOPICS_PATH}")
    message(FATAL_ERROR "DDS topics file not found: ${PX4_DDS_TOPICS_PATH}")
endif()

set(PX4_MAKE_ARGS
    px4_sitl_zenoh
    "CMAKE_ARGS=-DDDS_TOPICS_FILE=${PX4_DDS_TOPICS_FILE}"
)

add_custom_target(px4_sitl ALL
    COMMAND "${MAKE_EXECUTABLE}"
            ${PX4_MAKE_ARGS}
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
