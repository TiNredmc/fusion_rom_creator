// Fusion Cal tool. Generate or Decode the factory and SPC calibration constant of the DPO/MSO2000
// Coded by TinLethax

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct __attribute__((packed)){
	uint32_t 	u32CalStringLen;		// Cal Name length (Non-null terminated)
	char	 	*cCalStringPtr;		// Cal Name string without null termination
	uint32_t 	u32CalReadCount;		// Cal array element count
	uint32_t 	u32CalDataType;			// Cal data type 0->BYTE, 1->SHORT, 2->LONG, 3->FLOAT. Used for type-casting
	uint32_t	*p32CalData;			// actual Cal data 
}tTekCalConst;

typedef struct __attribute__((packed)){
	uint32_t	u32EndStringLen;		// Always = 3
	char		cEnd[3];				// 'E', 'N', 'D'
}tTekCalEnd;

// Private prototypes
int vFusion_handleDecode(char *calfilename);

FILE 	*fpCalFile;
char 	 cCalParamName[112];
uint16_t u16CalCheckSumStore = 0;
uint32_t u32CalReadBuf = 0;
uint32_t u32CalDataCount = 0;
uint32_t u32CalDataType = 0;
uint32_t u32CalDataSize = 0;
	
int main(int argc, char *argv[]){
	

	printf("Fusion Cal Tool\n");
	
	if(argc != 3){
		printf("Arguments is not equal to 3!\n");
		return -1;
	}
	
	if(strcmp(argv[1], "G") == 0){
		printf("Generate calibration data...\n");
		
	}else if(strcmp(argv[1], "D") == 0){
		printf("Decode calibration data...\n");
		return i32Fusion_handleDecode(argv[2]);
	}

	printf("Done!\n");
	return 0;
}

void vFusion_calculateChecksum(){
	
	
}

int i32Fusion_handleDecode(char *calfilename){
	printf("Opening cal file %s\n", calfilename);
	
	fpCalFile = fopen(calfilename, "rb");
	if(fpCalFile == NULL){
		printf("Can't open %s !\n", calfilename);
		return -1;
	}
	
	printf("Begin parsing...\n");
	
	fread(&u16CalCheckSumStore, sizeof(uint16_t), 1, fpCalFile);
	printf("Cal Checksum : \t0x%04X\n", u16CalCheckSumStore);
	
	while(1){
		// Read the cal name length
		fread(&u32CalReadBuf, sizeof(uint32_t), 1, fpCalFile);
		if(u32CalReadBuf == 0){
			printf("Error! : Cal name length is zero?!?!?\n");
			return -1;
		}
		
		u32CalReadBuf = __builtin_bswap32(u32CalReadBuf);
		
		printf("Cal name length : %d\n", u32CalReadBuf);
		
		// Read the cal name
		fread(cCalParamName, 1, u32CalReadBuf, fpCalFile);
		cCalParamName[u32CalReadBuf] = '\0';// Make C hapy with terminated C string
		// Check if it's the end
		if(strcmp(cCalParamName, "END") == 0){
			printf("***Reached Cal end of file***\n");
			fclose(fpCalFile);
			return 0;
		}	
		printf("Cal parameter name : \t%s\n", cCalParamName);
		
		// Read the cal data count
		fread(&u32CalDataCount, sizeof(uint32_t), 1, fpCalFile);
		u32CalDataCount = __builtin_bswap32(u32CalDataCount);
		fread(&u32CalDataType, sizeof(uint32_t), 1, fpCalFile);
		u32CalDataType = __builtin_bswap32(u32CalDataType);
		if(u32CalDataType == 0){
			u32CalDataSize = 1;
		}else if(u32CalDataType == 1){
			u32CalDataSize = 2;
		}else if(
			(u32CalDataType == 2) ||
			(u32CalDataType == 3)
		){
			u32CalDataSize = 4;
		}else{
			printf("Unknown cal data type %d! \n", u32CalDataType);
			fclose(fpCalFile);
			return -1;
		}
		
		printf("Cal data type : \t%s[%d]\n",
			u32CalDataType == 0 ? "byte" 	:
			(u32CalDataType == 1 ? "short" 	:
			(u32CalDataType == 2 ? "int"	:
			(u32CalDataType == 3 ? "float32" : "unknown"))) ,
			u32CalDataCount
			);
		
		for(unsigned int j = 0; j < u32CalDataCount; j++){
			fread(&u32CalReadBuf, u32CalDataSize, 1, fpCalFile);
			printf("Data[%d] : \t", j);
			if(u32CalDataType == 0){
				printf("0x%02X\t%d\t%d\n", (uint8_t)u32CalReadBuf, (int8_t)u32CalReadBuf, (uint8_t)u32CalReadBuf);
			}else if (u32CalDataType == 1){
				u32CalReadBuf = __builtin_bswap16(u32CalReadBuf);
				printf("0x%04X\t%d\t%d\n", (uint16_t)u32CalReadBuf,(int16_t)u32CalReadBuf, (uint16_t)u32CalReadBuf);
			}else if (u32CalDataType == 2){
				u32CalReadBuf = __builtin_bswap32(u32CalReadBuf);
				printf("0x%08X\t%d\t%d\n", (uint32_t)u32CalReadBuf, (int32_t)u32CalReadBuf, (uint32_t)u32CalReadBuf);
			}else if (u32CalDataType == 3){
				u32CalReadBuf = __builtin_bswap32(u32CalReadBuf);
				printf("0x%08X\t%f\n", (uint32_t)u32CalReadBuf, *(float *)&u32CalReadBuf);
			}else{
				printf("0x%08X (unknown type)\n", (uint32_t)u32CalReadBuf);
			}
		}
		
		
	}
	
}

