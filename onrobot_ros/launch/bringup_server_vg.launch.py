from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='onrobot_control',
            executable='onrobot_status_listener',
            name='onrobot_status_listener',
            output='screen'
        ),
        Node(
            package='onrobot_control',
            executable='onrobot_vg_command_server',
            name='onrobot_vg_command_server',
            output='screen'
        )
    ])
