from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import TimerAction


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='flightgear',
            executable='flightgear',
            output='screen',
        ),
        Node(
            package='dynamics',
            executable='dynamics',
            output='screen',
        ),
        Node(
            package='sensors',
            executable='sensors',
            output='screen',
        ),
        TimerAction(
            period=10.0,
            actions=[
                Node(
                    package='control',
                    executable='control',
                    output='screen',
                ),
            ],
        ),
    ])