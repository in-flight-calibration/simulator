import os

from launch import LaunchDescription
from launch.actions import (
    ExecuteProcess,
    IncludeLaunchDescription,
    TimerAction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    px4_launch = os.path.join(
        get_package_share_directory('px4'),
        'launch',
        'px4.launch.py',
    )

    return LaunchDescription([
        # Zenoh router
        ExecuteProcess(
            cmd=['ros2', 'run', 'rmw_zenoh_cpp', 'rmw_zenohd'],
            name='zenohd',
            output='screen',
        ),

        # PX4 SITL
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(px4_launch),
        ),

        # FlightGear
        Node(
            package='flightgear',
            executable='flightgear',
            output='screen',
        ),
    ])
