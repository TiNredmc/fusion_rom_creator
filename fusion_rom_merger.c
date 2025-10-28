// Fusion ROM merger. Merge the separate DPO/MSO2000 ROM dump into one
// Coded by TinLethax
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	
	FILE *fpRom_A;
	uint32_t u32RomALength;
	FILE *fpRom_B;
	uint32_t u32RomBLength;
	FILE *fpRom_out;
	
	uint16_t u16RomABufRead;
	uint16_t u16RomBBufRead;
	uint32_t u32RomOutBufWrite;
	
	printf("Fusion Rom merger\n");
	
	// Sanity check
	if(argc != 4){
		printf("Argument not equal to 4! Exiting...\n");
		printf("Usage : \n");
		printf("%s rom_a.bin rom_b.bin rom_out.bin\n", argv[0]);
		return -1;
	}
	
	// open file
	fpRom_A = fopen(argv[1], "rb");
	if(fpRom_A == NULL){
		printf("can't open ROM A !\n");
		return -1;
	}
	
	fpRom_B = fopen(argv[2], "rb");
	if(fpRom_A == NULL){
		printf("can't open ROM B !\n");
		return -1;
	}
	
	fpRom_out = fopen(argv[3], "w");
	if(fpRom_out == NULL){
		printf("can't create ROM out file!\n");
		return -1;
	}
	
	// Check ROM_A and ROM_B size and compare if they both match
	// Get ROM A length
	fseek(fpRom_A, 0L, SEEK_END);
	u32RomALength = ftell(fpRom_A);
	fseek(fpRom_A, 0L, SEEK_SET);
	
	if(u32RomALength != 0x01000000){
		printf("ROM A is not equal to 16MiB! (%d)\n", u32RomALength);
		return -1;
	}
	
	// Get ROM B Lenght
	fseek(fpRom_B, 0L, SEEK_END);
	u32RomBLength = ftell(fpRom_B);
	fseek(fpRom_B, 0L, SEEK_SET);
	
	if(u32RomBLength != 0x01000000){
		printf("ROM B is not equal to 16MiB! (%d)\n", u32RomBLength);
		return -1;
	}
	
	if(u32RomALength != u32RomBLength){
		printf("ROM A and ROM B are not the same length!\n");
		return -1;
	}

	while(u32RomALength != 0){
		fread(&u16RomABufRead, sizeof(uint16_t), 1, fpRom_A);
		fread(&u16RomBBufRead, sizeof(uint16_t), 1, fpRom_B);
		
		u32RomOutBufWrite = 
			__builtin_bswap16(u16RomABufRead) 			|
			(__builtin_bswap16(u16RomBBufRead) << 16)	;
		
		fwrite(&u32RomOutBufWrite, sizeof(uint32_t), 1, fpRom_out);
		
		u32RomALength -= 2;
	}
	
	fclose(fpRom_A);
	fclose(fpRom_B);
	fclose(fpRom_out);
	
	printf("Done!\n");
	
	return 0;
}