#!/bin/bash
# DPO/MSO2000 ROM image creator
# By TinLethax

UBOOT_FILE=$1
SPLASH_FILE=$2
KERNEL_FILE=$3
FS_FILE=$4
ENV_FILE="env.img"

# MTD Partition layout
# 0x00000000-0x00040000 : "U-Boot"
# 0x00040000-0x00080000 : "cal"
# 0x00080000-0x000c0000 : "environment"
# 0x000c0000-0x00100000 : "splash"
# 0x00100000-0x019c0000 : "fs1"
# 0x019c0000-0x01ec0000 : "fs2"
# 0x01ec0000-0x02000000 : "kernel"
# 0x00000000-0x02000000 : "whole_program_flash"

UBOOT_OFFSET=0
CAL_OFFSET=1
ENV_OFFSET=2
SPLASH_OFFSET=3
FS1_SIZE=17563648
FS1_OFFSET=4
FS2_SIZE=5242880
FS2_OFFSET=103
KERNEL_OFFSET=123

FOUND_NVKEYS="no"

echo "Fusion ROM image creator V1.0"
echo "By TinLethax"

if [ $# != 4 ]
then
	echo "Usage : fusion_rom_creator.sh [u_boot.img] [splash.img] [kernel.img] [filesystem.tar.gz]"
	echo "Arguments are not equal to four! quiting..."
	exit
fi

if ! test -e $1
then
	echo "U boot image not found!"
	exit
fi

if ! test -e $2
then
	echo "Splash image not found!"
	exit
fi

if ! test -e $3
then
	echo "Kernel image not found!"
	exit
fi

if ! test -e $4
then
	echo "File system tar not found!"
	exit
fi

if ! test -e $ENV_FILE
then
	echo "Environment file not found! Try git clone it again?"
	exit
fi

if ! test -e fusion_rom_splitter
then
	echo "Fusion Rom Splitter tool is missing, please compile it and put in the same folder as this script!"
	exit
fi

echo "Creating base image..."
dd if=/dev/zero of=fusion_rom.bin bs=1MiB count=32 >/dev/null 2>&1
sync
sync

echo "Writing U-boot ..."
dd if=$UBOOT_FILE of=fusion_rom.bin conv=notrunc >/dev/null 2>&1
sync
sync

echo "Writing Environment ..."
dd if=$ENV_FILE of=fusion_rom.bin bs=256KiB seek=$ENV_OFFSET conv=notrunc >/dev/null 2>&1
sync 
sync

echo "Writing Splash image..."
dd if=$SPLASH_FILE of=fusion_rom.bin bs=256KiB seek=$SPLASH_OFFSET conv=notrunc >/dev/null 2>&1
sync
sync

echo "Writing Kernel image..."
dd if=$KERNEL_FILE of=fusion_rom.bin bs=256KiB seek=$KERNEL_OFFSET conv=notrunc >/dev/null 2>&1
sync
sync

echo "Extracting rootfs file, will need sudo..."
if ! test -e rootfs
then
	mkdir rootfs
fi
sudo tar -xzf $FS_FILE -C rootfs

# looking for "nvkeys" folder in the same directory with this script
# you can put your original nvkey dumps if you have it
if  test -e nvkeys
then
	FOUND_NVKEYS="yes"
	echo "Found nvkeys! copying..."
	cp nvkeys/usr/local/perm/* rootfs/usr/local/perm/
fi

echo "Creating temporary FS1 partition image ..."
echo "Erase size : 256KiB Page Size : 256KiB"
mkfs.jffs2 -b -q -n -e256 -p0x40000 -r rootfs -o fs1.img

echo "Writing FS1..."
dd if=fs1.img of=fusion_rom.bin bs=256KiB seek=$FS1_OFFSET conv=notrunc >/dev/null 2>&1
sync
sync

echo "Creating FS2 folder to also store nv files"
if ! test -e rootfs2
then
	mkdir -p rootfs2/usr/local
fi

if test $FOUND_NVKEYS = "yes" 
then
	echo "Copying rootfs2 NV..."
	cp -r nvkeys/usr/local/nv rootfs2/usr/local/
else
	echo "No nvkeys, will just create an empty FS2 partition..."
fi

echo "Creating temporary FS2 partition image ..."
echo "Erase size : 256KiB Page Size : 256KiB"
mkfs.jffs2 -b -q -n -e256 -p0x40000 -r rootfs2 -o fs2.img

echo "Writing FS2..."
dd if=fs2.img of=fusion_rom.bin bs=256KiB seek=$FS2_OFFSET conv=notrunc >/dev/null 2>&1
sync
sync

if test -e fusion_rom_splitter
then
	echo "Creating the split image, this might take a while..."
	./fusion_rom_splitter fusion_rom.bin rom_a_u3700.bin rom_b_u3701.bin
fi

echo "Cleaning up..."
if test -e rootfs
then
	sudo rm -rf rootfs
fi

if test -e rootfs2
then
	sudo rm -rf rootfs2
fi

if test -e fs1.img
then
	sudo rm -rf fs1.img
fi

if test -e fs2.img
then
	sudo rm -rf fs2.img
fi

echo "Done!"
exit
