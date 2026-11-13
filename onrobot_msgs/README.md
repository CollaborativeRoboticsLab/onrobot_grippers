# OnRobot Messages

This package combines the RG and VG interfaces from the pinned Kinisi ROS 2
snapshot. `OnRobotRGInput`, `OnRobotRGOutput`, `OnRobotVGInput`, and
`OnRobotVGOutput` retain their upstream field names and types. The identical RG
and VG `SetCommand` services are represented once.

The interface comments preserve provenance and known units, but register
semantics, model-specific bounds, and status interpretation have not been
verified against the selected device firmware. These interfaces are definitions
only; the active control node does not consume them yet. Do not infer successful
physical motion from a service response.