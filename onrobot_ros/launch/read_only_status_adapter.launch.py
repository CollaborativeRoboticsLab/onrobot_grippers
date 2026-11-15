from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument("namespace", default_value=""),
            DeclareLaunchArgument(
                "params_file",
                default_value=PathJoinSubstitution(
                    [FindPackageShare("onrobot_control"), "config", "read_only_status_adapter.yaml"]
                ),
            ),
            Node(
                package="onrobot_control",
                executable="onrobot_read_only_status_adapter",
                namespace=LaunchConfiguration("namespace"),
                parameters=[LaunchConfiguration("params_file")],
                output="screen",
            ),
        ]
    )