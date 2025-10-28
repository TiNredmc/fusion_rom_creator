#!/bin/bash
# DPO/MSO2000 ROM image creator
# By TinLethax

UBOOT_FILE=$1
UBOOT_OFFSET=0
SPLASH_FILE=$2
SPLASH_OFFSET=3
KERNEL_FILE=$3
KERNEL_OFFSET=123
FS_FILE=$4
FS1_SIZE=17563648
FS1_OFFSET=4
FS2_SIZE=5242880
FS2_OFFSET=103

echo "Fusion ROM image creator V1.0"
echo "By TinLethax"

if [ $# != 4 ]
then
	echo "Arguments are not equal to four! quiting..."
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
sudo tar -xzvf $FS_FILE -C rootfs

if  test -e nvkeys
then
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

cp -r nvkeys/usr/local/nv rootfs2/usr/local/

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
sudo rm -rf rootfs
sudo rm -rf rootfs2
sudo rm -rf fs1.img
sudo rm -rf fs2.img

echo "Done!"
exit
