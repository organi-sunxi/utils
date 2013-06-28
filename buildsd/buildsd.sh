#!/bin/bash

TOPDIR=/home/yuq/projects/a10
TARGET=/dev/sdf
MOUNTDIR=/mnt/sddisk
UBOOT_CROSS_COMPILE=arm-none-eabi-
LINUX_CROSS_COMPILE=arm-linux-gnueabihf-

UBOOTDIR=$TOPDIR/sunxi-uboot
LINUXDIR=$TOPDIR/linux-a10
ROOTFSDIR=$TOPDIR/em6000-root

# create for TARGET
sudo mkdir -p $MOUNTDIR
cat <<EOF | sudo fdisk $TARGET
o
n




p
w
EOF
sudo mount "$(TARGET)1" $MOUNTDIR

###################################################################
# build uboot
cd $UBOOTDIR

# MMC uboot
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE Mele_A1000
sudo dd if=./spl/sunxi-spl.bin of=$TARGET bs=1024 seek=8
sudo dd if=./u-boot.bin of=$TARGET bs=1024 seek=32

# NAND uboot
make distclean
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE EM6000

# sunxi-spl.bin & u-boot.bin
cp spl/sunxi-spl.bin $MOUNTDIR
cp u-boot.bin $MOUNTDIR

# em6000.env
cd bootscript
make
cp em6000.env $MOUNTDIR

###################################################################
# build linux
cd $LINUXDIR

# uImage
make ARCH=arm em6000_fast_defconfig
make ARCH=arm CROSS_COMPILE=$LINUX_CROSS_COMPILE uImage LOADADDR=0x48000000
cp arch/arm/boot/uImage $MOUNTDIR

# script.bin
cd sunxi-board
./fex2bin ecohmi.fex script.bin
cp script.bin $MOUNTDIR

###################################################################
# rootfs

###################################################################
# splash

###################################################################
# tools



