# OnRobot Grippers

This repository is the starting point for a ROS 2 Jazzy OnRobot gripper stack.
The driver is not implemented yet; see the [implementation plan](docs/implementation-plan.md)
for the proposed architecture, safety requirements, and review gates.

## Development container

Open this repository in VS Code and select **Dev Containers: Reopen in Container**.
The image uses ROS 2 Jazzy on Ubuntu 24.04, Python and C++ build tools, colcon,
rosdep, pytest, and Xacro. The repository is mounted under `~/colcon_ws/src/`; the
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
container does not install a Modbus library until a supported version is chosen
and declared by the driver.

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
