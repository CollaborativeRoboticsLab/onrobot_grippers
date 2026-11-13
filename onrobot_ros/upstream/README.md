# Staged Upstream Source

`kinisi-a83cd73/` is the complete tracked source tree from
`kinisi-robotics/onrobot_grippers` at commit
`a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4`. Its upstream MIT `LICENSE` and
README are preserved with the snapshot.

This is a source-import stage, not a Jazzy port. It contains RG/VG control,
Modbus TCP, message, description, and Gazebo packages, a shared Modbus header,
and the original metapackage. The upstream README describes ROS 2 Humble, while
the metapackage uses ROS 1 `catkin`; dependencies and runtime behavior have not
been validated here. `COLCON_IGNORE` keeps this snapshot out of normal builds
until the packages are reviewed and refactored.

Do not launch its hardware-control nodes until their protocol and safety
behavior are reviewed for the exact device and firmware. The project's active
description packages remain in `onrobot_description/`, and the active
read-only transport remains in `onrobot_control/`.