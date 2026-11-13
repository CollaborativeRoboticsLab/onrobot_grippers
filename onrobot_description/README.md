# OnRobot descriptions

This directory contains separate ROS 2 description packages:

* `onrobot_rg_description`: RG2 and RG6.
* `onrobot_vg_description`: VG10 and VGC10 (one-cup and four-cup variants).

Both packages were transferred from
`kinisi-robotics/onrobot_grippers` at commit
`a83cd730e33828abf4e2c22fb0fe3c9c522a8ec4`. The pinned repository and both
package manifests identify the content as MIT; each package carries the
upstream license notice. No separate licenses or source notices were found in
the description trees. The history identifies the package reorganization
commit, but does not independently establish the original CAD/mesh authorship.

The package build installs meshes, Xacro files, RViz configs, and preview
images. Legacy ROS 1 launch files are retained in source but not installed.
Gazebo and transmission extensions remain upstream content and are not
validated for ROS 2 Jazzy. Model fidelity and collision suitability have not
been independently verified; no hardware or safety claim is made.