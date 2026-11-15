# OnRobot Grippers

This repository is the starting point for a ROS 2 Jazzy OnRobot gripper stack.
It currently contains a C++ read-only Modbus transport core and RG/VG robot
descriptions; see [ToDO.md](ToDO.md) for the safety requirements and remaining
work.

## Development container

Open this repository in VS Code and select **Dev Containers: Reopen in Container**.
The image uses ROS 2 Jazzy on Ubuntu 24.04, Python and C++ build tools, colcon,
rosdep, pytest, Xacro, Gazebo Harmonic through `ros_gz`, `gz_ros2_control`, and
libmodbus 3.1.10. It also includes `control_msgs` for the standard RG action.
The repository is mounted under `~/colcon_ws/src/`; the
VS Code workspace opens at `~/colcon_ws` so that sibling packages can be added.
New interactive Bash terminals source the workspace overlay if built, or the
Jazzy underlay otherwise.

Once packages exist, install their declared dependencies and build from the
workspace root:

```bash
rosdep update
rosdep install --from-paths $(colcon list --paths-only) --ignore-src --rosdistro jazzy -y
colcon build --symlink-install
colcon test
colcon test-result --verbose
```

Start a new terminal or run `source install/setup.bash` after building. The
control core currently provides serialized, bounded, read-only Modbus TCP
register access. The ROS polling adapter is disabled by default and publishes
raw register snapshots without assuming a model-specific map. It has no
hardware write path; use it with a real device only after verifying the exact
firmware manual, register range, and owner-approved safety setup.

The adapter requires an explicit parameter file with `enabled: true`, a numeric
IPv4 endpoint, and a register range taken from the exact firmware manual. It
publishes `onrobot_msgs/msg/RawRegisterSnapshot` on `raw_registers` and standard
`diagnostic_msgs/msg/DiagnosticArray` messages on `diagnostics`. Device and
endpoint parameters are read-only after startup. Register values remain raw
16-bit words; RG/VG status decoding is intentionally not inferred.

```bash
ros2 launch onrobot_control read_only_status_adapter.launch.py \
	namespace:=cell_a params_file:=/path/to/read_only_adapter.yaml
```

The installed example parameters leave polling disabled and the host empty.
The transport accepts numeric IPv4 addresses only. Polling, connect, and
response waits have configured upper bounds. Polling runs outside ROS callbacks;
shutdown may wait up to the configured response timeout for an in-flight read.
The node only reads holding registers; it exposes no command input and performs
no register writes.

The single RG server exposes `control_msgs/action/GripperCommand` at
`onrobot_rg/gripper_command`; the dual server exposes the `_A` and `_B` action
names. RG action positions are in meters and max effort is in newtons. The VG
server exposes `onrobot_msgs/action/VGCommand`, with separate mode and vacuum
percentage targets for both channels. Hardware-facing action servers reject
all goals because no verified hardware execution or feedback backend is
connected. `SetCommand.srv` remains generated only for compatibility and has no
active service server.

Gazebo RG2/RG6 launches start a separate simulation action bridge at
`/onrobot_rg2/gripper_command` or `/onrobot_rg6/gripper_command`. It maps
aperture linearly to the simulated revolute finger joint and waits for joint
feedback before completing a goal. This is a visualization/control
approximation, not a firmware or calibrated aperture model. The simulator does
not model gripper effort limits, so send `max_effort: 0`; positive effort goals
are rejected. VG10/VGC10 are fixed-link models with no suction or vacuum
dynamics, so their action goals remain unavailable in simulation.

The status listeners publish measured RG aperture on `rg_aperture/joint_states`
as a `sensor_msgs/msg/JointState` virtual prismatic coordinate in meters. The
default names are `rg_aperture`, `primary_rg_aperture`, and
`secondary_rg_aperture`; they can be overridden with read-only parameters.
These are aperture coordinates, not the revolute finger-joint positions in the
current URDF. Consumers must provide a matching virtual joint before using
these messages as robot-model joint state. Vendor status flags remain in the
OnRobot-specific input messages.

The `onrobot_gazebo` package launches RG2, RG6, VG10, or the four-cup VGC10 in
Gazebo Harmonic. RG2 and RG6 use ROS 2 control; VG10 and VGC10 are fixed-link
models for visualization and scene testing. Simulation is for software testing
only and does not validate physical gripper behavior or safety.

Launch either model with the default GUI, or run the simulation headlessly:

```bash
ros2 launch onrobot_gazebo bringup.launch.py gripper:=rg2
ros2 launch onrobot_gazebo bringup.launch.py gripper:=rg6
ros2 launch onrobot_gazebo bringup.launch.py gripper:=vg10
ros2 launch onrobot_gazebo bringup.launch.py gripper:=vgc10
ros2 launch onrobot_gazebo bringup.launch.py gripper:=rg2 gz_args:="-r -s"
```

MoveIt examples are available for all four grippers. RG2/RG6 launch standalone
planning and visualization demos; these do not execute trajectories. VG10/VGC10
are fixed-link tools, so their launches provide visualization for attachment
work, while the host arm supplies the planning group and end-effector semantics.

```bash
ros2 launch rg2_moveit_config demo.launch.py
ros2 launch rg6_moveit_config demo.launch.py
ros2 launch vg10_moveit_config visualize.launch.py
ros2 launch vgc10_moveit_config visualize.launch.py
```

The container uses Linux host networking for ROS 2 discovery and access to a
Compute Box on a reachable network. It runs in privileged mode, requests all
GPUs, adds host group ID `992`, forwards `DISPLAY`, and bind-mounts the host's
`~/.bash_history` and this repository's `.vscode` directory. These settings
grant broad host access and share shell history with the container; use them
only with trusted code and networks and choose `ROS_DOMAIN_ID` appropriately.
Before opening the container, ensure the bind-mount sources exist:

```bash
mkdir -p .vscode
touch ~/.bash_history
```

The host must have a working Docker GPU runtime and accessible GPUs; group ID
`992` must correspond to the intended host device group. Host networking,
GPU access, and the Linux-specific mounts need separate validation on Docker
Desktop or non-Linux hosts. Forwarding `DISPLAY` alone does not configure X11
socket access or authorization. Gazebo Harmonic, RViz, MoveIt 2, and the
joint-state publisher GUI are installed. No command is automatically sent to
hardware on container startup.

# Salute To

This project refer and builds upon the following repos,

- [kinisi-robotics/onrobot_grippers](https://github.com/kinisi-robotics/onrobot_grippers)
- [blooop/onrobot_grippers](https://github.com/blooop/onrobot_grippers)
- [BrettRD/onrobot_grippers](https://github.com/BrettRD/onrobot_grippers)
- [UOsaka-Harada-Laboratory/onrobot](https://github.com/UOsaka-Harada-Laboratory/onrobot)
- [cwsfa/onrobot-api](https://github.com/cwsfa/onrobot-api)