#!/bin/bash

######################################################################
# parse parameter

BOARD_CAP=`echo $BOARD | tr 'a-z' 'A-Z'`

TARGET_SDCARD=false
TARGET_UBOOT=false
TARGET_PACKIMG=false
TARGET_KERNEL=false
TARGET_ROOTFS=false

while getopts ht: opt
do
	case $opt in
		t)
			case $OPTARG in
				sdcard)
					TARGET_SDCARD=true
					;;
				uboot)
					TARGET_UBOOT=true
					;;
				packimg)
					TARGET_PACKIMG=true
					;;
				kernel)
					TARGET_KERNEL=true
					;;
				rootfs)
					TARGET_ROOTFS=true
					;;
				all)
					TARGET_UBOOT=true
					TARGET_PACKIMG=true
					TARGET_KERNEL=true
					TARGET_ROOTFS=true
					;;
				*)
					echo "Unknown target $OPTARG"
					;;
			esac
			;;
		h)
			echo "Usage: $0 [-h] [-t sdcard|uboot|packimg|kernel|rootfs|all]"
			;;
		*)
			echo "Unknown option $opt"
			exit 1
			;;
	esac
done

######################################################################
# chip parameter

declare -A BLOCK_SIZE
declare -A PAGE_SIZE

CHIPS=(K9GBG08U0A K9F4G08U0A)
# K9GBG08U0A
BLOCK_SIZE[K9GBG08U0A]=1024K
PAGE_SIZE[K9GBG08U0A]=8192
# K9F4G08U0A
BLOCK_SIZE[K9F4G08U0A]=128K
PAGE_SIZE[K9F4G08U0A]=2048

###################################################################
# SD card partition

# create for TARGET
if [ "$TARGET_SDCARD" = "true" ]
then
mkdir -p $MOUNTDIR
cat <<EOF | sudo fdisk $TARGET
o
n




p
w
EOF
sudo mkfs.vfat -I ${TARGET}1
echo "mount sdcard ${TARGET}1 on $MOUNTDIR"
sudo mount "${TARGET}1" $MOUNTDIR
else
mkdir -p $MOUNTDIR
fi

###################################################################
# tools

cd $TOOLSDIR

# tools for packimg
if [ "$TARGET_PACKIMG" = "true" ]
then
# mksplash
cd mksplash
make

# splash.bin
./mksplash -b32 $SPLASH_FILE
cp splash.bin $MOUNTDIR

# packimg
cd ../packimg
make
fi

###################################################################
# build uboot

cd $UBOOTDIR

# MMC uboot
if [ "$TARGET_SDCARD" = "true" ]
then
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE distclean
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE $(BOARD_CAP)_MMC $MAKE_OPT
make u-boot.img
sudo dd if=./spl/sunxi-spl.bin of=$TARGET bs=1024 seek=8
sudo dd if=./u-boot.img of=$TARGET bs=1024 seek=32
fi

# NAND uboot
if [ "$TARGET_UBOOT" = "true" ]
then
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE distclean
make CROSS_COMPILE=$UBOOT_CROSS_COMPILE $BOARD_CAP $MAKE_OPT

# sunxi-spl.bin & u-boot.bin
cp spl/sunxi-spl.bin $MOUNTDIR
cp u-boot.bin $MOUNTDIR

# em6000.env
cd bootscript
make
cp $BOARD.env $MOUNTDIR
fi

###################################################################
# build linux

cd $LINUXDIR

# uImage
if [ "$TARGET_KERNEL" = "true" ]
then
make ARCH=arm $LINUX_DEFAULT_CONFIG
make ARCH=arm CROSS_COMPILE=$LINUX_CROSS_COMPILE uImage LOADADDR=0x48000000 $MAKE_OPT
cp arch/arm/boot/uImage $MOUNTDIR
fi

# pack all
if [ "$TARGET_PACKIMG" = "true" ]
then

# script.bin
cd sunxi-board
./fex2bin $BOARD.fex script.bin
cp script.bin $MOUNTDIR
cd ..

# em6000.dtb & pack.img
for i in ${!CHIPS[*]}
do
	CHIP=${CHIPS[$i]}
	BSIZE=${BLOCK_SIZE[$CHIP]}
	PSIZE=${PAGE_SIZE[$CHIP]}
	CHIPSTR=b${BSIZE%[k|K]}p$((PSIZE/1024))
	FILENAME=$BOARD-$CHIPSTR
	get_mtdparts $BSIZE $PSIZE
	sed "s/<mtdparts>/$MTDPARTS/" arch/arm/boot/dts/$BOARD-var.dts > arch/arm/boot/dts/$FILENAME.dts
	make ARCH=arm $FILENAME.dtb
	rm -rf arch/arm/boot/dts/$FILENAME.dts
	mkdir -p $MOUNTDIR/$CHIPSTR
	cp arch/arm/boot/$FILENAME.dtb $MOUNTDIR/$CHIPSTR/$BOARD.dtb
	$TOOLSDIR/packimg/packimg -p $PSIZE $MOUNTDIR/$CHIPSTR/$BOARD.dtb@44000000 $MOUNTDIR/script.bin@43000000 $MOUNTDIR/splash.bin@43100000 $MOUNTDIR/$CHIPSTR/pack.img
done

fi

###################################################################
# rootfs

cd $ROOTFSDIR

if [ "$TARGET_ROOTFS" = "true" ]
then

for i in ${!CHIPS[*]}
do
	CHIP=${CHIPS[$i]}
	BSIZE=${BLOCK_SIZE[$CHIP]}
	PSIZE=${PAGE_SIZE[$CHIP]}
	CHIPSTR=b${BSIZE%[k|K]}p$((PSIZE/1024))
	make clean
	make TARGET_CHIP=$CHIP rootfs.img
	cp rootfs.img $MOUNTDIR/$CHIPSTR
done

fi



