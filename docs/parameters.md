# ROS 2 Parameters

This document describes the parameter surface available in the current Jazzy
workspace. The RG2 launch/config files select a model label only. The node does
not connect to a Compute Box, poll registers, or send commands.

## Current parameter

| Parameter | Type | Default | Current effect |
| --- | --- | --- | --- |
| `model` | string | `rg2` | Logged as the selected model label. It is not validated against a capability matrix and does not select a protocol implementation. |

The checked-in configuration is [`onrobot_ros/config/rg2.yaml`](../onrobot_ros/config/rg2.yaml).
It uses the ROS 2 parameter-file format:

```yaml
/**:
	ros__parameters:
		model: "rg2"
```

The wildcard scope also works when the launch file starts the node in a
namespace. The `model` launch argument overrides the value in the YAML file;
the default is `rg2`.

## Launch

After building and sourcing the workspace overlay:

```bash
ros2 launch onrobot_control rg2.launch.py
```

Set a namespace or a different model label with launch arguments:

```bash
ros2 launch onrobot_control rg2.launch.py namespace:=cell_a model:=rg2
```

The node name is `onrobot_control`; in the example above its fully qualified
name is `/cell_a/onrobot_control`. Inspect the current value with:

```bash
ros2 param get /cell_a/onrobot_control model
```

## Not Yet Configurable

The node does not currently declare endpoint, unit ID, polling, response-timeout,
register-map, or calibration parameters. Do not add these to the YAML file yet;
they would imply behavior the node does not implement. In particular, register
addresses, scaling, limits, and status interpretation must come from the exact
RG2/Compute Box firmware documentation and must not be deployment-time guesses.

When endpoint parameters are implemented, document their types, units, valid
ranges, defaults, and failure behavior here and add validation before opening a
connection. Keep device-specific protocol semantics in a versioned RG2/firmware
implementation, not in an unvalidated ROS parameter override.
