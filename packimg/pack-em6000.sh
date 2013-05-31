#!/bin/sh

set -o nounset

./packimg -p 0x2000 em6000.dtb@44000000 em6000.bin@43000000 splash.bin@43100000 $1
