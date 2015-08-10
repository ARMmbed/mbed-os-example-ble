# mbedOS-ble-demos
This repo contains a collection of standard BLE demos based on mbedOS and
yotta.

Each demo (subfolder) contains a separate yotta-module meant for building an
executable. Yotta modules come with a `module.json` to describe its metadata
and dependencies.

As an example, the BLE_Beacon demo comes with the following module.json:
```
{
  "name": "ble-beacon",
  "version": "0.0.1",
  "description": "BLE iBeacon example, building with yotta",
  "licenses": [
    {
      "url": "https://spdx.org/licenses/Apache-2.0",
      "type": "Apache-2.0"
    }
  ],
  "dependencies": {
    "mbed-drivers": "*",
    "ble": "ARMmbed/ble"
  },
  "targetDependencies": {},
  "bin": "./source"
}
```

From the above, the `name` field provides the name of the module: `ble-beacon`,
and `dependencies` lists its dependencies upon other modules
(referring to Git repos). More can be learnt about
[yotta](https://github.com/ARMmbed/yotta) from [this
tutorial](http://docs.yottabuild.org/tutorial/tutorial.html).

Build Instructions
==================

After cloning this repository, switch to any of the demo subdirectories, say
BLE_HeartRate, and execute the following:

```Shell
yotta target <an_appropriate_target_such_as_mkit-gcc>
yotta install
yotta build
```

The resulting binaries would be under `build/<yotta_target_name>/source/`.
Under that folder, the file called `<module_name>-combined.hex` is the one which
can be flashed to the target; the file called `<module_name>` is an ELF binary
containing useful symbols; whereas `<module_name>.hex` can be used for Firmware-
over-the-Air.
