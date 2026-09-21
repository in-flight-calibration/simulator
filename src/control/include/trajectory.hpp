#include <rclcpp/rclcpp.hpp>
#include <aircraft_msgs/msg/control_set_mode.hpp>

#include "ahrs/ahrs.hpp"
#include "control.hpp"
#include "panel.hpp"

class TrajectoryNode : public rclcpp::Node
{
public:
    static constexpr size_t UPDATE_INTERVAL_MS = 100;

    enum class Mode : uint8_t {
        IDLE = aircraft_msgs::msg::ControlSetMode::IDLE,
        TAKEOFF = aircraft_msgs::msg::ControlSetMode::TAKEOFF,
        CRUISE = aircraft_msgs::msg::ControlSetMode::CRUISE,
        SLALOM = aircraft_msgs::msg::ControlSetMode::SLALOM,
    };

    TrajectoryNode(
        std::shared_ptr<Ahrs> ahrs,
        std::shared_ptr<ControlNode> control,
        std::string control_set_mode_topic
    ) : 
        rclcpp::Node("trajectory_node"), 
        _ahrs(ahrs), 
        _control(control) 
    {
        _set_mode_sub = this->create_subscription<aircraft_msgs::msg::ControlSetMode>(
            control_set_mode_topic,
            10,
            std::bind(&TrajectoryNode::setModeCallback, this, std::placeholders::_1)
        );

        _timer = this->create_wall_timer(
            std::chrono::milliseconds(UPDATE_INTERVAL_MS),
            std::bind(&TrajectoryNode::update, this)
        );
    }

private:
    std::shared_ptr<Ahrs> _ahrs;
    std::shared_ptr<ControlNode> _control;

    rclcpp::Subscription<aircraft_msgs::msg::ControlSetMode>::SharedPtr _set_mode_sub;
    rclcpp::TimerBase::SharedPtr _timer;

    Mode _mode = Mode::IDLE;
    double _mode_start_time = 0.0;

    double _target_height = 0;
    double _target_speed = 30.0;

    void setMode(Mode mode) {
        _mode = mode;
        _mode_start_time = this->now().seconds();
        initMode();
    }

    void setModeCallback(const aircraft_msgs::msg::ControlSetMode::SharedPtr msg) {
        _target_height = msg->target_height;
        _target_speed = msg->target_speed;
        setMode(static_cast<Mode>(msg->mode));
    }

    void updateIdle() {
        auto& control_state = _control->getControlState();

        control_state.desired_height = NAN;
        control_state.desired_speed = NAN;

        control_state.desired_attitude = Eigen::Vector2d(NAN,NAN);
        control_state.desired_rates = Eigen::Vector3d{NAN, NAN, NAN};
        control_state.desired_throttle = NAN;
    }

    void updateTakeoff() {
        auto& control_state = _control->getControlState();

        control_state.desired_height = NAN;
        control_state.desired_speed = NAN;

        control_state.desired_attitude = Eigen::Vector2d(0.0, M_PI / 6.0);
        control_state.desired_throttle = 1.0;
    }

    void updateCruise() {
        auto& control_state = _control->getControlState();

        control_state.desired_height = _target_height;
        control_state.desired_speed = _target_speed;

        control_state.desired_attitude[0] = 0.0;
    }

    void updateSlalom() {
        auto& control_state = _control->getControlState();

        control_state.desired_height = _target_height;
        control_state.desired_speed = _target_speed;

        control_state.desired_attitude[0] = M_PI_4 * std::sin(2 * M_PI * this->now().seconds() / 10.0);
    }

    void initMode() {
        switch(_mode) {
            case Mode::CRUISE:
            case Mode::SLALOM:
            case Mode::IDLE:
            case Mode::TAKEOFF:
                break;
        }
    }

    void update() {
        switch(_mode) {
            case Mode::IDLE:
                updateIdle();
                break;
            case Mode::TAKEOFF:
                updateTakeoff();
                break;
            case Mode::CRUISE:
                updateCruise();
                break;
            case Mode::SLALOM:
                updateSlalom();
                break;
        }
    }
};
