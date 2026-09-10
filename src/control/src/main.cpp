#include <rclcpp/rclcpp.hpp>

#include "ahrs/ahrs_groundtruth.hpp"
#include "control.hpp"
#include "trajectory.hpp"

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    auto ahrs_node = std::make_shared<AhrsGroundtruth>("/dynamics/groundtruth");

    ControlParameters control_params
    {
        .energy_P = 0.02,
        .energy_I = 0.2,
        .distribution_P = 0.6,
        .distribution_I = 0.4,

        .attitude_P = Eigen::Vector2d(0.3, 0.3),
        .bank_compensation = 0.02,

        .rates_P = Eigen::Vector3d(0.5, 0.9, 0.8),
        .rates_I = Eigen::Vector3d(0.1, 0.3, 0.1)
    };

    auto control_node = std::make_shared<ControlNode>(
        ahrs_node,
        control_params,
        "/dynamics/control",
        "/control/state"
    );

    auto trajectory_node = std::make_shared<TrajectoryNode>(
        ahrs_node,
        control_node
    );

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(ahrs_node);
    executor.add_node(control_node);
    executor.add_node(trajectory_node);
    executor.spin();

    rclcpp::shutdown();
    return 0;
}