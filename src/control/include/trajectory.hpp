#include <rclcpp/rclcpp.hpp>

#include "ahrs/ahrs.hpp"
#include "control.hpp"

class TrajectoryNode : public rclcpp::Node
{
public:
    static constexpr size_t UPDATE_INTERVAL_MS = 100;

    enum class Phase {
        TAKEOFF,
        CRUISE,
        SLALOM,
    };

    TrajectoryNode(
        std::shared_ptr<Ahrs> ahrs,
        std::shared_ptr<ControlNode> control
    ) : 
        rclcpp::Node("trajectory_node"), 
        _ahrs(ahrs), 
        _control(control) 
    {
        _timer = this->create_wall_timer(
            std::chrono::milliseconds(UPDATE_INTERVAL_MS),
            std::bind(&TrajectoryNode::update, this)
        );
    }

private:
    std::shared_ptr<Ahrs> _ahrs;
    std::shared_ptr<ControlNode> _control;
    rclcpp::TimerBase::SharedPtr _timer;

    Phase _phase = Phase::TAKEOFF;
    double _phase_start_time = 0.0;

    void setPhase(Phase phase) {
        _phase = phase;
        _phase_start_time = this->now().seconds();
    }

    void updateTakeoff() {
        if (_ahrs->getPositionNed().z() < -200.0) {
            setPhase(Phase::CRUISE);
            return;
        }

        auto& control_state = _control->getControlState();

        control_state.desired_height = NAN;
        control_state.desired_speed = NAN;

        control_state.desired_attitude = Eigen::Vector2d(0.0, M_PI / 6.0);
        control_state.desired_throttle = 1.0;
    }

    void updateCruise() {
        if (this->now().seconds() - _phase_start_time > 30.0) {
            setPhase(Phase::SLALOM);
            return;
        }

        auto& control_state = _control->getControlState();

        control_state.desired_height = 200.0;
        control_state.desired_speed = 30.0;

        control_state.desired_attitude[0] = 0.0;
    }

    void updateSlalom() {
        auto& control_state = _control->getControlState();

        control_state.desired_height = 200.0;
        control_state.desired_speed = 30.0;

        control_state.desired_attitude[0] = M_PI_4 * std::sin(2 * M_PI * this->now().seconds() / 10.0);
    }

    void update() {
        switch(_phase) {
            case Phase::TAKEOFF:
                updateTakeoff();
                break;
            case Phase::CRUISE:
                updateCruise();
                break;
            case Phase::SLALOM:
                updateSlalom();
                break;
        }
    }
};
