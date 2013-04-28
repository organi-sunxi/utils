#!/bin/sh

blocks=128
src_page_size=8192
dst_page_size=$1
src_image="spl_$src_page_size.bin"
dst_image="spl_$1.bin"

echo "from $src_image to $dst_image"
dd if=/dev/zero of=$dst_image bs=$dst_page_size count=$blocks

for((i=0;i<$blocks;i++));
do
skip_pos=$(expr $src_page_size / $dst_page_size \* $i);
dd if=$src_image of=$dst_image seek=$i skip=$skip_pos bs=$dst_page_size count=1 conv=notrunc;
done
