# Legacy Upstream Sources

These RG/VG control, Modbus, and shared-header trees were moved from the
pinned Kinisi snapshot at `a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4` for
later refactoring into `onrobot_control`. Their original package manifests,
launch files, and source layout are retained for reference. The upstream MIT
notice is in this directory's `LICENSE`.

These files are not installed or run by the active package. They include
Humble-era Python code using the obsolete `pymodbus.client.sync` API and an RG
Compute Box power-cycle write path. Do not enable either on hardware before
reviewing it against the exact device firmware, vendor documentation, and
safety approval. The active C++ transport remains read-only.

The ROS-facing single and dual RG/VG command services and status listeners
have C++ replacements in `onrobot_control`. The dual-changer launch starts
only those ROS nodes; it does not connect to either gripper. The legacy
dual-changer restart-power service is intentionally not exposed.