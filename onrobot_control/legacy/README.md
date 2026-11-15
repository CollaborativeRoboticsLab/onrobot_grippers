# Legacy Upstream Sources

These RG/VG control, Modbus, and shared-header trees were moved from the
pinned Kinisi snapshot at `a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4` for
later refactoring into `onrobot_control`. The complete upstream snapshot is
preserved under `onrobot_ros/upstream/kinisi-a83cd73`; this directory keeps the
remaining protocol references. The upstream MIT notice is in this directory's
`LICENSE`.

The legacy ROS-facing RG/VG command servers and status listeners that have
active C++ replacements were removed to avoid keeping duplicate node
implementations. The interactive CLI controllers, model-specific Python
protocol code, and Modbus transports remain here as historical reference; they
are not installed or run by the active package.

The remaining code includes Humble-era Python using the obsolete
`pymodbus.client.sync` API and RG Compute Box power-cycle paths. Do not enable
it on hardware before review against the exact firmware, vendor documentation,
and owner-approved safety requirements. The active C++ transport remains
read-only, and the active command servers only publish ROS messages.