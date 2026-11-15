# MoveIt Examples

The RG2 and RG6 packages provide standalone MoveIt planning and RViz examples.
They use the existing description Xacros, a joint-state GUI, and OMPL. Planning
is visualization-only: trajectory execution is disabled because the current
Gazebo controllers expose position commands, not FollowJointTrajectory actions.

```bash
ros2 launch rg2_moveit_config demo.launch.py
ros2 launch rg6_moveit_config demo.launch.py
```

Pass `rviz:=false` to run the RG demo without RViz. The joint-state GUI remains
available for changing the single active finger joint.

VG10 and VGC10 have no movable joints, so their packages launch static model
visualization only. Their SRDF files intentionally define no planning group or
end-effector parent; those belong in the host arm's SRDF when a tool is
integrated. The tool should be attached to a chosen host-arm link and modeled
as fixed in the combined robot description.

```bash
ros2 launch vg10_moveit_config visualize.launch.py
ros2 launch vgc10_moveit_config visualize.launch.py
```