# OnRobot VG descriptions

This package contains the upstream Xacro/URDF descriptions and mesh assets for
VG10 and VGC10 in one-cup and four-cup configurations.

Expand models to URDF after building the workspace and installing `xacro`:
```
ros2 run xacro xacro "$(ros2 pkg prefix --share onrobot_vg_description)/urdf/onrobot_vg10_model.xacro" > /tmp/onrobot_vg10.urdf
ros2 run xacro xacro "$(ros2 pkg prefix --share onrobot_vg_description)/urdf/onrobot_vgc10_1cup_model.xacro" > /tmp/onrobot_vgc10_1cup.urdf
ros2 run xacro xacro "$(ros2 pkg prefix --share onrobot_vg_description)/urdf/onrobot_vgc10_4cups_model.xacro" > /tmp/onrobot_vgc10_4cups.urdf
```

The upstream ROS 1 `.launch` files are retained in the source tree but are not
installed. Gazebo plugin and transmission tags in the Xacro files have not been
ported or validated for Jazzy. Do not treat these descriptions as verified
collision or safety models.

Source: `kinisi-robotics/onrobot_grippers` at
`a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4`. The package and repository root
identify the content as MIT; the upstream license is included as `LICENSE`.
No separate mesh license or source attribution was found in the pinned tree.
