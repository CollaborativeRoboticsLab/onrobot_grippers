# Secondary Upstream Comparison

Compared the following shallow checkouts on 2026-11-13. The temporary source
copies have been removed after comparison; revisions and license findings are
recorded here. The pinned Kinisi snapshot remains the staged source baseline.

| Repository | Revision | License observed | Comparison and disposition |
| --- | --- | --- | --- |
| [blooop/onrobot_grippers](https://github.com/blooop/onrobot_grippers) | `7882293b19c2f04b3cfd40ebb01294ba55e7c344` | MIT | Package trees match the pinned Kinisi source; only an existing `COLCON_IGNORE` marker differs. Duplicate checkout removed. |
| [BrettRD/onrobot_grippers](https://github.com/BrettRD/onrobot_grippers) | `d04668d502c80456f5d006ab3266586940096d8c` | MIT | Contains ROS 1 `catkin` material and alternate controller files. VG message fields and `SetCommand` overlap Kinisi's ROS 2 interface; field-name differences are not a new contract. Duplicate checkout removed. |
| [UOsaka-Harada-Laboratory/onrobot](https://github.com/UOsaka-Harada-Laboratory/onrobot) | `46fae3557dd3aff678adf6b3320ec1338493d84a` | MIT | ROS 1 implementation includes dual-changer nodes and an alternate RG6 Xacro wrapper. Kinisi already has dual-changer nodes; the Xacro uses the same model assets. Duplicate checkout removed. |
| [cwsfa/onrobot-api](https://github.com/cwsfa/onrobot-api) | `da1eb38d47e13df123fa8a308a28ab09efebaf1e` | No license file found at the checked revision | Documents RG2, 2FG7, and VGC10 API examples and includes byte-decoding scripts. The 2FG7 material is a unique future research lead, but no code or decoded content is retained without redistribution permission. Checkout removed. |

## Merge Result

No secondary source files were merged into the staged Kinisi snapshot: the
licensed code differences are overlapping ROS 1/Humble alternatives, while the
only clearly additional model API is from a repository with no license file.
The local clones were removed to avoid retaining duplicate or unlicensed source.
This avoids treating community implementations as protocol authority. The
upstream commit and license are recorded for any later reviewed adoption.
