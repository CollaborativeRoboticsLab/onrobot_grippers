from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
	config_file = Path(get_package_share_directory("onrobot_control")) / "config" / "rg2.yaml"

	return LaunchDescription(
		[
			DeclareLaunchArgument("model", default_value="rg2"),
			DeclareLaunchArgument("namespace", default_value=""),
			Node(
				package="onrobot_control",
				executable="onrobot_control_node",
				name="onrobot_control",
				namespace=LaunchConfiguration("namespace"),
				output="screen",
				parameters=[str(config_file), {"model": LaunchConfiguration("model")}],
			),
		]
	)
