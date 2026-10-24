import os
import shlex

from launch import LaunchDescription
from launch.actions import ExecuteProcess


def generate_launch_description():
    px4_dir = os.environ['PX4_DIR']
    px4_cmd = shlex.split(os.environ['PX4_CMD'])
    px4_target = os.environ['PX4_TARGET']

    return LaunchDescription([
        ExecuteProcess(
            cmd=[*px4_cmd, px4_target],
            cwd=px4_dir,
            name='px4',
            output='screen',
        ),
    ])
