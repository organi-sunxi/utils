#!/bin/sh

set -o nounset

./packimg -p 0x2000 ep1000a.dtb@44000000 ep1000m.bin@43000000 splash.bin@43100000 $1
