# Scipp Tools

This directory contains a number of helper scripts for the scipp development process.

1. `run_asan.sh` builds all tests and runs them with Address Sanitizer enabled.
1. `run_ubsan.sh` builds all tests and runs them with Undefined-Behavior Sanitizer enabled.
1. `anaconda_remove_old_dev_packages.sh` remove packages from Anacond Cloud that have the `dev` label and are older than 1 month (intended mainly to be run by CI).

`run_sanitizer.sh` is a helper script and not intended for direct use.
