# PinkyOS
A UNIX-style kernel and OS based on the xv6 research kernel.
Exclusively supports armv8.x-a with a focus on "hard" real-time requirements.

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

