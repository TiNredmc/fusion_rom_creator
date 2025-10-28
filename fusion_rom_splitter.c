// Fusion ROM splitter. Split the single DPO/MSO2000 ROM image into two ROM images for each NOR flash
// Coded by TinLethax
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	
	FILE *fpRom_A;
	FILE *fpRom_B;
	FILE *fpRom_in;
	uint32_t u32RomInLength;
	
	uint16_t u16RomABufWrite;
	uint16_t u16RomBBufWrite;
	uint32_t u32RomInBufRead;
	
	printf("Fusion Rom splitter\n");
	
	// Sanity check
	if(argc != 4){
		printf("Argument not equal to 4! Exiting...\n");
		printf("Usage : \n");
		printf("%s rom_in.bin rom_a.bin rom_b.bin\n", argv[0]);
		return -1;
	}
	
	// open file
	fpRom_in = fopen(argv[1], "rd");
	if(fpRom_in == NULL){
		printf("can't read ROM in file!\n");
		return -1;
	}
	
	fpRom_A = fopen(argv[2], "w");
	if(fpRom_A == NULL){
		printf("can't create ROM A !\n");
		return -1;
	}
	
	fpRom_B = fopen(argv[3], "w");
	if(fpRom_A == NULL){
		printf("can't create ROM B !\n");
		return -1;
	}
	
	// Get the ROM length
	fseek(fpRom_in, 0L, SEEK_END);
	u32RomInLength = ftell(fpRom_in);
	fseek(fpRom_in, 0L, SEEK_SET);

	while(u32RomInLength != 0){

		fread(&u32RomInBufRead, sizeof(uint32_t), 1, fpRom_in);

		u16RomABufWrite = __builtin_bswap16((uint16_t)(u32RomInBufRead));
		u16RomBBufWrite = __builtin_bswap16((uint16_t)(u32RomInBufRead >> 16));
		
		fwrite(&u16RomABufWrite, sizeof(uint16_t), 1, fpRom_A);
		fwrite(&u16RomBBufWrite, sizeof(uint16_t), 1, fpRom_B);
		
		u32RomInLength -= 4;
	}
	
	fclose(fpRom_A);
	fclose(fpRom_B);
	fclose(fpRom_in);
	
	printf("Done!\n");
	
	return 0;
}