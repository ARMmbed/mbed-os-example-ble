---
name: Bug report
about: Create a report to help us improve
title: ''
labels: 'type: bug'
assignees: ''

---

### Description of defect

<!--
    Add detailed description of what you are reporting.
    Good example: https://os.mbed.com/docs/mbed-os/latest/contributing/workflow.html
-->

#### Target(s) and toolchain(s) (name and version) displaying this defect ?


#### What version of the example and mbed-os are you using (tag or sha) ?

<!--
    If you're using an old version, please try using the example branch `development`
    and mbed-os branch `master` to see if the issue is still present.
-->

#### How is this defect reproduced ?

#### Please attach a file containing the log with traces enabled.

<!--
    Enable tracing in the example by changing the tracing options in the mbed_app.json:
    ```
            "mbed-trace.enable": true,
            "mbed-trace.max-level": "TRACE_LEVEL_DEBUG",
            "cordio.trace-hci-packets": true,
            "cordio.trace-cordio-wsf-traces": true,
            "ble.trace-human-readable-enums": true
    ```
    and compile with `--profile debug`.

    Some trace options may be disabled if image cannot fit or the problem does not require them.
-->
