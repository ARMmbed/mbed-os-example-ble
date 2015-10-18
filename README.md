# ble-examples
This repo contains a collection of standard BLE example applications based on
mbed OS, and built with [yotta](https://github.com/ARMmbed/yotta). Each demo (sub-folder) contains a separate yotta-module
meant for building an executable.

Please browse to sub-folders for specific documentation.

Getting Started
===============

Hardware
--------

In order to use BLE in mbed OS you need one of the following hardware combinations:

Either...

 * A device with a radio on board, such as a Nordic nRF51-based board

or

 * A supported target, such as the FRDM K64f, whith a shield or external BLE peripheral, such as an ST shield


The [`ble` yotta module](https://github.com/ARMmbed/BLE_API) is responsible for
providing the BLE APIs on mbed OS. The `ble` module uses yotta targets and yotta
target dependencies in order to provide the appropriate implementation of the BLE API
for your chosen hardware combination.

A yotta `target` is a supported combination of hardware platform and toolchain.

As such, for any of the hardware combinations above, you will need to use or create a
yotta target that describes your configuration. The existing supported configurations
are descibed below.


Targets for BLE
---------------

The following targets have been tested to work with these examples:

Nordic (using the nrf51822-ble module):

* nrf51dk-armcc
* nrf51dk-gcc
* mkit-gcc
* mkit-armcc

ST (using the st-ble module):

* frdm-k64f-st-ble-gcc (a FRDM-k64f with an ST BLE shield)

*When adding a new BLE-capable target, you should test it with these examples (most
of which are already included in the test-suite anyway) and then submit a pull request
on this repository to add the target to this list*

Building and testing the examples
---------------------------------

After cloning this repository, switch to any of the demo subdirectories, say
BLE_HeartRate, and execute the following:

```Shell
yotta target <an_appropriate_target_from_the_list_above>
yotta install
yotta build
```

The resulting binaries would be under `build/<yotta_target_name>/source/`.

Exactly which binaries are generated is dependent upon the target that you have
chosen. For Nordic Semiconductor targets, the following files will be present:

 * `<module_name>-combined.hex` is the one which can be flashed to the target
 * `<module_name>` is an ELF binary containing useful symbols
 * `<module_name>.hex` contains only the application (not the SoftDevice binary) and can be used for Firmware-over-the-Air.

For other targets, typically those that use external hardware like a shield, only the
`<module_name>` or `<module_name>.hex` files will be required. Either can be flashed
to the board, depending on which tools are being used to transfer the binary.

At this point, you should have the following

* A board with the appropriate hardware to use BLE
* A binary, running on this board, that makes use of the BLE APIs

In order to verify that the example are working properly, you should follow the
README.md of the specific example, which will explain in more detail what the example
does and how to test it.

Creating your own applications that use BLE in mbed OS
======================================================

Read on if you wish to create your own yotta based application or to port
mbed-classic applications to mbed OS.

Modules
-------

yotta is a tool that we're building at mbed, to make it easier to build better
software written in C, C++ or other C-family languages. yotta describes
programs in terms of dependencies upon software components (a.k.a. modules).
yotta also controls the build of your software in order to ensure
downloaded modules are available to use in your code.

Yotta modules come with a `module.json` to describe their metadata
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
    "ble": "ARMmbed/ble"
  },
  "targetDependencies": {},
  "bin": "./source"
}
```

From the above, the `name` field provides the name of the module: `ble-beacon`,
`dependencies` lists its dependencies upon other modules (referring
to Git repos on Github by default), and `description` provides a synopsis.

For most application modules, there is a `source` sub-folder containing C/C++
sources which get picked up automatically for builds--in the case of
BLE_BEacon, there's a `main.cpp` under its `source/`.

More can be learnt about yotta from [this
tutorial](http://docs.yottabuild.org/tutorial/tutorial.html).


Creating new BLE yotta application-modules
------------------------------------------

Please refer to yotta documentation on [creating an executable](http://docs.yottabuild.org/tutorial/tutorial.html#Creating%20an%20Executable).
BLE applications would typically depend on the `ble` module to use
[BLE API](https://github.com/ARMmbed/ble), as can be seen from https://github.com/ARMmbed/ble-examples/blob/master/BLE_Beacon/module.json#L13.
Applications would also need the mbed-drivers module to bring in mbed OS APIs, minar, and capabilities of the target platform.

Porting mbed-classic (mbed 2.0) BLE Applications
================================================

Prior to mbed OS, all application callbacks would execute in handler mode
(i.e. interrupt context). mbed OS comes with its own scheduler,
[minar](https://github.com/ARMmbed/minar), which encourages an asynchronous
programming style based upon thread-mode callbacks (i.e. non-interrupt user
context). With mbed OS, application code is made up entirely of callback
handlers. There isn't even a main(); it has been replaced with an
`app_start()`. Please refer to [minar
documentation](https://github.com/ARMmbed/minar#impact) to understand its
impact.

If you're porting a mebd-2 application for mbed OS, please do the following:

* Replace `main()` with `void app_start(int argc, char *argv[])`. app_start()
  will be given control after system initialization; but like any other
  callback handler it will be expected to finish quickly without blocking. If
  application initialization needs to issue blocking calls, app_start() can
  pend callbacks for later activity.

* Unlike the former main(), app_start() should *not* finish with an infinite
  wait loop for system events or for entering sleep. app_start() is expected
  to return quickly and yield to the scheduler for callback execution. The
  system will be put to low-power sleep automatically when there are no
  pending callbacks; and event handling should be done by posting callbacks.
  Please remove any equivalent of the following from your app_start():

  ```C++
  while (true) {
      ble.waitForEvent();
  }
  ```

* Any objects which are expected to persist across callbacks need to be
  allocated either from the global static context or from the free-store (i.e.
  using malloc() or new()). This was also true for mbed-2 except for objects
  allocated on the stack of main()--they would persist for the lifetime of the
  application because main() would never return. This is no longer true from
  app_start(); objects allocated locally within app_start() will be destroyed
  upon its return.

* Applications need to migrate to newer system APIs. For instance, with
  mbed-2, applications would use the Ticker to post time-deferred callbacks.
  This should now be achieved using minar's postCallback APIs directly. Refer
  to https://github.com/ARMmbed/minar#using-events. The replacement code would
  look something like:

  ```C++
  minar::Scheduler::postCallback(callback).delay(minar::milliseconds(DELAY));
  ```

  or if we are more explicit:

  ```C++
  Event e(FunctionPointer0<void>(callback).bind());
  minar::Scheduler::postCallback(e).delay(minar::milliseconds(DELAY));
  ```

  Using Minar to schedule callbacks would mean that callback handler would
  execute in thread mode (non-interrupt context), which would result in a more
  stable system.

It might be beneficial to study the documents around [Minar
scheduler](https://github.com/ARMmbed/minar#minar-scheduler).
