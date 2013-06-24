#!/bin/bash

# usage: yuv2rgb.sh width height directory filenumber

for i in $(seq 0 1 $4)
do
	`dirname $0`/yuv2rgb $1 $2 $3/$i
done
