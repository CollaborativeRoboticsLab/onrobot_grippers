from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
	return LaunchDescription([
		DeclareLaunchArgument("model", default_value="rg2"),
		DeclareLaunchArgument("namespace", default_value=""),
		Node(
			package="onrobot_control",
			executable="onrobot_status_listener",
			name="onrobot_status_listener",
			namespace=LaunchConfiguration("namespace"),
			output="screen",
		),
		Node(
			package="onrobot_control",
			executable="onrobot_rg_command_server",
			name="onrobot_rg_command_server",
			namespace=LaunchConfiguration("namespace"),
			output="screen",
			parameters=[{"gripper_model": LaunchConfiguration("model")}],
		),
	])
