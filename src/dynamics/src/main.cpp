#include "rclcpp/rclcpp.hpp"

#include "aircrafts.hpp"
#include "dynamics.hpp"

#include <chrono>
#include <iostream>

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    const Eigen::Quaterniond q =
        Eigen::Quaterniond(
            Eigen::AngleAxisd(M_PI / 6.0, Eigen::Vector3d::UnitY())
        );

    Aircraft aircraft(
        smallUav(),
        Eigen::Vector3d::Zero(),
        Eigen::Vector3d::Zero(),
        q
    );

    Eigen::Vector3d EPBC{
        52.2692,
        20.9072,
        107.0
    };

    auto dynamics_node = std::make_shared<DynamicsNode>(
        aircraft,
        EPBC,
        std::chrono::milliseconds(2),
        "/dynamics/control",
        "/dynamics/groundtruth"
    );

    std::cout << "Dynamics node running..." << std::endl;
    rclcpp::spin(dynamics_node);

    rclcpp::shutdown();
    return 0;
}
