#include "flightgear_control_node.hpp"
#include "video_node.hpp"

#include <rclcpp/rclcpp.hpp>

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto flightgear_control_node = std::make_shared<FlightgearControlNode>(
        "/flightgear/position",
        "/flightgear/orientation",
        5500,
        std::chrono::milliseconds(66)
    );

    auto video_node = std::make_shared<VideoNode>(
        5501,
        "/flightgear/image",
        std::chrono::milliseconds(66)
    );

    flightgear_control_node->run();
    video_node->run();

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(flightgear_control_node);
    executor.add_node(video_node);

    executor.spin();

    rclcpp::shutdown();
    return 0;
}