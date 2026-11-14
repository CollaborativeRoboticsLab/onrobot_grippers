# OnRobot Grippers

This repository is the starting point for a ROS 2 Jazzy OnRobot gripper stack.
It currently contains a C++ read-only Modbus transport core and RG/VG robot
descriptions; see [ToDO.md](ToDO.md) for the safety requirements and remaining
work.

## Development container

Open this repository in VS Code and select **Dev Containers: Reopen in Container**.
The image uses ROS 2 Jazzy on Ubuntu 24.04, Python and C++ build tools, colcon,
rosdep, pytest, Xacro, and libmodbus 3.1.10. The repository is mounted under `~/colcon_ws/src/`; the
VS Code workspace opens at `~/colcon_ws` so that sibling packages can be added.
New interactive Bash terminals source the workspace overlay if built, or the
Jazzy underlay otherwise.

Once packages exist, install their declared dependencies and build from the
workspace root:

```bash
rosdep update
rosdep install --from-paths src --ignore-src --rosdistro jazzy -y
colcon build --symlink-install
colcon test
colcon test-result --verbose
```

Start a new terminal or run `source install/setup.bash` after building. The
control core currently provides serialized, bounded, read-only Modbus TCP
register access. It has no hardware write path; do not connect it to a gripper
until the exact firmware protocol and safety setup are approved.

The single and dual RG command servers select gripper models at startup.
Their `gripper_model`, `gripper_primary`, and `gripper_secondary` parameters
are read-only at runtime; restart the corresponding node to change a model.

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
socket access or authorization. RViz, MoveIt, simulation, and other optional
stacks are not installed. No command is automatically sent to hardware on
container startup.

# Salute To

This project refer and builds upon the following repos,

- [kinisi-robotics/onrobot_grippers](https://github.com/kinisi-robotics/onrobot_grippers)
- [blooop/onrobot_grippers](https://github.com/blooop/onrobot_grippers)
- [BrettRD/onrobot_grippers](https://github.com/BrettRD/onrobot_grippers)
- [UOsaka-Harada-Laboratory/onrobot](https://github.com/UOsaka-Harada-Laboratory/onrobot)
- [cwsfa/onrobot-api](https://github.com/cwsfa/onrobot-api)