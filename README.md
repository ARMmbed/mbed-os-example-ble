# ble-examples
This repo contains a collection of standard BLE example applications based on
mbed OS and [yotta](https://github.com/ARMmbed/yotta). Each demo (sub-folder) contains a separate yotta-module
meant for building an executable.

Modules
=======

yotta is a tool that we're building at mbed, to make it easier to build better
software written in C, C++ or other C-family languages. yotta describes
programs in terms of dependencies upon software components (a.k.a. modules).
yotta also controls the build of your software in order to ensure
downloaded modules are available to use in your code.

Yotta modules come with a `module.json` to describe its metadata
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
`dependencies` lists its dependencies upon other modules (referring
to Git repos in Github by default), and `description` provides a synopsis.

For most application modules, there is a `source` sub-folder containing C/C++
sources which get picked up automatically for builds--in the case of
BLE_BEacon, there's a `main.cpp` under source.

More can be learnt about yotta from [this
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

Creating new BLE yotta Application-modules
==========================================

Please refer to yotta documentation on [creating an executable](http://docs.yottabuild.org/tutorial/tutorial.html#Creating%20an%20Executable).
BLE applications would typically depend on the `ble` module to use
[BLE API](https://github.com/ARMmbed/ble), as can be seen from https://github.com/ARMmbed/ble-examples/blob/master/BLE_Beacon/module.json#L13.
Applications would also need the mbed-drivers module to bring in mbed OS APIs, minar, and capabilities of the target platform.

Suggested BLE targets
=====================

Currently, we support an initial port of mbed OS to the following targets:

* nrf51dk-armcc
* nrf51dk-gcc
* bbcmicro-armcc
* mkit-gcc
* mkit-armcc

Porting mbed-2 BLE Applications
===============================

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
