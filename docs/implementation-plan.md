# ROS 2 Jazzy OnRobot stack: implementation plan

This is a handoff for the implementation agent and a checklist for the later
review. It is a plan, not a claim that hardware, protocols, or packages have
been validated. Implement in small phases; do not copy community code without
reviewing its license and preserving all required notices.

## Reference assessment

Prioritize the [Kinisi fork](https://github.com/kinisi-robotics/onrobot_grippers)
at `a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4`: its README describes a ROS 2
Humble port of RG and VG packages, not verified Jazzy support. Its root LICENSE
is MIT. It is useful for package boundaries and behavior to investigate, not
an authoritative protocol specification.

* [blooop fork](https://github.com/blooop/onrobot_grippers) describes the same
  Humble RG/VG port; compare differences rather than assuming it is identical.
* [BrettRD fork](https://github.com/BrettRD/onrobot_grippers): Kinisi credits it
  for the ROS 2 RG port. Its inherited README still describes Noetic, so inspect
  package metadata and source rather than relying on that README.
* [UOsaka project](https://github.com/UOsaka-Harada-Laboratory/onrobot) documents
  ROS Noetic RG2/RG6 and VG10/VGC10 support and a dual changer configuration.
* [cwsfa Python API](https://github.com/cwsfa/onrobot-api) documents RG2, 2FG7,
  and VGC10 examples. No root license was visible in the repository listing
  checked; verify rights and provenance before reuse. Treat generated/decoded
  material with particular care.

These are related projects, not independent confirmation that a register or
safety behavior is correct. Record the exact reference commit and license for
any adopted material. Do not copy bundled vendor or mesh assets without checking
their separate provenance and redistribution terms.

## Decisions required before hardware implementation

Obtain from the owner: first gripper model(s), Compute Box and firmware versions,
network topology, single/dual tool changer and connection, access to the relevant
vendor protocol/manual and hardware, intended ROS consumers, and safety/risk
assessment. Decide which model is the first acceptance target; do not infer
that RG and VG or different firmware versions share register maps. Confirm
whether physical stop, cancellation, vacuum release, and power reset are
supported and safe for that setup. Block hardware writes if these are unknown.
Record choices, evidence, and unsupported combinations in the documentation.

## Phases and acceptance gates

1. **Inventory and protocol evidence.** Inspect the prioritized upstream at a
   pinned commit, package metadata, launch files, dependencies, and available
   licenses. Cross-check addressing, register maps, scaling, units, limits,
   status bits, and command sequencing against the vendor documentation for
   the selected firmware. Create a traceable model/firmware capability matrix;
   label any unsupported or unverified behavior. Exit: owner approves the first
   model and the evidence is sufficient for a testable protocol contract.
2. **Workspace and interfaces.** Create conventional Jazzy packages and
   manifests; keep generated ROS interfaces in an `ament_cmake` interface
   package and Python nodes in `ament_python` packages. Separate protocol
   transport, model-specific codec/state logic, and ROS adapters so the latter
   can later be replaced by C++ without changing the public contract. Choose
   standard ROS interfaces where they faithfully express the chosen device;
   otherwise specify typed custom interfaces, units, bounds, outcomes,
   namespacing, timestamps, and versioning. Define concurrency, timeouts,
   feedback, cancellation and ownership semantics before implementing them.
   Exit: documented interface contract and successful clean colcon build.
3. **Offline protocol core.** Select and declare a Jazzy/Ubuntu-compatible
   Modbus library/version after verifying its API and license; do not inherit
   old pymodbus pins blindly. Implement bounded transport operations, explicit
   connection and protocol errors, validated conversions, and a single
   serialized access path to each physical endpoint. Provide an injectable fake
   transport. Exit: unit tests cover known register vectors, boundaries, status
   decoding, malformed responses, timeouts, disconnects, and concurrency
   without requiring network or hardware.
4. **ROS adapter and bringup.** Expose the agreed command and state interfaces
   with model and endpoint parameters, clear diagnostics, configurable polling,
   bounded execution, and orderly shutdown. Reject invalid or unsupported
   commands before I/O. Never automatically replay commands after reconnect;
   do not equate transport success with successful physical motion. Do not
   claim cancellation physically stops the device unless verified. Exit:
   launch/integration tests against a fake endpoint cover success, rejection,
   faults, disconnect/reconnect, timeout, cancellation, and shutdown; check
   multiple namespaces and that callbacks remain responsive.
5. **Hardware validation.** Only after owner approval and an appropriate risk
   assessment, test the exact model and firmware on an isolated, supervised
   setup. Use conservative verified limits, keep personnel clear, and make an
   independent emergency stop available. Start with read-only checks, then
   individually authorized motions; record versions, test results, observed
   errors, and recovery. No automatic hardware tests in normal CI. Exit:
   a reproducible, reviewed hardware test report; otherwise explicitly label
   the stack as unverified on hardware.
6. **Expand and integrate.** Add each additional RG/VG/other device only with
   its own evidence, capability matrix, fake tests and hardware acceptance.
   Treat dual grippers, robot integration, models, RViz, simulation, MoveIt
   and ros2_control as separately scoped milestones, not prerequisites for a
   first driver. Do not imply simulation validates physical safety.
7. **C++ migration.** After the Python behavior and interfaces are stable,
   port transport/model logic and ROS adapter incrementally. Run the same
   protocol vectors, integration scenarios, and hardware acceptance against
   both implementations. Preserve interface names, units, documented error
   semantics and configuration, or explicitly version breaking changes.
   Avoid promising real-time behavior without a separate design and evidence.

## Engineering and review guidelines

* Prefer explicit state machines and typed commands over magic strings or
  model-dependent hidden defaults. Define a policy for simultaneous goals and
  avoid blocking ROS executor callbacks indefinitely.
* Treat a communication failure as unknown physical state, not as an open,
  closed, released, stopped, or successful state. Reconnection must not move,
  release, or reset hardware without a new explicit request. A software stop
  is not a safety-rated emergency stop.
* Specify ranges and SI units in ROS-facing contracts and convert only at the
  protocol boundary. Test overflow, rounding, out-of-range values and NaN/Inf.
  Do not assume force, vacuum, or object-detection semantics across models.
* Declare all runtime/build/test dependencies in package metadata and use rosdep.
  Keep the base container lean; make optional tools separate. Pin or constrain
  dependencies where needed and document verified versions.
* Add CI for clean Jazzy build, lint, unit and fake-backed integration tests;
  include `colcon test-result --verbose`. CI must not connect to real hardware.
  Document exact commands, parameters, examples, limitations and failure modes.
* Before requesting review, provide a per-phase change summary, source and
  license provenance, test output, supported model/firmware matrix, unresolved
  safety assumptions and any actual hardware test evidence. The review should
  trace every command through validation, serialization, transport, response,
  state reporting and failure recovery; inspect lifecycle and multi-instance
  behavior as well as Python-to-C++ compatibility.
