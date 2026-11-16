import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    OpaqueFunction,
    RegisterEventHandler,
    SetEnvironmentVariable,
)
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import (
    Command,
    FindExecutable,
    LaunchConfiguration,
    PathJoinSubstitution,
)
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    gripper = LaunchConfiguration("gripper").perform(context)
    model_name = f"onrobot_{gripper}"
    is_rg_gripper = gripper in {"rg2", "rg6"}
    description_package = (
        "onrobot_rg_description" if is_rg_gripper else "onrobot_vg_description"
    )
    description_share = get_package_share_directory(description_package)
    resource_path = os.path.dirname(description_share)
    existing_resource_path = os.environ.get("GZ_SIM_RESOURCE_PATH")
    if existing_resource_path:
        resource_path = os.pathsep.join([resource_path, existing_resource_path])
    world_file = PathJoinSubstitution(
        [FindPackageShare("onrobot_gazebo"), "worlds", "empty.sdf"]
    )
    model_filename = (
        "onrobot_vgc10_4cups_model.xacro"
        if gripper == "vgc10"
        else f"{model_name}_model.xacro"
    )
    model_file = PathJoinSubstitution(
        [
            FindPackageShare(description_package),
            "urdf",
            model_filename,
        ]
    )
    robot_description = Command([FindExecutable(name="xacro"), " ", model_file])

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution(
                [FindPackageShare("ros_gz_sim"), "launch", "gz_sim.launch.py"]
            )
        ),
        launch_arguments={
            "gz_args": [LaunchConfiguration("gz_args"), " ", world_file]
        }.items(),
    )
    state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[{"robot_description": robot_description, "use_sim_time": True}],
        remappings=[
            ("joint_states", f"/{model_name}/joint_states"),
            ("robot_description", f"/{model_name}/robot_description"),
        ],
        output="screen",
    )
    spawn_gripper = Node(
        package="ros_gz_sim",
        executable="create",
        arguments=[
            "-topic",
            f"/{model_name}/robot_description",
            "-name",
            model_name,
            "-z",
            "0.1",
        ],
        output="screen",
    )
    controller_manager = f"/{model_name}/controller_manager"
    clock_bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        arguments=["/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock"],
        output="screen",
    )
    spawn_actions = [spawn_gripper]
    if is_rg_gripper:
        joint_state_spawner = Node(
            package="controller_manager",
            executable="spawner",
            arguments=[
                "joint_state_broadcaster",
                "--controller-manager",
                controller_manager,
                "--controller-manager-timeout",
                "120",
            ],
            output="screen",
        )
        position_spawner = Node(
            package="controller_manager",
            executable="spawner",
            arguments=[
                "joint_position_controller",
                "--controller-manager",
                controller_manager,
                "--controller-manager-timeout",
                "120",
            ],
            output="screen",
        )
        simulated_action_server = Node(
            package="onrobot_control",
            executable="onrobot_simulated_rg_action_server",
            parameters=[{"model": gripper, "use_sim_time": True}],
            remappings=[
                ("joint_position_controller/commands", f"/{model_name}/joint_position_controller/commands"),
                ("joint_states", f"/{model_name}/joint_states"),
            ],
            output="screen",
        )
        spawn_actions = [
            RegisterEventHandler(
                OnProcessExit(
                    target_action=spawn_gripper,
                    on_exit=[joint_state_spawner, position_spawner, simulated_action_server],
                )
            ),
            spawn_gripper,
        ]

    return [
        SetEnvironmentVariable(
            name="GZ_SIM_RESOURCE_PATH",
            value=resource_path,
        ),
        gazebo,
        clock_bridge,
        state_publisher,
        *spawn_actions,
    ]


def generate_launch_description():
    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "gripper",
                default_value="rg2",
                choices=["rg2", "rg6", "vg10", "vgc10"],
            ),
            DeclareLaunchArgument("gz_args", default_value="-r"),
            OpaqueFunction(function=launch_setup),
        ]
    )
