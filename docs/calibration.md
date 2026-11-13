# Calibration and Validation Record

## Current Status

No RG2, Compute Box, firmware-specific calibration procedure, or calibration
measurements have been supplied for this workspace. No calibration values are
defined or applied by the node. Hardware behavior and calibration remain
unverified; do not infer a procedure or enter estimated offsets.

The current `model: rg2` ROS parameter is only a logged label. It is not a
calibration selection. The node does not connect to hardware or issue commands.

## Keep These Concepts Separate

- **Gripper calibration:** a device procedure and measured result documented by
  OnRobot for the exact model and firmware. Follow only the applicable vendor
  procedure and approved safety controls.
- **Robot tool-frame calibration:** the tool center point and orientation in the
  robot's coordinate system. This belongs to the robot integration and its
  calibration process, not to the gripper transport.
- **Protocol conversion:** mapping verified register values to physical units.
  This is protocol implementation, not calibration. Derive and test it against
  the vendor manual; do not tune it from guessed values.

Do not store calibration records in `rg2.yaml`. If a future verified workflow
requires per-device calibration, keep it separate from protocol definitions,
identify the device it applies to, and validate it before use.

## Record Template

Create a dated record only after the applicable vendor procedure and safety
review are available. Preserve the source evidence with the record.

| Field | Record |
| --- | --- |
| Gripper model and serial number | Not recorded |
| Compute Box model and serial number | Not recorded |
| Firmware version | Not recorded |
| Vendor procedure/manual revision | Not recorded |
| Tool-changer topology | Not recorded |
| Date and operator | Not recorded |
| Robot model and tool frame, if applicable | Not recorded |
| Reference instrument and calibration status | Not recorded |
| Environment and test setup | Not recorded |
| Procedure steps and source | Not recorded |
| Raw observations and units | Not recorded |
| Derived values, uncertainty, and repeatability | Not recorded |
| Acceptance criteria and result | Not recorded |
| Safety review and approval reference | Not recorded |

## Safe Evidence Process

1. Identify the exact device, firmware, manual revision, and approved setup.
2. Record the procedure, reference equipment, units, uncertainty, and
	acceptance criteria before collecting measurements.
3. Perform only the vendor-documented procedure under the required supervision
	and independent safety controls. This repository provides no calibration
	motion sequence.
4. Preserve raw readings separately from converted values. Record any
	configuration changes, failures, recovery, and approval.
5. Keep the driver marked unverified until the record is reviewed and the
	corresponding implementation and tests are validated.

Loss of communication is an unknown physical state, not evidence that a
gripper is open, closed, released, stopped, or calibrated.
