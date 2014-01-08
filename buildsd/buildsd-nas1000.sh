#!/bin/bash

######################################################################
# config

export TOPDIR=/home/yuq/projects/a10
export TARGET=/dev/sdf
export MOUNTDIR=$PWD/sddisk
export UBOOT_CROSS_COMPILE=arm-none-eabi-
export LINUX_CROSS_COMPILE=arm-linux-gnueabihf-
export LINUX_DEFAULT_CONFIG=sun7i_nas1000_defconfig
export BOARD=nas1000

export UBOOTDIR=$TOPDIR/sunxi-uboot
export LINUXDIR=$TOPDIR/linux-a10
export ROOTFSDIR=$TOPDIR/em6000-root
export TOOLSDIR=$TOPDIR/utils

export SPLASH_FILE="-b24 $TOOLSDIR/mksplash/THTF-128x128-24bit.bmp"

export MAKE_OPT=-j8

######################################################################
# mtd partition
# spl,uboot,packimg,kernel,initfs(rescue image include packimg2 and kernel),rootfs(for rescue)

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
