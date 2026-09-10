#pragma once

#include <Eigen/Dense>

#include <rclcpp/rclcpp.hpp>
#include <aircraft_msgs/msg/control.hpp>
#include <aircraft_msgs/msg/control_state.hpp>

#include "ahrs/ahrs.hpp"

struct ControlParameters {
    double energy_P;
    double energy_I;

    double distribution_P;
    double distribution_I;

    Eigen::Vector2d attitude_P;
    double bank_compensation;

    Eigen::Vector3d rates_P;
    Eigen::Vector3d rates_I;
};

struct ControlState {
    std::mutex mutex;

    double desired_speed = 0.0;
    double desired_height = 0.0;

    double energy_integral = 0.0;
    double distribution_integral = 0.0;

    double energy_error = 0.0;
    double distribution_error = 0.0;

     // roll, pitch
    Eigen::Vector2d desired_attitude = Eigen::Vector2d{0.0, 0.0};
    
     // roll rate, pitch rate, yaw rate
    Eigen::Vector3d desired_rates;
    Eigen::Vector3d rates_integral;

    double desired_throttle = 1.0;

    void reset() {
        desired_speed = NAN;
        desired_height = NAN;
        energy_error = 0.0;
        distribution_error = 0.0;
        energy_integral = 0.0;
        distribution_integral = 0.0;
        desired_attitude = Eigen::Vector2d{0.0, 0.0};
        desired_rates = Eigen::Vector3d{0.0, 0.0, 0.0};
        rates_integral = Eigen::Vector3d{0.0, 0.0, 0.0};
        desired_throttle = 0.0;
    }

    aircraft_msgs::msg::ControlState toMsg() const {
        aircraft_msgs::msg::ControlState control_state;

        control_state.energy_error = energy_error;
        control_state.distribution_error = distribution_error;

        control_state.energy_integral = energy_integral;
        control_state.distribution_integral = distribution_integral;

        control_state.desired_speed = desired_speed;
        control_state.desired_height = desired_height;

        control_state.desired_attitude.x = desired_attitude[0];
        control_state.desired_attitude.y = desired_attitude[1];
        control_state.desired_attitude.z = 0.0;

        control_state.desired_rates.x = desired_rates[0];
        control_state.desired_rates.y = desired_rates[1];
        control_state.desired_rates.z = desired_rates[2];

        control_state.rates_integral.x = rates_integral[0];
        control_state.rates_integral.y = rates_integral[1];
        control_state.rates_integral.z = rates_integral[2];

        return control_state;
    }
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

    ControlState& getControlState() {
        return _control_state;
    }

private:
    std::shared_ptr<AhrsGroundtruth> _ahrs;
    ControlParameters _control_params;

    size_t _counter = 0;
    static constexpr size_t COUNTER_MAX = 100;

    ControlState _control_state;

    aircraft_msgs::msg::Control _desired_control;

    rclcpp::Publisher<aircraft_msgs::msg::Control>::SharedPtr _control_pub;
    rclcpp::Publisher<aircraft_msgs::msg::ControlState>::SharedPtr _control_state_pub;
    rclcpp::TimerBase::SharedPtr _timer;

    void schedule(std::function<void()> func, size_t interval) {
        if (_counter % interval == 0) {
            func();
        }
    }

