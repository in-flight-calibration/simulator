#pragma once

#include "rigid_body.hpp"

class Solver {
public:
    Solver(RigidBody& body) : _body(body) {}

    void step(double dt) {
        const auto state = _body.getState().to_vec();

        const auto k1 = f(_time, state);
        const auto k2 = f(_time + dt / 2.0, state + (dt / 2.0) * k1);
        const auto k3 = f(_time + dt / 2.0, state + (dt / 2.0) * k2);
        const auto k4 = f(_time + dt, state + dt * k3);

        RigidBodyState next_state;
        next_state.from_vec(
            state + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4));
        _body.setState(next_state);
        _time += dt;
    }

    double getTime() const { return _time; }
    
private:
    double _time = 0.0;
    RigidBody& _body;

    Eigen::Vector<double, 13> f(double t, const Eigen::Vector<double, 13>& state) {
        RigidBodyState rb_state;
        rb_state.from_vec(state);
        _body.setState(rb_state);
        return _body.get_derivative(t);
    }
};