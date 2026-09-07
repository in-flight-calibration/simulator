#pragma once

#include "environment.hpp"

#include <Eigen/Dense>
#include <Eigen/Geometry>

struct RigidBodyParameters
{
    double mass;                    // [kg]
    Eigen::Matrix3d inertia;        // [kg*m^2]
};

struct RigidBodyState
{
    Eigen::Vector3d position;       // [m] world frame
    Eigen::Quaterniond orientation; // body -> world
    Eigen::Vector3d velocity;       // [m/s] body frame
    Eigen::Vector3d rates;          // [rad/s] body frame

    Eigen::Vector<double, 13> to_vec() const
    {
        Eigen::Vector<double, 13> vec;
        vec.segment<3>(0) = position;
        vec(3) = orientation.w();
        vec(4) = orientation.x();
        vec(5) = orientation.y();
        vec(6) = orientation.z();
        vec.segment<3>(7) = velocity;
        vec.segment<3>(10) = rates;
        return vec;
    }

    void from_vec(const Eigen::Vector<double, 13>& vec)
    {
        position = vec.segment<3>(0);
        orientation = Eigen::Quaterniond(
            vec(3),
            vec(4),
            vec(5),
            vec(6)
        );
        orientation.normalize();
        velocity = vec.segment<3>(7);
        rates = vec.segment<3>(10);
    }
};

class RigidBody
{
public:

    RigidBody(
        RigidBodyParameters params) :
          _rigid_body_params{params},
          _inertia_inverse{params.inertia.inverse()}
    {
    }

    const RigidBodyState& getState() const
    {
        return _state;
    }

    void setState(const RigidBodyState& state)
    {
        _state = state;
    }

    Eigen::Vector<double, 13> get_derivative(double t) const
    {
        const auto [forces, torques] = getForcesAndTorques(t);

        Eigen::Vector<double, 13> dx;

        const Eigen::Quaterniond& q     = _state.orientation;
        const Eigen::Vector3d& v        = _state.velocity;
        const Eigen::Vector3d& omega    = _state.rates;

        const Eigen::Vector4d q_vec{
            q.w(),
            q.x(),
            q.y(),
            q.z()
        };

        dx.segment<3>(0) = q * v;

        const Eigen::Quaterniond omega_q(
            0.0,
            omega.x(),
            omega.y(),
            omega.z()
        );

        const Eigen::Quaterniond q_dot = q * omega_q;

        const Eigen::Vector4d q_dot_vec{
            q_dot.w(),
            q_dot.x(),
            q_dot.y(),
            q_dot.z()
        };

        dx.segment<4>(3) = 0.5 * q_dot_vec + (1.0 - q_vec.dot(q_vec)) * q_vec;

        dx.segment<3>(7) =
            forces / _rigid_body_params.mass
            - omega.cross(v);

        dx.segment<3>(10) =
            _inertia_inverse *
            (
                torques
                - omega.cross(_rigid_body_params.inertia * omega)
            );

        return dx;
    }

protected:
    Eigen::Vector3d getGravity() const {
        return _state.orientation.conjugate() * Environment::instance().getGravity(_state.position);
    }

    virtual std::pair<Eigen::Vector3d, Eigen::Vector3d> getForcesAndTorques([[maybe_unused]] double t) const
    {
        return {getGravity(), Eigen::Vector3d::Zero()};
    }

    const RigidBodyParameters _rigid_body_params;
    RigidBodyState _state;

private:
    const Eigen::Matrix3d _inertia_inverse;
};