    void update_height_speed()
{
    if (!std::isfinite(_control_state.desired_height) ||
        !std::isfinite(_control_state.desired_speed)) {

        return;
    }

    constexpr double dt = UPDATE_INTERVAL_MS / 1000.0;

    const double height = -_ahrs->getPositionNed().z();
    const double speed  = _ahrs->getAirspeed().norm();

    if (!std::isfinite(height) || !std::isfinite(speed)) {
        return;
    }

    // ------------------------------------------------------------
    // Specific total energy
    // E = potential + kinetic
    // ------------------------------------------------------------

    const double energy =
        G * height +
        0.5 * speed * speed;

    const double desired_energy =
        G * _control_state.desired_height +
        0.5 * _control_state.desired_speed *
               _control_state.desired_speed;

    const double energy_error =
        desired_energy - energy;


    // ------------------------------------------------------------
    // Specific energy distribution
    // D = potential - kinetic
    // ------------------------------------------------------------

    const double distribution =
        G * height -
        0.5 * speed * speed;

    const double desired_distribution =
        G * _control_state.desired_height -
        0.5 * _control_state.desired_speed *
               _control_state.desired_speed;

    const double distribution_error =
        desired_distribution - distribution;


    _control_state.energy_error = energy_error;
    _control_state.distribution_error = distribution_error;


    // ------------------------------------------------------------
    // Throttle PI
    // ------------------------------------------------------------

    const double energy_integral_delta =
        _control_params.energy_I *
        energy_error *
        dt;

    const double throttle_unsaturated =
        _control_params.energy_P *
        energy_error +
        _control_state.energy_integral +
        energy_integral_delta;


    const bool throttle_saturated_high =
        throttle_unsaturated >= 1.0 &&
        energy_error > 0.0;

    const bool throttle_saturated_low =
        throttle_unsaturated <= 0.0 &&
        energy_error < 0.0;

    if (!throttle_saturated_high &&
        !throttle_saturated_low) {

        _control_state.energy_integral +=
            energy_integral_delta;
    }


    // ------------------------------------------------------------
    // Pitch PI
    // ------------------------------------------------------------

    const double distribution_error_normalized = distribution_error * 1e-3;

    const double distribution_integral_delta =
        _control_params.distribution_I *
        distribution_error_normalized *
        dt;

    const double pitch_unsaturated =
        _control_params.distribution_P *
        distribution_error_normalized +
        _control_state.distribution_integral +
        distribution_integral_delta;


    const bool pitch_saturated_high =
        pitch_unsaturated >= M_PI_4 &&
        distribution_error > 0.0;

    const bool pitch_saturated_low =
        pitch_unsaturated <= -M_PI_4 &&
        distribution_error < 0.0;

    if (!pitch_saturated_high &&
        !pitch_saturated_low) {

        _control_state.distribution_integral +=
            distribution_integral_delta;
    }


    // ------------------------------------------------------------
    // Outputs
    // ------------------------------------------------------------

    _control_state.desired_throttle =
        std::clamp(throttle_unsaturated, 0.0, 1.0);

    _control_state.desired_attitude[1] =
        std::clamp(
            pitch_unsaturated,
            -M_PI_4,
             M_PI_4);
}

    void update_attitude()
    {
        if (!std::isfinite(_control_state.desired_attitude[0]) || !std::isfinite(_control_state.desired_attitude[1])) {
            return;
        }

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

        const double phi_d = std::clamp(
            _control_state.desired_attitude[0],
            -M_PI_4,
            M_PI_4
        );

        const double pitch_delta = _control_params.bank_compensation * (1.0 / std::cos(phi_d) - 1.0);

        const Eigen::Vector2d roll_pitch{roll, pitch + pitch_delta};
        const Eigen::Vector2d attitude_error = _control_state.desired_attitude - roll_pitch;
        _control_state.desired_rates.head<2>() = _control_params.attitude_P.cwiseProduct(attitude_error);

        const double V = _ahrs->getAirspeed().norm();

        if (V > 1.0) {
            

            const double dpsi = G / V * std::tan(phi_d);
            _control_state.desired_rates[2] = (dpsi * std::cos(_control_state.desired_attitude[1]) - _control_state.desired_rates[1] * std::sin(phi_d)) / std::cos(phi_d);

        } else {
            _control_state.desired_rates[2] = 0.0;
        }
    }

    void update_rates()
    {
        const Eigen::Vector3d angular_velocity =_ahrs->getAngularVelocity();
        const Eigen::Vector3d error =_control_state.desired_rates - angular_velocity;

        const Eigen::Vector3d proportional =_control_params.rates_P.cwiseProduct(error);
        const Eigen::Vector3d integral_delta = _control_params.rates_I.cwiseProduct(error) 
            * UPDATE_INTERVAL_MS / 1000.0;

        const Eigen::Vector3d unsaturated = proportional + _control_state.rates_integral + integral_delta;

        for (int i = 0; i < 3; ++i) {
            const bool saturated_high =
                unsaturated[i] >= 1.0 && error[i] > 0.0;

            const bool saturated_low =
                unsaturated[i] <= -1.0 && error[i] < 0.0;

            if (!saturated_high && !saturated_low) {
                _control_state.rates_integral[i] += integral_delta[i];
            }
        }

        const Eigen::Vector3d torques = unsaturated
            .cwiseMax(-1.0)
            .cwiseMin(1.0);

        _desired_control.aileron = torques[0];
        _desired_control.elevator = torques[1];
        _desired_control.rudder = torques[2];
        _desired_control.throttle = _control_state.desired_throttle;
    }

    void publish_control() {
        _desired_control.header.stamp = this->now();
        _control_pub->publish(_desired_control);
    }

    void publish_control_state()
    {
        auto msg = _control_state.toMsg();
        msg.header.stamp = this->now();
        _control_state_pub->publish(msg);
    }

    void update() {
        _counter = (_counter + 1) % COUNTER_MAX;

        std::lock_guard<std::mutex> lock(_control_state.mutex);
        schedule(std::bind(&ControlNode::update_height_speed, this), 5);
        schedule(std::bind(&ControlNode::update_attitude, this), 5);
        update_rates();
        
        publish_control();
        publish_control_state();
    }
};

