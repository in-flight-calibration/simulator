#pragma once

#include "rigid_body.hpp"

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <cfloat>

#include <iostream>

struct AircraftParameters
{
    RigidBodyParameters rigid_body;

    Eigen::Vector<double, 6> C0;
    Eigen::Matrix<double, 6, 5> Cab; // [dC/dalpha, dC/dalpha^2, dC/dbeta, dC/dbeta^2, dC/dalpha*dbeta]
    Eigen::Matrix<double, 6, 3> Cpqr;
    Eigen::Matrix<double, 6, 3> Cdelta; // daileron, delevator, drudder

    double surface_area;    // [m^2]
    double wing_span;       // [m]
    double mean_chord;      // [m]
    double thrust_max;      // [N]

    struct launcher {
        double force;  // [N]
        double time;   // [s]
    } launcher;
};

struct AircraftControl {
    double aileron;     // [-1; 1]
    double elevator;    // [-1; 1]
    double rudder;      // [-1; 1]

    double throttle;    // [0; 1]
};

class Aircraft : public RigidBody
{
public:
    Aircraft(
        const AircraftParameters& aircraft_params,
        Eigen::Vector3d initial_position = Eigen::Vector3d::Zero(),
        Eigen::Vector3d initial_velocity = Eigen::Vector3d::Zero(),
        Eigen::Quaterniond initial_orientation = Eigen::Quaterniond::Identity()
    )
        : RigidBody(aircraft_params.rigid_body),
          _aircraft_param(aircraft_params)
    {
        _state.position = initial_position;
        _state.velocity = initial_velocity;
        _state.orientation = initial_orientation;

        _control = AircraftControl{0.0, 0.0, 0.0, 0.0};
        _launcher_start_time = -1.0;
    }

    void setControl([[maybe_unused]] double t, const AircraftControl& control) {
        _control = control;
    }

    void setWind(const Eigen::Vector3d& wind) {
        _wind = wind;
    }

    Eigen::Vector3d getAirspeed() const {
        return _state.velocity - _state.orientation.conjugate() * _wind;
    }

    void launch(double t) {
        if (_launcher_start_time < 0.0) {
            _launcher_start_time = t;
        }
    }

    std::pair<Eigen::Vector3d, Eigen::Vector3d> getForcesAndTorques(double t) const override
    {
        const Eigen::Vector3d airspeed = getAirspeed();
        const double dynamic_pressure = 0.5 * Environment::instance().getAirDensity(_state.position) * airspeed.squaredNorm();

        Eigen::Vector3d forces = getGravity();
        Eigen::Vector3d torques = Eigen::Vector3d::Zero();

        if (dynamic_pressure > 1.0) {
            const double alpha = std::atan2(airspeed.z(), airspeed.x());
            const double beta = std::asin(airspeed.y() / airspeed.norm());
            const double V = airspeed.norm();

            const Eigen::Vector<double, 5> ab(
                alpha, 
                alpha * alpha, 
                beta, 
                beta * beta, 
                alpha * beta);

            const Eigen::Vector3d bcb {
                _aircraft_param.wing_span,
                _aircraft_param.mean_chord,
                _aircraft_param.wing_span
            };

            const Eigen::Vector<double, 6> C 
                =  _aircraft_param.C0 
                + _aircraft_param.Cab * ab
                + _aircraft_param.Cpqr * bcb.cwiseProduct(_state.rates) / (2.0 * V)
                + _aircraft_param.Cdelta * Eigen::Vector3d(_control.aileron, _control.elevator, _control.rudder);


            forces += dynamic_pressure * _aircraft_param.surface_area * C.segment<3>(0);
            torques += dynamic_pressure * _aircraft_param.surface_area * bcb.cwiseProduct(C.segment<3>(3));
        }

        const double thrust = _control.throttle * _control.throttle * _aircraft_param.thrust_max;
        forces += Eigen::Vector3d(thrust, 0.0, 0.0);

        applyLauncherForcesAndTorques(t, forces, torques);
        return {forces, torques};
    }

private:
    AircraftParameters _aircraft_param;
    AircraftControl _control;

    Eigen::Vector3d _wind;

    double _launcher_start_time;

    void applyLauncherForcesAndTorques(double t, Eigen::Vector3d& forces, Eigen::Vector3d& torques) const {
        if (_launcher_start_time < 0.0) {
            forces = Eigen::Vector3d::Zero();
            torques = Eigen::Vector3d::Zero();
            return;
        }

        if (t < _launcher_start_time + _aircraft_param.launcher.time) {
            const Eigen::Vector3d launcher_force(_aircraft_param.launcher.force, 0.0, 0.0);
            const Eigen::Vector3d launcher_torque(0.0, 0.0, 0.0);

            forces += launcher_force;
            torques += launcher_torque;
        }
    }
};
