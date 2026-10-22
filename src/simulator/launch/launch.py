import os

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    IncludeLaunchDescription,
)
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    px4_launch = os.path.join(
        get_package_share_directory('px4'),
        'launch',
        'px4.launch.py',
    )

    config = LaunchConfiguration('config')
    zenoh_router = LaunchConfiguration('zenoh_router')

    return LaunchDescription([
        DeclareLaunchArgument(
            'config',
            description='Path to the simulator configuration file',
        ),
        DeclareLaunchArgument(
            'zenoh_router',
            default_value='true',
            description='Start a Zenoh router; false uses one already running on localhost:7447',
        ),

        # Zenoh router
        ExecuteProcess(
            cmd=['ros2', 'run', 'rmw_zenoh_cpp', 'rmw_zenohd'],
            name='zenohd',
            output='screen',
            condition=IfCondition(zenoh_router),
        ),

        # PX4 SITL
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(px4_launch),
        ),

        # FlightGear
        Node(
            package='flightgear',
            executable='flightgear',
            arguments=[config],
            output='screen',
        ),
    ])
