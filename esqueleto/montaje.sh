#!/bin/bash

current_path=$(pwd)

if [ "$1" == "mont" ]; then
	./fat-fuse ../resources/bb_fs.img mnt
fi
if [ "$1" == "um" ]; then
	umount mnt
fi
if [ "$1" == "test" ]; then
    if [ "$2" == "-d" ]; then
        	umount mnt
        	make clean
        	make
        	./fat-fuse $2 ../resources/bb_fs.img mnt
    elif [ "$2" == "-f" ]; then
        	umount mnt
        	make clean
        	make
        	gdb ./fat-fuse $2 ../resources/bb_fs.img mnt
    elif [ "$2" == "create" ]; then
        	umount mnt
        	make clean
        	make
        	./fat-fuse ../resources/bb_fs.img mnt
        	cd "$current_path/mnt" && ../montaje_bis.sh
    else
        	umount mnt
        	make clean
        	make
		./fat-fuse ../resources/bb_fs.img mnt
    fi
fi
if [[ "$1" == "restore" ]]; then
		git restore ../resources/bb_fs.img
		sudo umount mnt
		rm -r mnt
		mkdir mnt
fi
if [ "$1" == "help" ]; then
	echo  " Para montar imagen correr './montaje.sh mont'.
	Para desmontar: './montaje.sh um'.
	Para testing: './montaje.sh test' (desmonta, make clean, make y monta de nuevo). 
	Para testing y entrar al archivo: './montaje.sh test see'.
	Para restaurar la carpeta: './montaje.sh restore'
	Para restore de git: './montaje.sh restore imagen'
	Para ayuda: './mont.sh help'."
fi
