from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    namespace = LaunchConfiguration('namespace')
    primary_model = LaunchConfiguration('gripper_primary')
    secondary_model = LaunchConfiguration('gripper_secondary')

    return LaunchDescription([
        DeclareLaunchArgument('namespace', default_value=''),
        DeclareLaunchArgument('gripper_primary', default_value='rg2'),
        DeclareLaunchArgument('gripper_secondary', default_value='rg6'),
        Node(
            package='onrobot_control',
            executable='onrobot_dual_status_listener',
            name='onrobot_rg_dual_status_listener',
            namespace=namespace,
            output='screen',
        ),
        Node(
            package='onrobot_control',
            executable='onrobot_rg_dual_command_server',
            name='onrobot_rg_dual_command_server',
            namespace=namespace,
            output='screen',
            parameters=[{
                'gripper_primary': primary_model,
                'gripper_secondary': secondary_model,
            }],
        ),
    ])