import os

from launch import LaunchDescription
from launch.actions import ExecuteProcess


def generate_launch_description():
    px4_dir = os.environ.get('PX4_DIR')
    command = ['make', 'px4_sitl_zenoh', 'sihsim_ifc']
    dds_topics_file = os.environ.get('DDS_TOPICS_FILE', 'ifc_dds_topics.yaml')
    command.append(f'CMAKE_ARGS=-DDDS_TOPICS_FILE={dds_topics_file}')

    return LaunchDescription([
        ExecuteProcess(
            cmd=command,
            cwd=px4_dir,
            name='px4',
            output='screen',
        ),
    ])
