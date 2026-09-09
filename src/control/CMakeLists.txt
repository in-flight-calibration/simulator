cmake_minimum_required(VERSION 3.20)
project(control)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(aircraft_msgs REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(tf2_ros REQUIRED)

find_package(Eigen3 REQUIRED)

add_executable(control 
    src/main.cpp
)

target_include_directories(control PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(control 
    PUBLIC
    aircraft_msgs::aircraft_msgs
    ament_index_cpp::ament_index_cpp
    rclcpp::rclcpp 
    tf2::tf2
    tf2_eigen::tf2_eigen
    tf2_ros::tf2_ros

    Eigen3::Eigen
)

install(TARGETS
    control
    DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
    DESTINATION include
)

ament_package()
