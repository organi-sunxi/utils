#!/bin/bash

######################################################################
# config

export TOPDIR=/data/samba/armlinux/A10/linux-sunxi
export TARGET=/dev/sdf
export MOUNTDIR=$PWD/sddisk
export UBOOT_CROSS_COMPILE=arm-none-linux-gnueabi-
export LINUX_CROSS_COMPILE=arm-linux-gnueabihf-
export LINUX_DEFAULT_CONFIG=em6000_fast_defconfig
export BOARD=ep1000

export UBOOTDIR=$TOPDIR/uboot
export LINUXDIR=$TOPDIR/linux-sunxi
export ROOTFSDIR=$TOPDIR/root/root-git
export TOOLSDIR=$TOPDIR/utils

SPLASH_FILE=$TOPDIR/simit-320x240-24bit.bmp

MAKE_OPT=-j8

######################################################################
# mtd partition

function get_mtdparts {
case "$1" in
128K)
	let "OFFSET=$2*128+0x200000+0x080000"
	MTDPARTS="4M@$OFFSET(packimg),5M(kernel),24M(initfs),-(rootfs)"
	;;
256K)
	let "OFFSET=$2*128+0x200000+0x100000"
	MTDPARTS="4M@$OFFSET(packimg),5M(kernel),24M(initfs),-(rootfs)"
	;;
512K)
	let "OFFSET=$2*128+0x280000+0x180000"
	MTDPARTS="5M@$OFFSET(packimg),6M(kernel),32M(initfs),-(rootfs)"
	;;
1024K)
	let "OFFSET=$2*128+0x400000+0x300000"
	MTDPARTS="8M@$OFFSET(packimg),8M(kernel),64M(initfs),-(rootfs)"
	;;
2048K)
	let "OFFSET=$2*128+0x800000+0x600000"
	MTDPARTS="12M@$OFFSET(packimg),12M(kernel),64M(initfs),-(rootfs)"
	;;
*)
	echo "unsupported block size"
	;;
esac
}

source buildsd.sh $@
