# Remaining work: ROS 2 Jazzy OnRobot stack

This is not a claim that the protocol or hardware has been validated. RG2 is
the first acceptance target; RG6 and other devices are follow-up targets.
The current command services publish ROS messages only, and the transport is
read-only. The dual-gripper launch does not connect to a changer, and the
legacy power-cycle service remains disabled.

## Protocol and safety prerequisites

- [ ] Obtain the exact RG2 and Compute Box firmware versions, vendor manual
  revision, network and tool-changer topology, intended ROS consumers, hardware
  access, and owner-approved risk assessment.
- [ ] Verify addressing, register maps, scaling, units, limits, status bits,
  command sequencing, and the support and safety of stop, cancellation, vacuum
  release, and power reset. Do not assume maps or semantics are shared across
  models or firmware.
- [ ] Record a traceable model/firmware capability matrix, evidence, and
  unsupported combinations. Obtain owner approval before implementing or
  enabling hardware writes. Unknown details block hardware writes.

## Interfaces and offline core

- [ ] Verify imported message semantics against the selected firmware. Define
  typed interfaces, units, bounds, outcomes, namespaces, timestamps, versioning,
  ownership, concurrency, feedback, timeouts, and cancellation.
- [ ] Evaluate `control_msgs/action/GripperCommand` for RG2. If adopted, declare
  the dependency and verify a clean Jazzy build; establish bounds, conversions,
  truthful feedback/result flags, and failure and concurrent-goal semantics
  from the verified protocol before implementing the adapter.
- [ ] Add firmware-backed model-specific protocol vectors, status decoding, and
  deterministic offline tests, including malformed responses. Do not add writes
  before the protocol and safety gates are met.

## ROS adapter and validation

- [x] Add a disabled-by-default, independently testable read-only ROS adapter
  with model and endpoint parameters, diagnostics, configurable polling,
  bounded execution, orderly shutdown, and startup validation before I/O.
- [x] Cover fake-backed config rejection, disabled startup, read faults and
  timeout, reconnect, raw snapshots, multiple namespaces, responsive callbacks,
  and orderly shutdown. There is no command input or physical motion claim.
- [ ] Before adding any command/action path, define and test command
  cancellation and replay semantics against the verified firmware protocol.
- [ ] After owner approval and a risk assessment, validate the exact hardware and
  firmware on an isolated, supervised setup with conservative verified limits
  and an independent emergency stop. Start read-only, then use individually
  authorized motions; record versions, results, errors, and recovery. Keep
  automatic hardware tests out of normal CI.
- [ ] Add each RG6, VG, or other device only with its own evidence, capability
  matrix, fake tests, and hardware acceptance. Scope dual hardware, robot
  integration, and ros2_control separately.

## Engineering and review

- [ ] Prefer explicit state machines and typed commands. Specify simultaneous
  goal handling, avoid indefinitely blocking callbacks, and use SI units at ROS
  interfaces with conversions at the protocol boundary.
- [ ] Treat communication failure as unknown physical state. Do not claim
  cancellation physically stops the device without evidence; software stop is
  not a safety-rated emergency stop, and simulation does not validate safety.
- [ ] Test overflow, rounding, out-of-range values, and NaN/Inf. Declare
  dependencies, document verified versions, and keep optional tools separate.
- [x] Add hardware-free CI for clean Jazzy builds, C++ formatting, unit and
  fake-backed integration tests, and `colcon test-result --verbose`.
- [x] Document adapter launch, parameters, topics, disabled default, and
  read-only/raw-data limitations.
- [ ] Before review, provide a per-phase change summary, provenance, test
  results, supported model/firmware matrix, unresolved safety assumptions, and
  hardware evidence. Trace validation through transport, feedback, recovery,
  lifecycle, and multi-instance behavior.

## Reference and provenance notes

The Kinisi snapshot at `a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4` was
reviewed as a ROS 2 Humble reference, not a verified Jazzy or protocol authority.
The upstream MIT notice must be preserved for adopted material. The legacy
Python transports use an obsolete `pymodbus.client.sync` API and include an
unapproved power-cycle path. Mesh provenance and model accuracy remain
unverified; do not copy assets without checking their separate rights.

See [the secondary upstream comparison](docs/upstream-comparison.md) for
checked revisions, licenses, and reuse decisions. No cwsfa code or decoded
content was retained without redistribution permission. Review licenses and
record exact source revisions before adopting any community material.