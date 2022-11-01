#!/bin/bash
if [ "$1" == "mont" ]; then
    ./fat-fuse ../resources/bb_fs.img mnt
fi
if [ "$1" == "help" ]; then
    echo -e " Para montar imagen correr './montaje.sh mont'.\n Para desmontar: './montaje.sh um'. \n Para ayuda: './mont.sh help'."
fi
if [ "$1" == "um" ]; then
    umount mnt
fi
