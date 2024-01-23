#!/bin/bash
./qemu/build/qemu-system-aarch64 -L ./qemu/pc-bios/ -m 512M -smp 4 -cpu cortex-a57 -machine virt,iommu=smmuv3 -kernel ./linux/arch/arm64/boot/Image -initrd ./rootfs.img -append "rdinit=/linuxrc nokaslr console=ttyAMA0 loglevel=8" -nographic -s $@
