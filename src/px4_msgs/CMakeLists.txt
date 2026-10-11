cmake_minimum_required(VERSION 3.20)
project(px4_msgs)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

set(PX4_TOPICS_FILE "$ENV{DDS_TOPICS_FILE}")
if(PX4_TOPICS_FILE STREQUAL "")
    set(PX4_TOPICS_FILE "ifc_dds_topics.yaml")
endif()
if(NOT IS_ABSOLUTE "${PX4_TOPICS_FILE}")
    set(PX4_TOPICS_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/../px4/PX4-Autopilot/src/modules/zenoh/${PX4_TOPICS_FILE}"
    )
endif()

execute_process(
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/import_messages.sh"
    RESULT_VARIABLE import_result
)
if(NOT import_result EQUAL 0)
    message(FATAL_ERROR "Failed to import PX4 messages")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${PX4_TOPICS_FILE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/import_messages.sh"
)

file(GLOB PX4_MSG_FILES CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/msg/*.msg"
)
if(NOT PX4_MSG_FILES)
    message(FATAL_ERROR
        "No PX4 messages were imported from ${PX4_TOPICS_FILE}"
    )
endif()

rosidl_generate_interfaces(${PROJECT_NAME}
    ${PX4_MSG_FILES}
)

add_custom_target(import_px4_messages
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/import_messages.sh"
    VERBATIM
)
add_dependencies(${PROJECT_NAME} import_px4_messages)

ament_export_dependencies(rosidl_default_runtime)
ament_package()