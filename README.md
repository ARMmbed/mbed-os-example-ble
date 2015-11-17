# BLE Examples
This repo contains a collection of BLE example applications based on
mbed OS and built with [yotta](https://github.com/ARMmbed/yotta). Each example subdirectory contains a separate yotta module meant for building an executable.

Please browse to subdirectories for specific documentation.

Getting Started
===============

Hardware
--------

In order to use BLE in mbed OS you need one of the following hardware combinations:



 * A device with a radio on board, such as a Nordic nRF51-based board.

 * A supported target, such as the FRDM K64F, with a shield or external BLE peripheral, such as an ST shield.

The [`ble` yotta module](https://github.com/ARMmbed/ble) provides the BLE APIs on mbed OS. The `ble` module uses yotta targets and yotta
target dependencies to provide the appropriate implementation of the BLE API
for your chosen hardware combination.

A yotta `target` is a supported combination of hardware board and toolchain. This means that, for any of the hardware combinations above, you will need to use or create a
yotta target that describes your configuration. The existing supported configurations
are described below.

Targets for BLE
---------------

The following targets have been tested and work with these examples:

Nordic (using the nrf51822-ble module):

* nrf51dk-armcc
* nrf51dk-gcc
* mkit-gcc
* mkit-armcc

ST (using the st-ble module):

* frdm-k64f-st-ble-gcc (an FRDM-k64f with an ST BLE shield)

Building and testing the examples
---------------------------------

__To build an example:__

1. Clone the repository containing the collection of examples:

	```
	$ git clone https://github.com/ARMmbed/ble-examples.git
	```


	**Tip:** If you don't have GitHub installed, you can [download a zip file](https://github.com/ARMmbed/ble-examples/archive/master.zip) of the repository.

1. Using a command-line tool, navigate to any of the example directories, like BLE_Beacon:

	```
	$ cd ble-examples
	$ cd BLE_Beacon
	```

1. Set a yotta target. For example, if you have and Nordic nRF51 and the GCC toolchain:

	```
	yotta target nrf51dk-gcc
	```



1. Run the build:

	```yotta build```

__To run the application on your board:__

1. Connect your mbed board to your computer over USB. It appears as removable storage.

1. When you run the ``yotta build`` command, as you did above, yotta creates a BIN or a combined HEX file in a ```build/<target-name>/source``` directory under the example's directory. Drag and drop the file to the removable storage.


Exactly which executables are generated depends on the target that you have
chosen. For Nordic Semiconductor targets, the following .hex files will be present:

 * `<module_name>-combined.hex` is the one which can be flashed to the target.
 * `<module_name>` is an ELF binary containing symbols (useful for debugging).
 * `<module_name>.hex` contains only the application (not the SoftDevice binary) and can be used for Firmware Over the Air.


Creating your own applications using BLE in mbed OS
======================================================

Read on if you wish to create your own yotta-based application or to port
mbed Classic applications to mbed OS.

Modules
-------

yotta is a tool that we're building at mbed, to make it easier to build better
software written in C, C++ or other C-family languages. yotta describes
programs in terms of dependencies on modules: libraries or other programs.
yotta also controls the build of your software and ensures
that modules are available to use in your code. It downloads these modules from a public online registry, or other sources such as GitHub.

Yotta modules come with a `module.json` file that describes their metadata
and dependencies.

As an example, the BLE_Beacon demo comes with the following `module.json`:
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
    "ble": "^1.0.0"
  },
  "targetDependencies": {},
  "bin": "./source"
}
```

The `name` field provides the name of the module: `ble-beacon`,
`dependencies` lists its dependencies upon other modules (referring
to repositories either on GitHub or on the yotta public registry), and `description` provides a synopsis.

For most application modules, there is a `source` subfolder containing C or C++
sources that get picked up automatically for builds. In the case of
BLE_BEacon, there's a `main.cpp` under its `source/`.

You can learn  about yotta from [this
tutorial](http://docs.yottabuild.org/tutorial/tutorial.html).


Creating new BLE yotta applications
------------------------------------------

Please refer to our mbed OS documentation on [creating an executable](https://docs.mbed.com/docs/getting-started-mbed-os/en/latest/Full_Guide/app_on_yotta/).


BLE applications typically depend on the `ble` module to use
[BLE API](https://github.com/ARMmbed/ble). You can see an example in [the #include directive in the BLE Beacon example](https://github.com/ARMmbed/ble-examples/blob/master/BLE_Beacon/module.json#L13).

Applications also need the `mbed-drivers` module to bring in mbed OS APIs, MINAR, and capabilities of the target board. When using ‘ble’ in an mbed OS application, the ‘mbed-drivers’ module is pulled in as implicit dependency by ‘ble’.

Porting mbed Classic (mbed 2.0) BLE Applications
================================================

Prior to mbed OS, all application callbacks would execute in handler mode
(interrupt context). mbed OS comes with its own scheduler,
[MINAR](https://github.com/ARMmbed/minar), which encourages an asynchronous
programming style based on thread-mode callbacks (non-interrupt user
context). With mbed OS, application code is made up entirely of callback
handlers. There isn't even a main(); it has been replaced with an
`app_start()`. Please refer to [MINAR
documentation](https://github.com/ARMmbed/minar#impact) to understand its
impact.

If you're porting an mebd Classic application to mbed OS, please do the following:

* Replace `main()` with `void app_start(int argc, char *argv[])`. app_start()
  will receive control after system initialization, but like any other
  callback handler it will be expected to finish quickly without blocking. If
  application initialization needs to issue blocking calls, app_start() can
  pend callbacks for later activity.

* Unlike the former main(), app_start() should *not* finish with an infinite
  wait loop for system events or for entering sleep. app_start() is expected
  to return quickly and yield to the scheduler for callback execution. The
  system will be put to low-power sleep automatically when there are no
  pending callbacks, and event handling should be done by posting callbacks.
  Please remove any equivalent of the following from your app_start():

  ```C++
  while (true) {
      ble.waitForEvent();
  }
  ```

* Any objects which are expected to persist across callbacks need to be
  allocated either from the global static context or from the free-store (that is,
  using malloc() or new()). This was also true for mbed Classic except for objects
  allocated on the stack of main() - they would persist for the lifetime of the
  application because main() would never return. This is no longer true for
  app_start(); objects allocated locally within app_start() will be destroyed
  upon its return.

* Applications need to migrate to newer system APIs. For instance, with
  mbed Classic, applications would use the Ticker to post time-deferred callbacks.
  This should now be achieved using MINAR's postCallback APIs directly. Refer
  to [https://github.com/ARMmbed/minar#using-events](https://github.com/ARMmbed/minar#using-events). The replacement code would
  look something like:

  ```C++
  minar::Scheduler::postCallback(callback).delay(minar::milliseconds(DELAY));
  ```

  Or if we are more explicit:

  ```C++
  Event e(FunctionPointer0<void>(callback).bind());
  minar::Scheduler::postCallback(e).delay(minar::milliseconds(DELAY));
  ```

  Using MINAR to schedule callbacks means that the callback handler will
  execute in thread mode (non-interrupt context), which would result in a more
  stable system.

Again, you might find it useful to study the documentation covering [MINAR](https://github.com/ARMmbed/minar#minar-scheduler).
