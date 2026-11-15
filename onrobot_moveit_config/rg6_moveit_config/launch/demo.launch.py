from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from moveit_configs_utils import MoveItConfigsBuilder
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    package_name = "rg6_moveit_config"
    moveit_config = (
        MoveItConfigsBuilder("onrobot_rg6_model", package_name=package_name)
        .robot_description()
        .robot_description_semantic()
        .joint_limits()
        .planning_pipelines(pipelines=["ompl"])
        .trajectory_execution(
            file_path="config/trajectory_execution.yaml",
            moveit_manage_controllers=False,
        )
        .to_moveit_configs()
    )
    description = moveit_config.robot_description
    rviz_config = os.path.join(
        get_package_share_directory(package_name), "config", "planning.rviz"
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument("rviz", default_value="true"),
            Node(
                package="joint_state_publisher_gui",
                executable="joint_state_publisher_gui",
                name="joint_state_publisher_gui",
                parameters=[description],
                output="screen",
            ),
            Node(
                package="robot_state_publisher",
                executable="robot_state_publisher",
                parameters=[description],
                output="screen",
            ),
            Node(
                package="moveit_ros_move_group",
                executable="move_group",
                output="screen",
                parameters=[moveit_config.to_dict(), {"allow_trajectory_execution": False}],
            ),
            Node(
                package="rviz2",
                executable="rviz2",
                arguments=["-d", rviz_config],
                parameters=[moveit_config.to_dict()],
                condition=IfCondition(LaunchConfiguration("rviz")),
                output="screen",
            ),
        ]
    )