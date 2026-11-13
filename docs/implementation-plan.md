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

### Pinned Kinisi snapshot: static findings

Inspected commit `a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4` on 2026-11-13.
This was a source-tree review only; it was not built, tested, or connected to
hardware.

* The README calls this a ROS 2 Humble port and says the VG packages were added
  to the RG port. The tree separates RG/VG control, description, Modbus TCP,
  and message packages. The README also retains ROS 1 instructions and package
  material, so it is not a clean Jazzy baseline.
* Both RG and VG Modbus transports import
  `pymodbus.client.sync.ModbusTcpClient`, while their package manifests do not
  declare a `pymodbus` dependency. Treat this as a dependency/API compatibility
  warning; no current Jazzy compatibility conclusion has been established.
* The RG `SetCommand` service accepts a string and declares `success` and
  `message` response fields, but the inspected server currently returns `None`
  for both fields (`TODO: implement`). Its command parsing is not a suitable
  verified public contract to reuse as-is.
* The RG transport contains a Compute Box power-cycle write path. Do not carry
  it forward without vendor documentation and owner approval for the selected
  hardware and safety setup.
* The root LICENSE and both RG/VG description package manifests identify MIT;
  no separate mesh license or source notice was found in the pinned description
  trees. Sample mesh history records commit `aa7874ff1f6c8e1212f5e94f0773d474b7aded45`
  by Takuya Kiyokawa as a package reorganization, but does not establish the
  original CAD authorship. The description packages were transferred with the
  upstream license notice; mesh provenance and model accuracy remain unverified.

This snapshot review does not satisfy phase 1's protocol-evidence gate. The
RG2 is selected as the first target, but the firmware and vendor protocol
evidence are not available, and no hardware validation has been supplied.

### Owner input status

As of 2026-11-13, the owner has selected RG2 as the first acceptance target,
with RG6 deferred. Compute Box and firmware versions, tool-changer topology, and
vendor manual revision are unknown. “Latest” is not a sufficiently specific
firmware target for a capability matrix or protocol contract. Do not enable
hardware writes until these details and the required safety approval are
recorded.

## Decisions required before hardware implementation

RG2 is selected as the first acceptance target. Obtain the Compute Box and
firmware versions, network topology, tool-changer topology, relevant vendor
protocol/manual and hardware access, intended ROS consumers, and safety/risk
assessment. Do not infer that RG and VG or different firmware versions share
register maps. Confirm whether physical stop, cancellation, vacuum release, and
power reset are supported and safe for that setup. Block hardware writes if
these are unknown. Record choices, evidence, and unsupported combinations.

## Phases and acceptance gates

1. **Inventory and protocol evidence.** Obtain the exact RG2 Compute Box and
  firmware versions, tool-changer topology, and vendor manual. Cross-check
  addressing, register maps, scaling, units, limits, status bits, and command
  sequencing against that documentation. Create a traceable model/firmware
  capability matrix and label unsupported or unverified behavior. Exit:
  evidence is sufficient for a testable protocol contract and owner approval.
2. **Workspace and interfaces.** Complete conventional Jazzy packaging and
  dependency exports for the existing C++ node and transport. Keep generated
  ROS interfaces in an `ament_cmake` interface package. Add model-specific
  codec/state logic and ROS adapters as separate, independently testable
  layers. Choose standard ROS interfaces where they faithfully express the
  device; otherwise specify typed custom interfaces, units, bounds, outcomes,
  namespacing, timestamps, and versioning. Define concurrency, timeouts,
  feedback, cancellation, and ownership semantics before implementation.
  Exit: documented interface contract and successful clean colcon build.
    RG2-first C++ interface proposal: use Jazzy's
    `control_msgs/action/GripperCommand`, whose goal carries a target gap
    (`position`) and `max_effort`, and whose feedback/result carry measured
    position and effort plus `stalled` and `reached_goal` flags. The standard
    action specifies position in meters and feedback effort in Newtons. Declare
    `control_msgs` as a package dependency and verify a clean Jazzy build; it is
    not installed in the current container. This proposal does not define RG2
    bounds or conversions, which hardware states can truthfully populate the
    flags, or failure, timeout, cancellation, and concurrent-goal semantics.
    Verify those against the exact firmware and vendor documentation before
    implementing a hardware adapter. RG6 remains a separate follow-up target.
3. **Offline protocol core.** Add model-specific vectors, status decoding, and
  approved protocol tests based on the verified RG2 protocol contract. Extend
  malformed-response coverage as required by that contract. Keep tests
  independent of real hardware. Do not add write operations before protocol
  evidence and safety approval. Exit: verified behavior is covered by
  deterministic offline tests.
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
