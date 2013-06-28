#!/bin/bash

TOPDIR=/home/yuq/projects/a10
#TARGET=/dev/sdf
#MOUNTDIR=/mnt/sddisk
MOUNTDIR=$TOPDIR/tmp
TARGET_SDCARD=false
UBOOT_CROSS_COMPILE=arm-none-eabi-
LINUX_CROSS_COMPILE=arm-linux-gnueabihf-

UBOOTDIR=$TOPDIR/sunxi-uboot
LINUXDIR=$TOPDIR/linux-a10
ROOTFSDIR=$TOPDIR/em6000-root
TOOLSDIR=$TOPDIR/utils

SPLASH_FILE=$TOPDIR/simit-320x240-24bit.bmp

declare -a BLOCK_SIZE
declare -a SPL_SIZE

CHIPS=(K9GBG08U0A K9F4G08U0A)
# K9GBG08U0A
BLOCK_SIZE[K9GBG08U0A]=1024K
PAGE_SIZE[K9GBG08U0A]=8192
# K9F4G08U0A
BLOCK_SIZE[K9F4G08U0A]=128K
PAGE_SIZE[K9F4G08U0A]=2048

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

# create for TARGET
if [ "$TARGET_SDCARD" = "true" ]
then
sudo mkdir -p $MOUNTDIR
cat <<EOF | sudo fdisk $TARGET
o
n




p
w
EOF
sudo mount "$(TARGET)1" $MOUNTDIR
else
mkdir -p $MOUNTDIR
fi

###################################################################
# tools
cd $TOOLSDIR

# mksplash
cd mksplash
make

# splash.bin
./mksplash $SPLASH_FILE
cp splash.bin $MOUNTDIR

# packimg
cd ../packimg
make

###################################################################
# build uboot
cd $UBOOTDIR

# MMC uboot
if [ "$TARGET_SDCARD" = "true" ]
then
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE distclean
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE Mele_A1000
sudo dd if=./spl/sunxi-spl.bin of=$TARGET bs=1024 seek=8
sudo dd if=./u-boot.bin of=$TARGET bs=1024 seek=32
fi

# NAND uboot
#make CROSS_COMPILE=$UBOOT_CROSS_COMPILE distclean
#make CROSS_COMPILE=$UBOOT_CROSS_COMPILE EM6000

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
#make ARCH=arm em6000_fast_defconfig
#make ARCH=arm CROSS_COMPILE=$LINUX_CROSS_COMPILE uImage LOADADDR=0x48000000
cp arch/arm/boot/uImage $MOUNTDIR

# script.bin
cd sunxi-board
./fex2bin ecohmi.fex script.bin
cp script.bin $MOUNTDIR
cd ..

# em6000.dtb & pack.img
for i in ${!CHIPS[*]}
do
	CHIP=${CHIPS[$i]}
	BSIZE=${BLOCK_SIZE[$CHIP]}
	PSIZE=${PAGE_SIZE[$CHIP]}
	FILENAME=em6000-$CHIP
	get_mtdparts $BSIZE $PSIZE
    sed "s/<mtdparts>/$MTDPARTS/" arch/arm/boot/dts/em6000-var.dts > arch/arm/boot/dts/$FILENAME.dts
	make ARCH=arm $FILENAME.dtb
	rm -rf arch/arm/boot/dts/$FILENAME.dts
	mkdir -p $MOUNTDIR/$CHIP
	cp arch/arm/boot/$FILENAME.dtb $MOUNTDIR/$CHIP/em6000.dtb
	$TOOLSDIR/packimg/packimg $PSIZE $MOUNTDIR/$CHIP/em6000.dtb@44000000 $MOUNTDIR/script.bin@43000000 $MOUNTDIR/splash.bin@43100000 $MOUNTDIR/$CHIP/pack.img
done

###################################################################
# rootfs


###################################################################
# splash

###################################################################
# tools



