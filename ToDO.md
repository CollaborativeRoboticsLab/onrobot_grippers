# Remaining hardware work: ROS 2 Jazzy OnRobot stack

This checklist covers device-specific evidence and physical acceptance only.
It is not a claim that the protocol or hardware has been validated. Hardware-
facing RG and VG action endpoints reject all goals because no verified hardware
execution and feedback backend is connected. The transport remains read-only;
the RG Gazebo bridge is position-only simulation and does not validate physical
behavior or safety.

## RG2 acceptance gate

- [ ] Record the exact RG2 and Compute Box models, hardware revisions, serial
  numbers, firmware versions, vendor manual revisions, network and tool-changer
  topology, intended ROS consumers, and accountable device owner.
- [ ] Complete a traceable record for this exact combination using the
  [hardware capability and validation matrix](docs/hardware-capability-matrix.md).
  Cite primary-source evidence for addressing, register maps, encoding, units,
  scaling, limits, status flags, command sequencing, feedback, and recovery.
  Mark unknown behavior as unsupported; do not infer semantics from another
  model or firmware.
- [ ] Obtain an owner-approved risk assessment and written authorization before
  any hardware write or motion. Confirm the isolated test setup, supervision,
  verified conservative limits, and independent emergency stop.
- [ ] Verify device-specific stop, cancellation, command replay, timeout,
  communication-loss, and recovery behavior. Verify power-reset and vacuum-
  release behavior only if applicable and explicitly authorized.
- [ ] After approval, begin with read-only checks. Perform only individually
  authorized hardware tests under the approved procedure; record firmware,
  configuration, raw observations, outcomes, errors, and recovery in the matrix.
  Do not add automatic physical-device tests to normal CI.
- [ ] Review the evidence and obtain explicit owner approval for each supported
  capability before enabling any hardware execution path. Until then, keep
  action goals rejected and the transport read-only.

## Additional devices

- [ ] Validate RG6, VG, and any other device as separate model/firmware
  combinations, each with its own vendor evidence, capability record, risk
  approval, and supervised hardware acceptance. Do not assume RG2 behavior
  applies to another model.
- [ ] Scope dual-gripper hardware, robot integration, and ros2_control support
  separately; require device-specific evidence and approval for each.

Communication failure means the physical state is unknown. Software
cancellation is not a safety-rated emergency stop, and simulation does not
validate physical behavior or safety.