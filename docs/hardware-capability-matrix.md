# Hardware Capability and Validation Matrix

## Status

This is a recording template, not evidence of hardware support. No device or
firmware combination is verified by this document. Keep hardware writes and
action execution disabled until the exact combination has primary-source
documentation, owner approval, and supervised validation.

Create a separate completed record for each exact gripper, Compute Box, and
firmware combination. Do not treat results from one model or firmware as
evidence for another. Use `Unknown` when a fact has not been verified; unknown
protocol or safety behavior blocks writes.

## Device and Evidence Identity

| Field | Record |
| --- | --- |
| Gripper model, serial number, and hardware revision | Not recorded |
| Compute Box model, serial number, and hardware revision | Not recorded |
| Gripper firmware version | Not recorded |
| Compute Box firmware version | Not recorded |
| Vendor manual and procedure revisions | Not recorded |
| Network, Modbus, and tool-changer topology | Not recorded |
| Intended ROS consumers and namespace | Not recorded |
| Evidence location and revision/hash | Not recorded |
| Record owner and date | Not recorded |
| Risk assessment and approval reference | Not recorded |

Do not record network credentials or other secrets in this document.

## Capability Evidence

For each row, record the documented value, units, source page or section, and
how it was independently verified. Separate vendor-documented behavior from
bench observations. Do not fill gaps with values inferred from another model.

| Capability | Documented value and units | Evidence reference | Verification result and limits |
| --- | --- | --- | --- |
| Transport, addressing, and connection limits | Unknown | Not recorded | Not verified |
| Register map, access type, and address base | Unknown | Not recorded | Not verified |
| Encoding, signedness, byte/word order, and scaling | Unknown | Not recorded | Not verified |
| Position, force, speed, and vacuum units and limits | Unknown | Not recorded | Not verified |
| Status, busy, grip-detected, and fault semantics | Unknown | Not recorded | Not verified |
| Command sequencing and acknowledgement | Unknown | Not recorded | Not verified |
| Stop, cancellation, and command replay behavior | Unknown | Not recorded | Not verified |
| Vacuum release behavior, if applicable | Unknown | Not recorded | Not verified |
| Power reset behavior and authorization, if applicable | Unknown | Not recorded | Not verified |
| Timeout, communication loss, and recovery behavior | Unknown | Not recorded | Not verified |
| Simultaneous goals and device ownership behavior | Unknown | Not recorded | Not verified |

## Supervised Validation Record

Complete only after the exact vendor procedure and owner-approved risk controls
are available. Follow the approved procedure; this template does not prescribe
motion commands or safety limits.

| Field | Record |
| --- | --- |
| Test location and isolation from production | Not recorded |
| Operator and supervising owner | Not recorded |
| Independent emergency stop and verification | Not recorded |
| Verified conservative limits and source | Not recorded |
| Initial read-only checks and raw evidence | Not recorded |
| Individually authorized test and approval reference | Not recorded |
| Observed state, units, timestamps, and expected result | Not recorded |
| Cancellation, timeout, communication-loss, and recovery outcome | Not recorded |
| Errors, deviations, and unresolved behavior | Not recorded |
| Final disposition and reviewer sign-off | Not recorded |

Automatic hardware tests do not belong in normal CI. A successful simulation or
fake-backed test is not evidence of physical behavior or functional safety.

## Support Decision

| Exact model and firmware combination | Supported capabilities | Unsupported or unknown behavior | Decision and approver |
| --- | --- | --- | --- |
| Not recorded | None verified | All capabilities | Not approved |

Do not enable a hardware command path while required evidence, safety review,
or approval is missing. Communication failure means the physical state is
unknown; software cancellation is not a safety-rated stop.