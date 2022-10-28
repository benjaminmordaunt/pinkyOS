# PinkyOS
A hobby kernel and OS based (loosely) on the xv6 research kernel.
Exclusively supports armv8.x-a with a focus on real-time requirements and novel Arm architectural features. For example, there will be first-class support for transactional memory operations and, later, Confidential Compute Architecture (CCA, incl. "Realms").

## Running under QEMU
1. `make` the kernel and userspace applications.
2. Ensure `qemu-system-arm` is installed on your host machine.
3. Execute `./run.sh`.

## Debugging under QEMU
1. `make` the kernel and userspace applications.
2. Ensure `qemu-system-arm` and `lldb` are installed on your host machine.
3. Execute `./run-debug.sh`.
4. Separately, run `lldb kernel.elf`.
5. Connect to QEMU's gdb server using `gdb-remote 1234`.

## Credits
This is a fork of `sudharson14/xv6-OS-for-arm-v8`. See `LICENSE` for Copyright information.
Copyright (c) 2022+ Benjamin Mordaunt

# Architecture Notes

## Managing technical debt

As can be seen when looking at the Linux kernel project, the requirement to *not break userspace* (and others) has led to a slew of incremental hacks on fundamental systems such as memory management. In turn, this greatly increases the maintenance burden for contributors, makes navigating the codebase more problematic and deters new contributors who _just can't understand_ the design decisions - and for good reason.

In contrast, PinkyOS uses "_finite backwards-compatibility_" to enable applications built for older versions of the kernel to function under a compatiblity layer, up to 3 API-breaking versions hence. For applications even older than this, they can continue to run under _multiple_ (stacked) compatibility layers. The older an application is, the more it will be "punished" in terms of performance and capabilities, but at least they will run.

In this sense, pinkyOS places feature-richness and high performance well above legacy compatibility - a feature that will likely dissuade most industry users, but which likely shouldn't.
