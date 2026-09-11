#pragma once

#include <geometry_msgs/msg/vector3.hpp>
#include <rclcpp/rclcpp.hpp>
#include <tf2_eigen/tf2_eigen.hpp>

#include "aircraft_msgs/msg/control.hpp"
#include "aircraft_msgs/msg/groundtruth.hpp"

#include "aircraft.hpp"
#include "solver.hpp"

#include <mutex>

class DynamicsNode : public rclcpp::Node
{
public:
    DynamicsNode(
        Aircraft& aircraft,
        Eigen::Vector3d home,
        std::chrono::milliseconds interval,
        std::string control_topic,
        std::string groundtruth_topic
    )
        : rclcpp::Node("dynamics_node"),
            _home{home},
            _interval{interval.count() / 1000.0},
            _aircraft{aircraft},
            _environment{*this},
            _solver{aircraft}
    {
        _control_sub =
            create_subscription<aircraft_msgs::msg::Control>(
                control_topic,
                10,
                std::bind(
                    &DynamicsNode::controlCallback,
                    this,
                    std::placeholders::_1));

        _groundtruth_pub =
            create_publisher<aircraft_msgs::msg::Groundtruth>(
                groundtruth_topic,
                10);

        _timer = create_wall_timer(
            interval,
            std::bind(&DynamicsNode::timerCallback, this)
        );
    }

private:
    const Eigen::Vector3d _home;
    const double _interval;

    Aircraft& _aircraft;
    Environment _environment;
    Solver _solver;
    std::mutex _mutex;

    double _time;
    RigidBodyState _rigid_body_state;
    Eigen::Vector3d _airspeed;
    Eigen::Vector3d _acceleration_body;

    rclcpp::Subscription<aircraft_msgs::msg::Control>::SharedPtr
        _control_sub;

    rclcpp::Publisher<aircraft_msgs::msg::Groundtruth>::SharedPtr _groundtruth_pub;

    rclcpp::TimerBase::SharedPtr _timer;

    void controlCallback(const aircraft_msgs::msg::Control::SharedPtr msg) {
        const AircraftControl control{
            .aileron = std::clamp(msg->aileron, -1.0, 1.0),
            .elevator = std::clamp(msg->elevator, -1.0, 1.0),
            .rudder = std::clamp(msg->rudder, -1.0, 1.0),
            .throttle = std::clamp(msg->throttle, 0.0, 1.0)
        };

        std::lock_guard<std::mutex> lock(_mutex);
        const double time = _solver.getTime();
        _aircraft.launch(time);
        _aircraft.setControl(time, control);
    }

    void nedToLla(const Eigen::Vector3d& pos,
              double& lat,
              double& lon,
              double& alt) const
    {
        constexpr double R = 6378137.0;
        constexpr double DEG2RAD = M_PI / 180.0;
        constexpr double RAD2DEG = 180.0 / M_PI;

        const double lat0 = _home.x() * DEG2RAD;

        lat = _home.x() + pos.x() / R * RAD2DEG;
        lon = _home.y() + pos.y() / (R * std::cos(lat0)) * RAD2DEG;
        alt = _home.z() - pos.z();
    }

    void timerCallback() {
        const double altitude = -_rigid_body_state.position.z();
        _environment.update(_time, altitude);

        RigidBodyState next_rigid_body_state;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _aircraft.updateEnvironment(_environment);
            _solver.step(_interval);
            next_rigid_body_state = _aircraft.getState();

            _time = _solver.getTime();
            _airspeed = _aircraft.getAirspeed();
        }

        Eigen::Vector3d prev_velocity_ned = _rigid_body_state.orientation * _rigid_body_state.velocity;
        _rigid_body_state = next_rigid_body_state;
        Eigen::Vector3d velocity_ned = _rigid_body_state.orientation * _rigid_body_state.velocity;
        Eigen::Vector3d acceleration_ned = (velocity_ned - prev_velocity_ned) / _interval;
        _acceleration_body = _rigid_body_state.orientation.conjugate() * acceleration_ned;
        publish();
    }

    void publish() {
        auto toMsg = [](const Eigen::Vector3d& vec, auto& out) {
            out.x = vec.x();
            out.y = vec.y();
            out.z = vec.z();
        };

        aircraft_msgs::msg::Groundtruth groundtruth_msg;

        groundtruth_msg.header.stamp = this->now();
        toMsg(_rigid_body_state.position, groundtruth_msg.position);
        toMsg(_rigid_body_state.velocity, groundtruth_msg.linear_velocity);
        toMsg(_rigid_body_state.rates, groundtruth_msg.angular_velocity);
        toMsg(_airspeed, groundtruth_msg.airspeed);
        toMsg(_acceleration_body, groundtruth_msg.linear_acceleration);

        groundtruth_msg.orientation.x = _rigid_body_state.orientation.x();
        groundtruth_msg.orientation.y = _rigid_body_state.orientation.y();
        groundtruth_msg.orientation.z = _rigid_body_state.orientation.z();
        groundtruth_msg.orientation.w = _rigid_body_state.orientation.w();

        nedToLla(_rigid_body_state.position,
                 groundtruth_msg.latitude,
                 groundtruth_msg.longitude,
                 groundtruth_msg.altitude);

        _groundtruth_pub->publish(groundtruth_msg);
    }
};