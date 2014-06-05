#!/bin/bash
#  Utility/example to read environment settings from a Parallella flash memory
#  5/22/14 Fred Huettig
#
#  Option: -all  - list all var's from flash

set -e

FLASHNODE=/dev/mtd0
MKARGS="b 31 0"
ENVSTART=5111814
ENVSIZE=1024
SKUVAR="AdaptevaSKU"

if [ ! -b $FLASHNODE -a ! -c $FLASHNODE ] ; then
    if [ -e $FLASHNODE ] ; then
	echo "ERROR: Something's in the way of $FLASHNODE"
	exit 1
    fi

    sudo mknod $FLASHNODE $MKARGS
    echo "Created node $FLASHNODE"
fi

ENV=`tail -c +$ENVSTART $FLASHNODE | head -c $ENVSIZE | tr -s "\0" "\n"`
if [ "$ENV" == "" ] ; then
    echo "Can't read from flash, did you sudo?"
    exit 2;
fi

ENV+=$'\n'   # Make sure there is a \n at the end

# I'm sure there is a more efficient way to do this,
# but the following should be reasonably portable
if [ "$1" = "-all" ] ; then
    ETEMP=$ENV
    while [ "$ETEMP" ] ; do
	echo "> ${ETEMP%%$'\n'*}"
	ETEMP="${ETEMP#*$'\n'}"
    done
fi

# Find our SKU, first check if it's there at all:
if [[ "$ENV" =~ $SKUVAR= ]] ; then
    AdaptevaSKU="${ENV#*$SKUVAR=}"
    AdaptevaSKU="${AdaptevaSKU%%$'\n'*}"
    echo "Our SKU is: $AdaptevaSKU"
else
    echo "It's very likely our SKU is SKUA101040, but not certain"
fi
