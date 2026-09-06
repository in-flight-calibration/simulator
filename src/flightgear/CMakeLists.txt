cmake_minimum_required(VERSION 3.20)
project(flightgear)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(geographic_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(OpenCV REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)

add_executable(flightgear 
    src/main.cpp
    src/flightgear_control_node.cpp
    src/process.cpp
    src/video_node.cpp
)

target_include_directories(flightgear PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(flightgear 
    PUBLIC 
    ament_index_cpp::ament_index_cpp
    cv_bridge::cv_bridge
    geographic_msgs::geographic_msgs
    image_transport::image_transport
    rclcpp::rclcpp 
    sensor_msgs::sensor_msgs
    std_msgs::std_msgs
    tf2::tf2
    tf2_ros::tf2_ros
    ${OpenCV_LIBS}
)

target_compile_definitions(flightgear PRIVATE
  SCRIPTS_DIR="${CMAKE_SOURCE_DIR}/scripts"
)

install(TARGETS
  flightgear
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
    DESTINATION include
)

ament_package()
