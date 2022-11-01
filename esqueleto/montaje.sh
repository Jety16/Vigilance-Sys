#!/bin/bash
if [ "$1" == "mont" ]; then
    ./fat-fuse ../resources/bb_fs.img mnt
fi
if [ "$1" == "um" ]; then
    umount mnt
fi
if [ "$1" == "test" ]; then
    umount mnt
    make clean
    make
    ./fat-fuse ../resources/bb_fs.img mnt
fi
if [ "$1" == "help" ]; then
    echo  " Para montar imagen correr './montaje.sh mont'.
    Para desmontar: './montaje.sh um'.
    Para testing: './montaje.sh test' (desmonta, make clean, make y monta de nuevo). 
    Para ayuda: './mont.sh help'."
fi