#!/bin/sh

flash_eraseall /dev/mtd0
nandwrite -p /dev/mtd0 spl.bin

flash_eraseall /dev/mtd1
nandwrite -p /dev/mtd1 u-boot.bin

flash_eraseall /dev/mtd2
nandwrite -p /dev/mtd2 em6000.env

packimg_burn -d /dev/mtd3 -c 5 -f pack.img

flash_eraseall /dev/mtd4
nandwrite -p /dev/mtd4 uImage

flash_eraseall /dev/mtd5
nandwrite -p /dev/mtd5 initfs.img

ubiformat /dev/mtd6
ubiattach /dev/ubi_ctrl -m 6
ubimkvol /dev/ubi0 -N rootfs -s 128MiB
ubiupdatevol /dev/ubi0_0 rootfs.img

