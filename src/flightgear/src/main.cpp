#include "flightgear_control_node.hpp"
#include "flightgear_process_node.hpp"
#include "flightgear_video_node.hpp"

#include <rclcpp/rclcpp.hpp>

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto process_node = std::make_shared<FlightgearProcessNode>();
    auto control_node = std::make_shared<FlightgearControlNode>(process_node->camera());
    auto video_node = std::make_shared<FlightgearVideoNode>(process_node->camera());

    process_node->run();
    control_node->run();
    video_node->run();

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(control_node);
    executor.add_node(video_node);
    executor.add_node(process_node);

    RCLCPP_INFO(process_node->get_logger(), "Flightgear running...");
    executor.spin();

    rclcpp::shutdown();
    return 0;
}
