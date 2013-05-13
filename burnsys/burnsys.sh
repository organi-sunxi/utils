#!/bin/sh

flash_eraseall /dev/mtd0
nandwrite -p /dev/mtd0 spl.bin

flash_eraseall /dev/mtd1
nandwrite -p /dev/mtd1 u-boot.bin

flash_eraseall /dev/mtd2
nandwrite -p /dev/mtd2 em6000.env

flash_eraseall /dev/mtd3
nandwrite -p /dev/mtd3 em6000.dtb
#nandwrite -p /dev/mtd3 em6000.atags

flash_eraseall /dev/mtd4
nandwrite -p /dev/mtd4 usplash.bin

flash_eraseall /dev/mtd5
nandwrite -p /dev/mtd5 uscript.bin

flash_eraseall /dev/mtd6
nandwrite -p /dev/mtd6 uImage

flash_eraseall /dev/mtd7
nandwrite -p /dev/mtd7 initfs.img

ubiformat /dev/mtd8
ubiattach /dev/ubi_ctrl -m 8
ubimkvol /dev/ubi0 -N rootfs -s 128MiB
ubiupdatevol /dev/ubi0_0 rootfs.img

