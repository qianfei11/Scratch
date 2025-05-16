# Scratch

Build a debuggable os from scratch (AArch64).

## BusyBox (`busybox-1.36.1`)

Enable `building static binary (no shared libs)` and disable `tc`:

```bash
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
# Or directly `cp ./busybox.config ./busybox-1.36.1/.config`
$ cat .config | grep CONFIG_STATIC
CONFIG_STATIC=y
$ cat .config | grep CONFIG_TC
# CONFIG_TC is not set
```

Compilation:

```bash
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j`nproc`
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- install -j`nproc`
```

Configuration:

```bash
$ cd _install
$ mkdir -pv {etc,proc,sys,usr/{bin,sbin}}
```

Edit `init` script:

```bash
#!/bin/sh

mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo -e "\nBoot took $(cut -d' ' -f1 /proc/uptime) seconds\n"

exec /bin/sh
```

Pack the filesystem:

```bash
$ find . -print0 | cpio --null -ov --format=newc | gzip > ../initramfs.cpio.gz
```

## Linux (`linux-6.13.7`)

Enable the tiny configuration and compilation:

```bash
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- tinyconfig
# Or directly `cp ./linux.config ./linux-6.13.7/.config`
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image qemu/qemu-mali.dtb -j`nproc`
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- scripts_gdb -j`nproc`
```

## QEMU (`qemu-9.2.2`)

Compilation:

```bash
$ mkdir build/ && cd build/
$ ../configure --target-list=aarch64-softmmu --enable-debug --disable-docs
$ make -j`nproc`
```

## Run

Execute and debug the os:

```bash
# ./gen_rootfs.sh
$ ./run.sh
$ ./run.sh -trace "smmuv3_*"
```
