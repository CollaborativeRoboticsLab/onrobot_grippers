# DualChanger Legacy Reference

The legacy ROS-facing dual RG command and status nodes were removed after their
message-publishing and status-listening roles moved to the active C++ nodes in
`onrobot_control`. The active nodes do not connect to a gripper or changer.

`OnRobotRGTcpDualNode.py` remains only as historical protocol reference. It
contains an RG Compute Box power-cycle path and must not be run on hardware
without firmware verification, owner approval, and a reviewed safety plan. Do
not use the archived ROS 1 launch or service instructions.
