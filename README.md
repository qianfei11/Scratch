# Scratch

Build a debuggable os from scratch (AArch64).

## BusyBox (`busybox-1.36.1`)

Enable `building static binary (no shared libs)`:

```bash
$ cat .config | grep STATIC
CONFIG_STATIC=y
# CONFIG_FEATURE_LIBBUSYBOX_STATIC is not set
```

Compilation:

```bash
$ export ARCH=arm64
$ export CROSS_COMPILE=aarch64-linux-gnu-
$ make -j`nproc`
$ make install -j`nproc`
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
# Or directly `cp ./linux-config ./linux-6.13.7/.config`
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bzImage -j`nproc`
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
