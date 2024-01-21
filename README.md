# Scratch

Build a debuggable os from scratch (AArch64).

```bash
$ git clone ...
$ git submodule init --update --recursive
```

## BusyBox

Enable `building static binary (no shared libs)`:

```bash
$ cat .config | grep STATIC
CONFIG_STATIC=y
# CONFIG_FEATURE_LIBBUSYBOX_STATIC is not set
```

Since `stime()` has been deprecated in glibc 2.31 and replaced with `clock_settime()`, we need to apply this patch to busybox:

```bash
$ git apply ../replace-stime-with-clock_settime.patch
```

```bash
$ export ARCH=arm64
$ export CROSS_COMPILE=aarch64-linux-gnu-
$ make -j`nproc`
$ make install -j`nproc`
```

```bash
$ cd _install
$ mkdir -p proc sys dev etc/init.d
```

```bash
$ cat etc/init.d/rcS
#!/bin/sh
echo "INIT SCRIPT"
mkdir /tmp
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev
mount -t debugfs none /sys/kernel/debug
mount -t tmpfs none /tmp
echo -e "Boot took $(cut -d' ' -f1 /proc/uptime) seconds"
setsid /bin/cttyhack setuidgid 0 /bin/sh
$ chmod +x etc/init.d/rcS
```

```bash
$ find . | cpio -o --format=newc > ../rootfs.img
```

## Linux

```bash
$ make defconfig ARCH=arm64
```

```bash
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bzImage -j`nproc`
```

## QEMU

```bash
$ mkdir build/ && cd build/
$ ../configure --target-list=x86_64-softmmu,x86_64-linux-user,arm-softmmu,arm-linux-user,aarch64-softmmu,aarch64-linux-user --enable-kvm
$ make -j`nproc`
```

## Run

```bash
$ ./qemu/build/qemu-system-aarch64 -m 512M -smp 4 -cpu cortex-a57 -machine virt -kernel ./linux/arch/arm64/boot/Image -initrd ./busybox/rootfs.img -append "rdinit=/linuxrc nokaslr console=ttyAMA0 loglevel=8" -nographic -s
```
