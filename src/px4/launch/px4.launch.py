from pathlib import Path
import os

from launch import LaunchDescription
from launch.actions import ExecuteProcess
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    project_dir = Path(
        get_package_share_directory('px4')
    ).parents[3]

    px4_dir = project_dir / 'src' / 'px4' / 'PX4-Autopilot'
    command = ['make', 'px4_sitl_zenoh', 'sihsim_ifc']
    dds_topics_file = os.environ.get('DDS_TOPICS_FILE', 'ifc_dds_topics.yaml')
    command.append(f'CMAKE_ARGS=-DDDS_TOPICS_FILE={dds_topics_file}')

    return LaunchDescription([
        ExecuteProcess(
            cmd=command,
            cwd=str(px4_dir),
            name='px4',
            output='screen',
        ),
    ])