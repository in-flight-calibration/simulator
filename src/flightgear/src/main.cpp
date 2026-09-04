#include "video_node.hpp"

#include <rclcpp/rclcpp.hpp>

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<VideoNode>(
        5004,
        "/flightgear/image",
        std::chrono::milliseconds(66)
    );

    node->run();

    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}