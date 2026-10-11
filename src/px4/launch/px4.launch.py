from pathlib import Path

from launch import LaunchDescription
from launch.actions import ExecuteProcess
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    project_dir = Path(
        get_package_share_directory('px4')
    ).parents[3]

    px4_dir = project_dir / 'src' / 'px4' / 'PX4-Autopilot'

    return LaunchDescription([
        ExecuteProcess(
            cmd=['make', 'px4_sitl_zenoh', 'sihsim_ifc'],
            cwd=str(px4_dir),
            name='px4',
            output='screen',
        ),
    ])