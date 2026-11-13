# OnRobot RG descriptions

This package contains the upstream Xacro/URDF descriptions and mesh assets for
RG2 and RG6.

Expand a model to URDF after building the workspace and installing `xacro`:
```
ros2 run xacro xacro "$(ros2 pkg prefix --share onrobot_rg_description)/urdf/onrobot_rg2_model.xacro" > /tmp/onrobot_rg2.urdf
ros2 run xacro xacro "$(ros2 pkg prefix --share onrobot_rg_description)/urdf/onrobot_rg6_model.xacro" > /tmp/onrobot_rg6.urdf
```

The upstream ROS 1 `.launch` files are retained in the source tree but are not
installed. Gazebo plugin and transmission tags in the Xacro files have not been
ported or validated for Jazzy. Do not treat these descriptions as verified
collision or safety models.

Source: `kinisi-robotics/onrobot_grippers` at
`a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4`. The package and repository root
identify the content as MIT; the upstream license is included as `LICENSE`.
No separate mesh license or source attribution was found in the pinned tree.
