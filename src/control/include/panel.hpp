#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <rclcpp/rclcpp.hpp>
#include <aircraft_msgs/msg/control_set_mode.hpp>

class ControlPanel
{
public:
    ControlPanel(
        std::string control_set_mode_topic
    )
        : node_(std::make_shared<rclcpp::Node>("control_panel"))
    {
        publisher_ = node_->create_publisher<aircraft_msgs::msg::ControlSetMode>(
            control_set_mode_topic, 10);

        thread_ = std::thread(&ControlPanel::run, this);
    }

    ~ControlPanel()
    {
        stop();
    }

    rclcpp::Node::SharedPtr get_node() const
    {
        return node_;
    }

private:
    void run()
    {
        int argc = 1;
        char arg0[] = "control_panel";
        char *argv[] = {arg0, nullptr};

        QApplication app(argc, argv);

        QWidget window;
        window.setWindowTitle("Aircraft Control");
        window.setMinimumWidth(320);

        auto *layout = new QVBoxLayout(&window);
        auto *form = new QFormLayout();

        auto *mode_combo = new QComboBox(&window);

        mode_combo->addItem(
            "IDLE",
            aircraft_msgs::msg::ControlSetMode::IDLE);
        mode_combo->addItem(
            "TAKEOFF",
            aircraft_msgs::msg::ControlSetMode::TAKEOFF);
        mode_combo->addItem(
            "CRUISE",
            aircraft_msgs::msg::ControlSetMode::CRUISE);
        mode_combo->addItem(
            "SLALOM",
            aircraft_msgs::msg::ControlSetMode::SLALOM);

        form->addRow("Mode:", mode_combo);

        auto *target_height = new QDoubleSpinBox(&window);
        target_height->setRange(-1000.0, 50000.0);
        target_height->setDecimals(1);
        target_height->setSingleStep(10.0);
        target_height->setValue(1000.0);

        form->addRow("Target height:", target_height);

        auto *target_speed = new QDoubleSpinBox(&window);
        target_speed->setRange(0.0, 500.0);
        target_speed->setDecimals(1);
        target_speed->setSingleStep(1.0);
        target_speed->setValue(30.0);

        form->addRow("Target speed:", target_speed);

        layout->addLayout(form);

        auto *send_button = new QPushButton("SEND", &window);
        send_button->setMinimumHeight(45);
        layout->addWidget(send_button);

        auto *status_label = new QLabel("Ready", &window);
        layout->addWidget(status_label);

        QObject::connect(
            send_button,
            &QPushButton::clicked,
            [&]()
            {
                aircraft_msgs::msg::ControlSetMode msg;

                msg.mode = static_cast<uint8_t>(
                    mode_combo->currentData().toInt());

                msg.target_height = target_height->value();
                msg.target_speed = target_speed->value();

                publisher_->publish(msg);

                status_label->setText(
                    QString("Sent: %1 | H: %2 | V: %3")
                        .arg(mode_combo->currentText())
                        .arg(msg.target_height, 0, 'f', 1)
                        .arg(msg.target_speed, 0, 'f', 1));
            });

        window.show();

        app.exec();
    }

    void stop()
    {
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    rclcpp::Node::SharedPtr node_;

    rclcpp::Publisher<
        aircraft_msgs::msg::ControlSetMode>::SharedPtr publisher_;

    std::thread thread_;
};