int i32Fusion_attachToFile(
	FILE *fpOut,
	char *cCalParamString,
	uint32_t u32DataType,
	uint32_t u32DataCount,
	void 	*vDataPtr,
	){
	
	uint8_t u8ParamNameLen;
	
	if(fpOut == NULL)
		return -1;
	
	if(cCalParamString == NULL)
		return -1;
	
	if(strlen(cCalParamString) > 99)
		return -1;
	
	if(u32DataType > 3)
		return -1;
	
	printf("Attaching %d parameter...\n", cCalParamString);
	
	u8ParamNameLen = strlen(cCalParamString);
	
	// Write cal param name length
	fwrite(&u8ParamNameLen, 1, 1, fpOut);
	// Write cal param name
	fwrite(cCalParamString, 1, u8ParamNameLen - 1, fpOut);
	
	// Write cal Data Count
	fwrite(&u32DataCount, sizeof(uint32_t), 1, fpOut);
	// Write cal Data type
	fwrite(&u32DataType, sizeof(uint32_t), 1, fpOut);
	
	// Write the actual data
	if(u32DataType == 0){
		fwrite(vDataPtr, 1, u32DataCount, fpOut);
	}else if (u32DataType == 1){
		fwrite(vDataPtr, 2, u32DataCount, fpOut);
	}else if (u32DataType == 2){
		fwrite(vDataPtr, 4, u32DataCount, fpOut);
	}else if (u32DataType == 3){
		fwrite(vDataPtr, 4, u32DataCount, fpOut);
	}else{
		printf("0x%08X (unknown type)\n", (uint32_t)u32DataType);
		return -1;
	}

	return 0;
}

int i32Fusion_attachFactoryInfo(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Factory calibration info...\n");
	
	i32Fusion_attachToFile(fpOut, "facStatus", 1, 1, NULL);
	i32Fusion_attachToFile(fpOut, "facVersion", 2, 1, NULL);
	i32Fusion_attachToFile(fpOut, "facConstSize", 2, 1, NULL);
	i32Fusion_attachToFile(fpOut, "facDate", 0, 12, NULL);
	i32Fusion_attachToFile(fpOut, "facSerialNumber", 0, 10, "B000000000");
	i32Fusion_attachToFile(fpOut, "facVersionMajor", 2, 1, NULL);
	i32Fusion_attachToFile(fpOut, "facVersionMinor", 2, 1, NULL);
	i32Fusion_attachToFile(fpOut, "facVersionBuild", 0, 1, NULL);
	i32Fusion_attachToFile(fpOut, "facNumSteps", 2, 1, NULL);

}

