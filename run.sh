#!/bin/bash

./qemu-9.2.2/build/qemu-system-aarch64 \
    -L ./qemu-9.2.2/pc-bios/ \
    -machine virt,iommu=smmuv3,gic-version=3,secure=on,virtualization=on \
    -cpu cortex-a76 \
    -smp 4 \
    -m 512M \
    -kernel ./linux-6.13.7/arch/arm64/boot/Image \
    -append "rootfstype=ramfs nokaslr rdinit=/init console=ttyAMA0" \
    -initrd ./busybox-1.36.1/initramfs.cpio.gz \
    -nographic \
    $@
