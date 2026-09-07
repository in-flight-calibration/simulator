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
    Solver _solver;
    std::mutex _mutex;

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
        double time;
        RigidBodyState rigid_body_state;
        Eigen::Vector3d airspeed;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _solver.step(_interval);
            time = _solver.getTime();
            rigid_body_state = _aircraft.getState();
            airspeed = _aircraft.getAirspeed();
        }

        const auto sec = static_cast<int32_t>(time);
        const auto nanosec =
            static_cast<uint32_t>((time - sec) * 1e9);

        aircraft_msgs::msg::Groundtruth groundtruth_msg;

        groundtruth_msg.header.stamp.sec        = sec;
        groundtruth_msg.header.stamp.nanosec    = nanosec;

        groundtruth_msg.position            = tf2::toMsg(rigid_body_state.position);
        groundtruth_msg.orientation         = tf2::toMsg(rigid_body_state.orientation);
        groundtruth_msg.linear_velocity     = toMsg<geometry_msgs::msg::Vector3>(rigid_body_state.velocity);
        groundtruth_msg.angular_velocity    = toMsg<geometry_msgs::msg::Vector3>(rigid_body_state.rates);
        groundtruth_msg.airspeed            = toMsg<geometry_msgs::msg::Vector3>(airspeed);

        nedToLla(rigid_body_state.position,
                 groundtruth_msg.latitude,
                 groundtruth_msg.longitude,
                 groundtruth_msg.altitude);

        _groundtruth_pub->publish(groundtruth_msg);
    }

    template<typename T>
    static T toMsg(const Eigen::Vector3d& vec) {
        T msg;
        msg.x = vec.x();
        msg.y = vec.y();
        msg.z = vec.z();
        return msg;
    }
};