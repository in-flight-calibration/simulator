#include <rclcpp/rclcpp.hpp>

#include "ahrs/ahrs_groundtruth.hpp"
#include "control.hpp"

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto ahrs_node = std::make_shared<AhrsGroundtruth>("/dynamics/groundtruth");

    ControlParameters control_params
    {
        .attitude_P = Eigen::Vector2d(0.3, 0.3),
        .rates_P = Eigen::Vector3d(0.5, 1.2, 0.4),
        .rates_I = Eigen::Vector3d(0.1, 0.3, 0.1)
    };

    auto control_node = std::make_shared<ControlNode>(
        ahrs_node,
        control_params,
        "/dynamics/control",
        "/control/state"
    );

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(ahrs_node);
    executor.add_node(control_node);
    executor.spin();

    rclcpp::shutdown();
    return 0;
}