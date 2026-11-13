from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'ip',
            default_value='192.168.1.1',
            description='IP address'
        ),
        DeclareLaunchArgument(
            'port',
            default_value='502',
            description='Port number'
        ),
        DeclareLaunchArgument(
            'changer_addr',
            default_value='65',
            description='Changer address'
        ),
        DeclareLaunchArgument(
            'dummy',
            default_value='false',
            description='Dummy mode'
        ),

        Node(
            package='onrobot_vg_control',
            executable='OnRobotVGStatusListener.py',
            name='OnRobotVGStatusListener',
            output='screen',
            parameters=[{
                'ip': LaunchConfiguration('ip'),
                'port': LaunchConfiguration('port'),
                'changer_addr': LaunchConfiguration('changer_addr'),
                'dummy': LaunchConfiguration('dummy')
            }]
        ),

        Node(
            package='onrobot_vg_control',
            executable='OnRobotVGTcpNode.py',
            name='OnRobotVGTcpNode',
            output='screen'
        ),

        Node(
            package='onrobot_vg_control',
            executable='OnRobotVGSimpleControllerServer.py',
            name='OnRobotVGSimpleControllerServer',
            output='screen'
        )
    ])
