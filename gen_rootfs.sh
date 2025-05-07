#!/bin/bash

set -x

ROOTFS_PATH="$PWD/busybox-1.36.1/_install"

pushd $ROOTFS_PATH
find . -print0 | cpio --null -o --format=newc | gzip > ../initramfs.cpio.gz
popd
