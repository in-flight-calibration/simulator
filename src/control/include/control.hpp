#pragma once

#include <Eigen/Dense>

#include <rclcpp/rclcpp.hpp>
#include <aircraft_msgs/msg/control.hpp>
#include <aircraft_msgs/msg/control_state.hpp>

#include "ahrs/ahrs.hpp"

struct ControlParameters {
    Eigen::Vector2d attitude_P;

    Eigen::Vector3d rates_P;
    Eigen::Vector3d rates_I;
};

class ControlNode : public rclcpp::Node
{
public:
    static constexpr double G = 9.805;
    static constexpr size_t UPDATE_INTERVAL_MS = 4;

    ControlNode(
        std::shared_ptr<AhrsGroundtruth> ahrs,
        const ControlParameters& control_params,
        const std::string& control_topic,
        const std::string& control_state_topic
    ) : 
        rclcpp::Node("control_node"), 
        _ahrs(ahrs), 
        _control_params(control_params) 
    {

        _control_pub = this->create_publisher<aircraft_msgs::msg::Control>(
            control_topic,
            10
        );

        _control_state_pub = this->create_publisher<aircraft_msgs::msg::ControlState>(
            control_state_topic,
            10
        );

        _timer = this->create_wall_timer(
            std::chrono::milliseconds(UPDATE_INTERVAL_MS),
            std::bind(&ControlNode::update, this)
        );
    }
private:
    std::shared_ptr<AhrsGroundtruth> _ahrs;
    ControlParameters _control_params;

    size_t _counter = 0;
    static constexpr size_t COUNTER_MAX = 100;

     // roll, pitch
    Eigen::Vector2d _desired_attitude = Eigen::Vector2d{0.0, M_PI / 6.0};
    
     // roll rate, pitch rate, yaw rate
    Eigen::Vector3d _desired_rates;
    Eigen::Vector3d _rates_integral;

    double _desired_throttle = 1.0;

    aircraft_msgs::msg::Control _desired_control;

    rclcpp::Publisher<aircraft_msgs::msg::Control>::SharedPtr _control_pub;
    rclcpp::Publisher<aircraft_msgs::msg::ControlState>::SharedPtr _control_state_pub;
    rclcpp::TimerBase::SharedPtr _timer;

    void schedule(std::function<void()> func, size_t interval) {
        if (_counter % interval == 0) {
            func();
        }
    }

    void update_attitude()
    {
        const Eigen::Quaterniond q = _ahrs->getOrientation();

        const double roll = std::atan2(
            2.0 * (q.w() * q.x() + q.y() * q.z()),
            1.0 - 2.0 * (q.x() * q.x() + q.y() * q.y())
        );

        const double pitch = std::asin(std::clamp(
            2.0 * (q.w() * q.y() - q.z() * q.x()),
            -1.0,
            1.0
        ));

        const Eigen::Vector2d roll_pitch{roll, pitch};
        const Eigen::Vector2d attitude_error = _desired_attitude - roll_pitch;
        _desired_rates.head<2>() = _control_params.attitude_P.cwiseProduct(attitude_error);

        const double V = _ahrs->getAirspeed().norm();

        if (V > 1.0) {
            const double phi_d = std::clamp(
                _desired_attitude[0],
                -M_PI_4,
                M_PI_4
            );

            const double dpsi = G / V * std::tan(phi_d);
            _desired_rates[2] = (dpsi * std::cos(_desired_attitude[1]) - _desired_rates[1] * std::sin(phi_d)) / std::cos(phi_d);

        } else {
            _desired_rates[2] = 0.0;
        }
    }

    void update_rates()
    {
        const Eigen::Vector3d angular_velocity =_ahrs->getAngularVelocity();
        const Eigen::Vector3d error =_desired_rates - angular_velocity;

        const Eigen::Vector3d proportional =_control_params.rates_P.cwiseProduct(error);
        const Eigen::Vector3d integral_delta = _control_params.rates_I.cwiseProduct(error) 
            * UPDATE_INTERVAL_MS / 1000.0;

        const Eigen::Vector3d unsaturated = proportional + _rates_integral + integral_delta;

        for (int i = 0; i < 3; ++i) {
            const bool saturated_high =
                unsaturated[i] >= 1.0 && error[i] > 0.0;

            const bool saturated_low =
                unsaturated[i] <= -1.0 && error[i] < 0.0;

            if (!saturated_high && !saturated_low) {
                _rates_integral[i] += integral_delta[i];
            }
        }

        const Eigen::Vector3d torques = unsaturated
            .cwiseMax(-1.0)
            .cwiseMin(1.0);

        _desired_control.aileron = torques[0];
        _desired_control.elevator = torques[1];
        _desired_control.rudder = torques[2];
        _desired_control.throttle = _desired_throttle;
    }

    void publish_control() {
        _desired_control.header.stamp = this->now();
        _control_pub->publish(_desired_control);
    }

    

    void publish_control_state()
    {
        aircraft_msgs::msg::ControlState control_state;

        control_state.header.stamp = this->now();

        control_state.desired_attitude.x = _desired_attitude[0];
        control_state.desired_attitude.y = _desired_attitude[1];
        control_state.desired_attitude.z = 0.0;

        control_state.desired_rates.x = _desired_rates[0];
        control_state.desired_rates.y = _desired_rates[1];
        control_state.desired_rates.z = _desired_rates[2];

        control_state.rates_integral.x = _rates_integral[0];
        control_state.rates_integral.y = _rates_integral[1];
        control_state.rates_integral.z = _rates_integral[2];

        _control_state_pub->publish(control_state);
    }

    void update() {
        _counter = (_counter + 1) % COUNTER_MAX;

        schedule(std::bind(&ControlNode::update_attitude, this), 5);
        update_rates();
        
        publish_control();
        publish_control_state();
    }
};

