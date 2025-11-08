# fusion_rom_creator

Fusion Rom creator tool for recovering bricked DPO/MSO 2000 series oscilloscope.

# Setup

0. If runs on WSL. This must be run inside the linux home folder. Running this on the Windows NTFS drives will mess the file permission and owner
1. Download the latest firmware of DPO/MSO2000 from Tektronix website. Extract the firmware.img with 7z to obtain all of the content inside
2. Compile the fusion_rom_splitter.c in the same folder with the script with command
```
gcc fusion_rom_splitter.c -o fusion_rom_splitter
``` 

# Usage

Use the command below to generate the A (U3700 on the mainboard) and B (U3701 on the mainbord) ROM images for NOR flash IC  

```
./fusion_rom_creator.sh [path_to_uboot.img] [path_to_splash.img] [path_to_kernel.img] [path_to_filesystem.tar.gz]
```

# TODO

* Finish the Fusion Cal tool to generates the initial default factory calibration data
* Ability to populate serial numbers