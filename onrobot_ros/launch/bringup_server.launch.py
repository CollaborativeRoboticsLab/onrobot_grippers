from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
  namespace = LaunchConfiguration('namespace')
  gripper =   LaunchConfiguration('gripper')

  ns_arg =      DeclareLaunchArgument('namespace', default_value='')
  gripper_arg = DeclareLaunchArgument("gripper",   default_value="rg6")

  status = Node(
    name='onrobot_status_listener',
    package='onrobot_control',
    namespace=namespace,
    executable='onrobot_status_listener',
    output='screen'
  )

  control = Node(
    name='onrobot_rg_command_server',
    package='onrobot_control',
    namespace=namespace,
    executable='onrobot_rg_command_server',
    output='screen',
    parameters=[{
      'gripper_model': gripper,
    }]
  )

  return LaunchDescription([
    ns_arg, gripper_arg, status, control])