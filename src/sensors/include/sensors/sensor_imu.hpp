#pragma once

#include <random>

#include "sensors/sensor.hpp"

#include <aircraft_msgs/msg/sensor_imu.hpp>
#include <magnetic_model/magnetic_model.hpp>
#include <magnetic_model/magnetic_model_manager.hpp>
#include <sensor_msgs/msg/nav_sat_fix.hpp>

struct SensorImuParams
{
    Eigen::Quaterniond accel_rotation = Eigen::Quaterniond::Identity();
    Eigen::Vector3d accel_scaling = Eigen::Vector3d::Ones();
    Eigen::Vector3d accel_bias = Eigen::Vector3d::Zero();
    double accel_noise_sd = 0.0;

    Eigen::Vector3d gyro_bias = Eigen::Vector3d::Zero();
    double gyro_noise_sd = 0.0;

    double mag_noise_sd = 0.0;
};

class SensorImu : public Sensor<aircraft_msgs::msg::SensorImu, SensorImuParams>
{
public:
    SensorImu(rclcpp::Node& parent, const SensorImuParams& params, std::string topic_name, double update_time = -1.0)
        : Sensor<aircraft_msgs::msg::SensorImu, SensorImuParams>(parent, params, topic_name, update_time)
    {
        magnetic_model::MagneticModelManager manager{_parent};
        auto result = manager.getMagneticModel(_parent.now(), true);
        if (!result) throw std::runtime_error(result.error());
        _mag_model = result.value();
    }

private:
    std::shared_ptr<magnetic_model::MagneticModel> _mag_model;

    std::mt19937 _gen{std::random_device{}()};
    std::normal_distribution<double> _accel_dist{0.0, 1.0};
    std::normal_distribution<double> _gyro_dist{0.0, 1.0};
    std::normal_distribution<double> _mag_dist{0.0, 1.0};

    aircraft_msgs::msg::SensorImu process(
        [[maybe_unused]] double dt, 
        const aircraft_msgs::msg::Groundtruth& groundtruth, 
        [[maybe_unused]] const aircraft_msgs::msg::Environment& environment
    ) override {
        aircraft_msgs::msg::SensorImu message;
        message.header.stamp = _parent.now();
        updateAccel(groundtruth, message.accel);
        updateGyro(groundtruth, message.gyro);
        updateMag(groundtruth, message.mag);
        return message;
    }

    void addNoise(Eigen::Vector3d& vec, auto dist, double noise_sd) {
        vec.x() += dist(_gen) * noise_sd;
        vec.y() += dist(_gen) * noise_sd;
        vec.z() += dist(_gen) * noise_sd;
    }

    void updateAccel(const aircraft_msgs::msg::Groundtruth& groundtruth, auto& out) {
        Eigen::Vector3d accel = Eigen::Vector3d(
            groundtruth.linear_acceleration.x,
            groundtruth.linear_acceleration.y,
            groundtruth.linear_acceleration.z);
        Eigen::Quaterniond orientation = Eigen::Quaterniond(
            groundtruth.orientation.w,
            groundtruth.orientation.x,
            groundtruth.orientation.y,
            groundtruth.orientation.z
        );
        const Eigen::Vector3d gravity = orientation.conjugate() * Eigen::Vector3d(0.0, 0.0, 9.805);
        Eigen::Vector3d accel_meas = 
            _params.accel_rotation * _params.accel_scaling.cwiseProduct(accel - gravity) 
            + _params.accel_bias;
        addNoise(accel_meas, _accel_dist, _params.accel_noise_sd);
        out.x = accel_meas.x();
        out.y = accel_meas.y();
        out.z = accel_meas.z();
    }

    void updateGyro(const aircraft_msgs::msg::Groundtruth& groundtruth, auto& out) {
        Eigen::Vector3d gyro = Eigen::Vector3d(
            groundtruth.angular_velocity.x,
            groundtruth.angular_velocity.y,
            groundtruth.angular_velocity.z);
        Eigen::Vector3d gyro_meas = gyro + _params.gyro_bias;
        addNoise(gyro_meas, _gyro_dist, _params.gyro_noise_sd);
        out.x = gyro_meas.x();
        out.y = gyro_meas.y();
        out.z = gyro_meas.z();
    }

    void updateMag(const aircraft_msgs::msg::Groundtruth& groundtruth, auto& out) {
        sensor_msgs::msg::NavSatFix fix;
        fix.latitude  = groundtruth.latitude;
        fix.longitude = groundtruth.longitude;
        fix.altitude  = groundtruth.altitude;
        const auto stamp = _parent.get_clock()->now();
        auto result = _mag_model->getMagneticField(fix, stamp);

        if (!result) {
            return;
        }

        const auto mag_field_enu = result.value().field.magnetic_field;
        Eigen::Quaterniond orientation = Eigen::Quaterniond(
            groundtruth.orientation.w,
            groundtruth.orientation.x,
            groundtruth.orientation.y,
            groundtruth.orientation.z
        );
        Eigen::Vector3d mag_meas_ned = Eigen::Vector3d(
            mag_field_enu.y,
            mag_field_enu.x,
            -mag_field_enu.z
        ) * 1e6;
        Eigen::Vector3d mag_meas = orientation.conjugate() * mag_meas_ned;
        addNoise(mag_meas, _mag_dist, _params.mag_noise_sd);
        out.x = mag_meas.x();
        out.y = mag_meas.y();
        out.z = mag_meas.z();
    }
};