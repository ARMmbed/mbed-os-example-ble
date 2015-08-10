# mbedOS-ble-demos
This repo contains a collection of standard BLE demos based on mbedOS and
yotta.

Each demo (subfolder) comes with a `module.json` to describe an executable
module in terms of its dependencies. More can be learnt about
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
Under that folder, the file called `<project>-combined.hex` is the one which
can be flashed to the target; the file called `<project>` is an ELF binary
containing useful symbols; whereas `<project>.hex` can be used for Firmware-
over-the-Air.
