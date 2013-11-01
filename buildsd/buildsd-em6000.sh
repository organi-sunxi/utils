#!/bin/bash

######################################################################
# config

export TOPDIR=/home/yuq/projects/a10
export TARGET=/dev/sdf
export MOUNTDIR=$PWD/sddisk
export UBOOT_CROSS_COMPILE=arm-none-eabi-
export LINUX_CROSS_COMPILE=arm-linux-gnueabihf-
export LINUX_DEFAULT_CONFIG=em6000_fast_defconfig
export BOARD=em6000

export UBOOTDIR=$TOPDIR/sunxi-uboot
export LINUXDIR=$TOPDIR/linux-a10
export ROOTFSDIR=$TOPDIR/em6000-root
export TOOLSDIR=$TOPDIR/utils

export SPLASH_FILE=$TOPDIR/simit-320x240-24bit.bmp

export MAKE_OPT=-j8

bash buildsd.sh $@
