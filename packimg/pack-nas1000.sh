#!/bin/bash

set -o nounset

$(dirname $0)/packimg -p 0x2000 nas1000.dtb@44000000 nas1000.bin@43000000 $1