int i32Fusion_attachAdc(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADC Range info...\n");
	
	// calAttachAdcCCs
	i32Fusion_attachToFile(fpOut, "AdcRange20MHzBW", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "AdcRangeFullBW", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "AdcOffset", 2, 4, NULL);
	
	return 0;
}

int i32Fusion_attachAdg420Offset(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADG420 Offset info...\n");
	
	i32Fusion_attachToFile(fpOut, "Adg420offset5mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset10mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset20mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset50mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset100mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset200mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset-5mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset-10mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset-20mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset-50mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset-100mV", 2, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420offset-200mV", 2, 4, NULL);
	
	
	return 0;
}

int i32Fusion_attachAdg420NullDac(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADG420 Null Dac info...\n");
	
	i32Fusion_attachToFile(fpOut, "Adg420NullDac", 0, 4, NULL);
	
	return 0;
}

int i32Fusion_attachAdg420LfComp(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADG420 LF Comp info...\n");
	
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_5mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_10mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_20mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_40mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_50mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_80mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_100mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_200mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_5mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_10mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_20mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_40mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_50mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_80mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_100mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_200mV_25X", 0, 4, NULL);
	
	return 0;
}

int i32Fusion_Deskew(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Deskew info...\n");
	
	i32Fusion_attachToFile(fpOut, "VertDeskewAtten_1X", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewAtten_25X", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_0", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_1", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_2", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_3", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_4", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_5", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_6", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_7", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_8", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_9", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_10", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_11", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_12", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_13", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_14", 3, 4, NULL);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_15", 3, 4, NULL);
	
	return 0;
}

int i32Fusion_BwLimit(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching 20Mhz Bandwidth Limit info...\n");
	
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_5mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_10mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_20mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_40mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_50mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_80mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_100mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_200mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_5mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_10mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_20mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_40mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_50mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_80mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_100mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_200mV_25X", 0, 4, NULL);
	
	printf("Attaching Full Bandwidth Limit info...\n");
	
	i32Fusion_attachToFile(fpOut, "bwLimitFull_5mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_10mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_20mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_40mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_50mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_80mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_100mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_200mV_1X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_5mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_10mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_20mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_40mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_50mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_80mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_100mV_25X", 0, 4, NULL);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_200mV_25X", 0, 4, NULL);
	

	return 0;
}

int i32Fusion_Tek0001(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Tek0001 info...\n");
	
	i32Fusion_attachToFile(fpOut, "tek0001SystemPllSample", 2, 2, NULL);
	i32Fusion_attachToFile(fpOut, "tek0001TrigIfPllSample", 2, 2, NULL);
	i32Fusion_attachToFile(fpOut, "tek0001TrigIfPllByteAlignment", 2, 2, NULL);
	i32Fusion_attachToFile(fpOut, "tek0001TrigIfPllTrigPlacement", 2, 2, NULL);
	
}

int i32Fusion_DigCmpDac(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Digital channels info...\n");
	
	
	i32Fusion_attachToFile(fpOut, "DigCmpGain_0_7", 3, 1, NULL);
	i32Fusion_attachToFile(fpOut, "DigCmpGain_8_15", 3, 1, NULL);
	i32Fusion_attachToFile(fpOut, "DigCmpFacOffset_0_7", 2, 1, NULL);
	i32Fusion_attachToFile(fpOut, "DigCmpFacOffset_8_15", 2, 1, NULL);
	i32Fusion_attachToFile(fpOut, "DigCmpSpcOffset_0_7", 2, 1, NULL);
	i32Fusion_attachToFile(fpOut, "DigCmpSpcOffset_8_15", 2, 1, NULL);
	
	return 0;
}