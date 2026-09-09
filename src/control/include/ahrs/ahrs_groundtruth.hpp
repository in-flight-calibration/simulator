#pragma once

#include <rclcpp/rclcpp.hpp>
#include <aircraft_msgs/msg/groundtruth.hpp>

#include "ahrs/ahrs.hpp"

class AhrsGroundtruth : public rclcpp::Node, public Ahrs
{
public:
    AhrsGroundtruth(const std::string& groundtruth_topic) : rclcpp::Node("ahrs_groundtruth") {
        _groundtruth_sub = this->create_subscription<aircraft_msgs::msg::Groundtruth>(
            groundtruth_topic,
            10,
            std::bind(&AhrsGroundtruth::groundtruthCallback, this, std::placeholders::_1)
        );
    }

    virtual ~AhrsGroundtruth() = default;

    virtual Eigen::Vector3d getPositionNed() const {
        Eigen::Vector3d position_ned;
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        position_ned(0) = _groundtruth.position.x;
        position_ned(1) = _groundtruth.position.y;
        position_ned(2) = _groundtruth.position.z;
        return position_ned;
    }

    virtual Eigen::Vector3d getVelocityNed() const {
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        Eigen::Vector3d velocity_ned;
        velocity_ned(0) = _groundtruth.linear_velocity.x;
        velocity_ned(1) = _groundtruth.linear_velocity.y;
        velocity_ned(2) = _groundtruth.linear_velocity.z;
        return velocity_ned;
    }

    virtual Eigen::Quaterniond getOrientation() const {
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        Eigen::Quaterniond orientation;
        orientation.w() = _groundtruth.orientation.w;
        orientation.x() = _groundtruth.orientation.x;
        orientation.y() = _groundtruth.orientation.y;
        orientation.z() = _groundtruth.orientation.z;
        return orientation;
    }

    virtual Eigen::Vector3d getAcceleration() const {
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        Eigen::Vector3d acceleration;
        acceleration(0) = _groundtruth.linear_acceleration.x;
        acceleration(1) = _groundtruth.linear_acceleration.y;
        acceleration(2) = _groundtruth.linear_acceleration.z;
        return acceleration;
    }
    
    virtual Eigen::Vector3d getAngularVelocity() const {
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        Eigen::Vector3d angular_velocity;
        angular_velocity(0) = _groundtruth.angular_velocity.x;
        angular_velocity(1) = _groundtruth.angular_velocity.y;
        angular_velocity(2) = _groundtruth.angular_velocity.z;
        return angular_velocity;
    }

    virtual Eigen::Vector3d getAirspeed() const {
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        Eigen::Vector3d airspeed;
        airspeed(0) = _groundtruth.airspeed.x;
        airspeed(1) = _groundtruth.airspeed.y;
        airspeed(2) = _groundtruth.airspeed.z;
        return airspeed;
    }

    virtual Eigen::Vector3d getLla() const {
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        Eigen::Vector3d lla;
        lla(0) = _groundtruth.latitude;
        lla(1) = _groundtruth.longitude;
        lla(2) = _groundtruth.altitude;
        return lla;
    }

private:
    aircraft_msgs::msg::Groundtruth _groundtruth;
    mutable std::mutex _groundtruth_mutex;

    rclcpp::Subscription<aircraft_msgs::msg::Groundtruth>::SharedPtr
        _groundtruth_sub;

    void groundtruthCallback(const aircraft_msgs::msg::Groundtruth::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(_groundtruth_mutex);
        _groundtruth = *msg;
    }
};
