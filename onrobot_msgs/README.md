# OnRobot Messages

This package combines OnRobot-specific register messages with the typed
two-channel `VGCommand` action. RG control uses the standard
`control_msgs/action/GripperCommand` interface from the control package.
`OnRobotRGInput`, `OnRobotRGOutput`, `OnRobotVGInput`, and `OnRobotVGOutput`
retain their legacy register-field names and types for low-level compatibility.
`SetCommand.srv` remains generated for compatibility only; no active control
node provides it.

The interface comments preserve provenance and known units, but register
semantics, model-specific bounds, and status interpretation have not been
verified against the selected device firmware. The current action servers reject
all goals until an execution backend can report truthful device feedback; they
do not publish hardware command messages. The `RawRegisterSnapshot` message is
for un-decoded diagnostics, not a model-specific status contract.