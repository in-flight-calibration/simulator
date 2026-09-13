#include "rclcpp/rclcpp.hpp"

#include "sensor_node.hpp"

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    SensorParameters params;
    auto node = std::make_shared<SensorNode>(
        params,
        std::chrono::milliseconds(4),
        "/dynamics/groundtruth",
        "/environment"
    );
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
