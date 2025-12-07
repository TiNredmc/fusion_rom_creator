// Fusion Cal tool. Generate or Decode the factory and SPC calibration constant of the DPO/MSO2000
// Coded by TinLethax

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FACTORY_PARTITION_SIZE	0x40000

enum{
	DATATYPE_BYTE 	= 0,
	DATATYPE_SHORT 	= 1,
	DATATYPE_LONG 	= 2,
	DATATYPE_FLOAT	= 3
};

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

typedef struct __attribute__((packed)){
	uint32_t	facStatus;
	uint32_t	facVersion;
	uint32_t	facConstSize;
	
	uint8_t 	facTimeSec;
	uint8_t		facTimeMin;
	uint8_t		facTimeHour;
	uint8_t		facTimeDay;
	uint16_t	facTimeMonth;
	uint32_t	facTimeYear;
	
	uint16_t	reserved1;
	
	char		facSerialNumber[12];
	uint32_t	facNumSteps;
	uint32_t	facVersionMajor;
	uint32_t	facVersionMinor;
	uint32_t	facVersionBuild;
}tCalFactory_t;

// Private prototypes
int i32Fusion_handleDecode(char *calfilename);
int i32Fusion_handleGenerate(char *calfilename);

int i32Fusion_attachToFile(
	FILE *fpOut,
	char *cCalParamString,
	uint32_t u32DataType,
	uint32_t u32DataCount,
	void 	*vDataPtr
	);
int i32Fusion_attachFactoryInfo(FILE *fpOut);
int i32Fusion_attachAdc(FILE *fpOut);
int i32Fusion_attachAdg420Offset(FILE *fpOut);
int i32Fusion_attachAdg420NullDac(FILE *fpOut);
int i32Fusion_attachAdg420LfComp(FILE *fpOut);
int i32Fusion_Deskew(FILE *fpOut);
int i32Fusion_BwLimit(FILE *fpOut);
int i32Fusion_Tek0001(FILE *fpOut);
int i32Fusion_DigCmpDac(FILE *fpOut);


FILE 	*fpCalFile;
char 	 cCalParamName[112];
uint16_t u16CalCheckSumStore = 0;
uint16_t u16CalCheckSumCalculate = 0;
uint32_t u32CalReadWriteBuf = 0;
uint32_t u32CalDataCount = 0;
uint32_t u32CalDataType = 0;
uint32_t u32CalDataSize = 0;

uint32_t u32CalNameLen = 0;
	
uint32_t u32Mod7 = 0;
uint32_t u32Mod7Counter = 0;
uint32_t u32SumAccumulator = 0;	
	
/***********************************************************************/	
tCalFactory_t tFactoryConst = {
	.facStatus			=	1,
	.facVersion			=	0,
	.facConstSize		=	0x354,
	
	.facTimeSec			=	0,
	.facTimeMin			=	0,
	.facTimeHour		=	8,
	.facTimeDay			=	6,
	.facTimeMonth		= 	12,
	.facTimeYear		= 	2025,
	
	.facSerialNumber	= 	"RBCLUB!",// Serial number of the scope
	.facNumSteps		= 	87,
	.facVersionMajor	= 	1,	// Firmware verison major
	.facVersionMinor	=	35,	// Firmware verison minor
	.facVersionBuild	= 	0
};	

uint32_t u32AdcRangeDefault[4] = {
	0x00000100,
	0x00000100,
	0x00000100,
	0x00000100
};

uint32_t u32AdcOffDefault[4] = {0};

int32_t i32Adg420OffsetCCDefault5mV[4] = {20, 20, 20, 20};
int32_t i32Adg420OffsetCCDefault10mV[4] = {40, 40, 40, 40};
int32_t i32Adg420OffsetCCDefault20mV[4] = {80, 80, 80, 80};
int32_t i32Adg420OffsetCCDefault50mV[4] = {200, 200, 200, 200};
int32_t i32Adg420OffsetCCDefault100mV[4] = {400, 400, 400, 400};
int32_t i32Adg420OffsetCCDefault200mV[4] = {800, 800, 800, 800};
int32_t i32Adg420OffsetCCDefault500mV[4] = {2000, 2000, 2000, 2000};
int32_t i32Adg420OffsetCCDefault1V[4] = {4000, 4000, 4000, 4000};

int32_t i32Adg420OffsetCCDefault_5mV[4] = {-20, -20, -20, -20};
int32_t i32Adg420OffsetCCDefault_10mV[4] = {-40, -40, -40, -40};
int32_t i32Adg420OffsetCCDefault_20mV[4] = {-80, -80, -80, -80};
int32_t i32Adg420OffsetCCDefault_50mV[4] = {-200, -200, -200, -200};
int32_t i32Adg420OffsetCCDefault_100mV[4] = {-400, -400, -400, -400};
int32_t i32Adg420OffsetCCDefault_200mV[4] = {-800, -800, -800, -800};
int32_t i32Adg420OffsetCCDefault_500mV[4] = {-2000, -2000, -2000, -2000};
int32_t i32Adg420OffsetCCDefault_1V[4] = {-4000, -4000, -4000, -4000};


uint8_t u8Adg420NullDacDefault[4] = {0x80, 0x80, 0x80, 0x80};

uint8_t u8Adg420LfCompDefault[4] = {0x2E, 0x2E, 0x2E, 0x2E};

float f32Deskew1xAttenDefault[4] = {
	0.0f, 0.0f, 0.0f, 0.0f
};

float f32Deskew25xAttenDefault[4] = {
	-4.9999999E-10f,
	-4.9999999E-10f,
	-4.9999999E-10f,
	-4.9999999E-10f
};

float f32DeskewDefault[4] = {
	0.0f, 0.0f, 0.0f, 0.0f
};

uint8_t u8BwLimit20MhzDefault[4] = {0x02, 0x02, 0x02, 0x02};
uint8_t u8BwLimitFullDefault[4] = {0x0F, 0x0F, 0x0F, 0x0F};

uint32_t u32Tek0001SystemPllSampleDefault[2] = {0x00000006, 0x00000006};
uint32_t u32Tek0001TrigIfPllSampleDefault[2] = {0x00000006, 0x00000006};
uint32_t u32Tek0001TrigPllByteAlignmentDefault[2] = {0x00000010, 0x00000010};
uint32_t u32Tek0001TrigIfPllTrigPlacementDefault[2] = {0x00000000, 0x00000000};

float f32DigCmpGainDefault[4] = {
	89.0f, 89.0f, 89.0f, 89.0f
};

uint32_t u32DigCmpOffsetDefault[4] = {
	0x00000000, 
	0x00000010, 
	0x00000010, 
	0x00000010
};

/***********************************************************************/
	
int main(int argc, char *argv[]){
	

	printf("Fusion Cal Tool\n");
	
	if(argc != 3){
		printf("Arguments is not equal to 3!\n");
		return -1;
	}
	
	if(strcmp(argv[1], "G") == 0){
		printf("Generate calibration data...\n");
		return i32Fusion_handleGenerate(argv[2]);
	}else if(strcmp(argv[1], "D") == 0){
		printf("Decode calibration data...\n");
		return i32Fusion_handleDecode(argv[2]);
	}

	printf("Done!\n");
	return 0;
}

int i32Fusion_calculateChecksum(FILE *fpOut, uint8_t isWrite){
	if(fpOut == NULL)
		return -1;
	
	if(isWrite == 0){// Seek to first byte when we want to read the check sum
		fseek(fpOut, 0, SEEK_SET);
		fread(&u16CalCheckSumStore, sizeof(uint16_t), 1, fpCalFile);
		printf("Cal stored Checksum : \t0x%04X\n", u16CalCheckSumStore);
	}else{// Seek to thrid byte to start reading from file buffer to calculate the checksum
		fseek(fpOut, 2, SEEK_SET);
	}
	
	u16CalCheckSumCalculate = 0;
	
	u32Mod7 = 0;
	u32Mod7Counter = 0;
	u32SumAccumulator = 0;
	
	while (fread(&u32CalReadWriteBuf,1,1,fpOut) == 1) {
      u32Mod7 = u32Mod7Counter & 0b00000111;
      u32Mod7Counter = u32Mod7Counter + 1;
      u32SumAccumulator =
           u32SumAccumulator +
           (u32CalReadWriteBuf << u32Mod7) + (u32CalReadWriteBuf >> (7 - u32Mod7 & 0b00111111));
    }
	
	u16CalCheckSumCalculate = __builtin_bswap16(u32SumAccumulator & 0xFFFF);
	
	printf("Cal calculated Checksum : \t0x%04X\n", u16CalCheckSumCalculate);
	if(isWrite == 0){
		printf("Cal checksum : %s\n", (u16CalCheckSumCalculate == u16CalCheckSumStore) ? "matched" : "MISMATCHED!");
		return (u16CalCheckSumCalculate != u16CalCheckSumStore);
	}else{
		printf("Storing checksum to file...\n");
		fseek(fpOut, 0, SEEK_SET);
		fwrite(&u16CalCheckSumCalculate, sizeof(uint16_t), 1, fpCalFile);
		printf("Checksum wrote!\n");
		return 0;
	}
}

int i32Fusion_appendCalEnd(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	u32CalReadWriteBuf = 3;
	u32CalReadWriteBuf = __builtin_bswap32(u32CalReadWriteBuf);
	fwrite(&u32CalReadWriteBuf, sizeof(uint32_t), 1, fpOut);
	fwrite("END", 3, 1, fpOut);
	
	return 0;
}

int i32Fusion_handleDecode(char *calfilename){
	printf("Opening cal file %s\n", calfilename);
	
	fpCalFile = fopen(calfilename, "rb");
	if(fpCalFile == NULL){
		printf("Can't open %s !\n", calfilename);
		return -1;
	}
	
	printf("Begin parsing...\n");
	
	i32Fusion_calculateChecksum(fpCalFile, false);
	fseek(fpCalFile, 2, SEEK_SET);
	
	while(1){
		// Read the cal name length
		fread(&u32CalReadWriteBuf, sizeof(uint32_t), 1, fpCalFile);
		if(u32CalReadWriteBuf == 0){
			printf("Error! : Cal name length is zero?!?!?\n");
			fclose(fpCalFile);
			return -1;
		}
		
		u32CalReadWriteBuf = __builtin_bswap32(u32CalReadWriteBuf);
		
		printf("Cal name length : %d\n", u32CalReadWriteBuf);
		
		// Read the cal name
		fread(cCalParamName, 1, u32CalReadWriteBuf, fpCalFile);
		cCalParamName[u32CalReadWriteBuf] = '\0';// Make C hapy with terminated C string
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
		if(u32CalDataType == DATATYPE_BYTE){
			u32CalDataSize = 1;
		}else if(u32CalDataType == DATATYPE_SHORT){
			u32CalDataSize = 2;
		}else if(
			(u32CalDataType == DATATYPE_LONG) ||
			(u32CalDataType == DATATYPE_FLOAT)
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
			fread(&u32CalReadWriteBuf, u32CalDataSize, 1, fpCalFile);
			printf("Data[%d] : \t", j);
			if(u32CalDataType == 0){
				printf("0x%02X\t%d\t%d\n", (uint8_t)u32CalReadWriteBuf, (int8_t)u32CalReadWriteBuf, (uint8_t)u32CalReadWriteBuf);
			}else if (u32CalDataType == 1){
				u32CalReadWriteBuf = __builtin_bswap16(u32CalReadWriteBuf);
				printf("0x%04X\t%d\t%d\n", (uint16_t)u32CalReadWriteBuf,(int16_t)u32CalReadWriteBuf, (uint16_t)u32CalReadWriteBuf);
			}else if (u32CalDataType == 2){
				u32CalReadWriteBuf = __builtin_bswap32(u32CalReadWriteBuf);
				printf("0x%08X\t%d\t%d\n", (uint32_t)u32CalReadWriteBuf, (int32_t)u32CalReadWriteBuf, (uint32_t)u32CalReadWriteBuf);
			}else if (u32CalDataType == 3){
				u32CalReadWriteBuf = __builtin_bswap32(u32CalReadWriteBuf);
				printf("0x%08X\t%f\n", (uint32_t)u32CalReadWriteBuf, *(float *)&u32CalReadWriteBuf);
			}else{
				printf("0x%08X (unknown type)\n", (uint32_t)u32CalReadWriteBuf);
			}
		}
		
		
	}
	
}

int i32Fusion_handleGenerate(char *calfilename){
	printf("Cearting cal file %s\n", calfilename);
	
	fpCalFile = fopen(calfilename, "w+");
	if(fpCalFile == NULL){
		printf("Can't create %s !\n", calfilename);
		return -1;
	}
	
	// Prewrite an empty checksum
	u16CalCheckSumCalculate = 0;
	fwrite(&u16CalCheckSumCalculate, sizeof(uint16_t), 1, fpCalFile);
	
	if(i32Fusion_attachFactoryInfo(fpCalFile)){
		printf("ERROR : Can't write Factory Info!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_attachAdc(fpCalFile)){
		printf("ERROR : Can't write ADC range and offset!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_attachAdg420Offset(fpCalFile)){
		printf("ERROR : Can't write ADG420 AFE offset!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_attachAdg420NullDac(fpCalFile)){
		printf("ERROR : Can't write ADG420 Null DAC!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_attachAdg420LfComp(fpCalFile)){
		printf("ERROR : Can't write ADG420 Low Pass compensation!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_Deskew(fpCalFile)){
		printf("ERROR : Can't write Deskew data!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_BwLimit(fpCalFile)){
		printf("ERROR : Can't write Bandwidth limit data!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_Tek0001(fpCalFile)){
		printf("ERROR : Can't write ASIC Tek0001 data!\n");
		fclose(fpCalFile);
		return -1;
	}else if(i32Fusion_DigCmpDac(fpCalFile)){
		printf("ERROR : Can't write Digital Channel Gain and offset!\n");
		fclose(fpCalFile);
		return -1;	
	}else if(i32Fusion_appendCalEnd(fpCalFile)){
		printf("ERROR : Can't write calibration end!\n");
		fclose(fpCalFile);
		return -1;		
	}else if(i32Fusion_calculateChecksum(fpCalFile, true)){
		printf("ERROR : Can't write Checksum!\n");
		fclose(fpCalFile);
		return -1;
	}
	
	printf("***Cal end of file***\n");
	fclose(fpCalFile);
}

int i32Fusion_attachToFile(
	FILE *fpOut,
	char *cCalParamString,
	uint32_t u32DataType,
	uint32_t u32DataCount,
	void 	*vDataPtr
	){
	
	if(fpOut == NULL)
		return -1;
	
	if(cCalParamString == NULL)
		return -1;
	
	if(strlen(cCalParamString) > 99)
		return -1;
	
	if(u32DataType > 3)
		return -1;
	
	printf("Attaching parameter : %s\n", cCalParamString);
	u32CalNameLen = strlen(cCalParamString);
	// Write cal param name length
	u32CalNameLen = __builtin_bswap32(u32CalNameLen);
	fwrite(&u32CalNameLen, sizeof(uint32_t), 1, fpOut);
	// Write cal param name 
	u32CalNameLen = __builtin_bswap32(u32CalNameLen);
	fwrite(cCalParamString, 1, u32CalNameLen, fpOut);
	
	// Write cal Data Count
	u32CalDataCount 	= __builtin_bswap32(u32DataCount);
	fwrite(&u32CalDataCount, sizeof(uint32_t), 1, fpOut);
	
	// Write cal Data type
	u32CalDataType		= __builtin_bswap32(u32DataType);
	fwrite(&u32CalDataType, sizeof(uint32_t), 1, fpOut);
	
	if(u32DataType > DATATYPE_FLOAT){
		printf("0x%08X unknown data type!\n", u32DataType);
		return -1;
	}
	
	// Write the actual data
	for(unsigned int j = 0; j < u32DataCount; j++){
		if(u32DataType == DATATYPE_BYTE){
			u32CalReadWriteBuf = *((uint8_t *)vDataPtr + j);
			fwrite(&u32CalReadWriteBuf, 1, 1, fpOut);
		}else if (u32DataType == DATATYPE_SHORT){
			u32CalReadWriteBuf = *((uint16_t *)vDataPtr + j);
			u32CalReadWriteBuf = __builtin_bswap16(u32CalReadWriteBuf);
			fwrite(&u32CalReadWriteBuf, 2, 1, fpOut);
		}else if (u32DataType == DATATYPE_LONG){
			u32CalReadWriteBuf = *((uint32_t *)vDataPtr + j);
			u32CalReadWriteBuf = __builtin_bswap32(u32CalReadWriteBuf);
			fwrite(&u32CalReadWriteBuf, 4, 1, fpOut);
		}else if (u32DataType == DATATYPE_FLOAT){
			u32CalReadWriteBuf = *((uint32_t *)vDataPtr + j);
			u32CalReadWriteBuf = __builtin_bswap32(u32CalReadWriteBuf);
			fwrite(&u32CalReadWriteBuf, 4, 1, fpOut);
		}
	}

	return 0;
}

int i32Fusion_attachFactoryInfo(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Factory calibration info...\n");
	
	i32Fusion_attachToFile(fpOut, "facStatus", DATATYPE_SHORT, 1, &tFactoryConst.facStatus);
	i32Fusion_attachToFile(fpOut, "facVersion", DATATYPE_LONG, 1, &tFactoryConst.facVersion);
	i32Fusion_attachToFile(fpOut, "facConstSize", DATATYPE_LONG, 1, &tFactoryConst.facConstSize);
	i32Fusion_attachToFile(fpOut, "facDate", DATATYPE_BYTE, 12, &tFactoryConst.facTimeSec);
	i32Fusion_attachToFile(fpOut, "facSerialNumber", DATATYPE_BYTE, 10, &tFactoryConst.facSerialNumber);
	i32Fusion_attachToFile(fpOut, "facVersionMajor", DATATYPE_LONG, 1, &tFactoryConst.facVersionMajor);
	i32Fusion_attachToFile(fpOut, "facVersionMinor", DATATYPE_LONG, 1, &tFactoryConst.facVersionMinor);
	i32Fusion_attachToFile(fpOut, "facVersionBuild", DATATYPE_BYTE, 1, &tFactoryConst.facVersionBuild);
	i32Fusion_attachToFile(fpOut, "facNumSteps", DATATYPE_LONG, 1, &tFactoryConst.facNumSteps);

}

int i32Fusion_attachAdc(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADC Range info...\n");
	
	// calAttachAdcCCs
	i32Fusion_attachToFile(fpOut, "AdcRange20MHzBW", DATATYPE_LONG, 4, &u32AdcRangeDefault);
	i32Fusion_attachToFile(fpOut, "AdcRangeFullBW", DATATYPE_LONG, 4, &u32AdcRangeDefault);
	i32Fusion_attachToFile(fpOut, "AdcOffset", DATATYPE_LONG, 4, &u32AdcOffDefault);
	
	return 0;
}

int i32Fusion_attachAdg420Offset(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADG420 Offset info...\n");
	
	i32Fusion_attachToFile(fpOut, "Adg420offset5mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault5mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset10mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault10mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset20mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault20mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset50mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault50mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset100mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault100mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset200mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault200mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset500mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault500mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset1V", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault1V);
	i32Fusion_attachToFile(fpOut, "Adg420offset-5mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_5mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset-10mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_10mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset-20mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_20mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset-50mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_50mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset-100mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_100mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset-200mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_200mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset-500mV", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_500mV);
	i32Fusion_attachToFile(fpOut, "Adg420offset-1V", DATATYPE_LONG, 4, &i32Adg420OffsetCCDefault_1V);
	
	return 0;
}

int i32Fusion_attachAdg420NullDac(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADG420 Null Dac info...\n");
	
	i32Fusion_attachToFile(fpOut, "Adg420NullDac", DATATYPE_BYTE, 4, u8Adg420NullDacDefault);
	
	return 0;
}

int i32Fusion_attachAdg420LfComp(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching ADG420 LF Comp info...\n");
	
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_5mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_10mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_20mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_40mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_50mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_80mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_100mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_200mV_1X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_5mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_10mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_20mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_40mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_50mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_80mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_100mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	i32Fusion_attachToFile(fpOut, "Adg420LfComp_200mV_25X", DATATYPE_BYTE, 4, &u8Adg420LfCompDefault);
	
	return 0;
}


int i32Fusion_Deskew(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Deskew info...\n");
	
	i32Fusion_attachToFile(fpOut, "VertDeskewAtten_1X", DATATYPE_FLOAT, 4, &f32Deskew1xAttenDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewAtten_25X", DATATYPE_FLOAT, 4, &f32Deskew25xAttenDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_0", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_1", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_2", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_3", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_4", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_5", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_6", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_7", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_8", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_9", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_10", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_11", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_12", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_13", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_14", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	i32Fusion_attachToFile(fpOut, "VertDeskewBwFilt_15", DATATYPE_FLOAT, 4, &f32DeskewDefault);
	
	return 0;
}


int i32Fusion_BwLimit(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching 20Mhz Bandwidth Limit info...\n");
	
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_5mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_10mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_20mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_40mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_50mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_80mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_100mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_200mV_1X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_5mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_10mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_20mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_40mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_50mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_80mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_100mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	i32Fusion_attachToFile(fpOut, "bwLimit20Mhz_200mV_25X", DATATYPE_BYTE, 4, &u8BwLimit20MhzDefault);
	
	printf("Attaching Full Bandwidth Limit info...\n");
	
	i32Fusion_attachToFile(fpOut, "bwLimitFull_5mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_10mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_20mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_40mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_50mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_80mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_100mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_200mV_1X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_5mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_10mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_20mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_40mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_50mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_80mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_100mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	i32Fusion_attachToFile(fpOut, "bwLimitFull_200mV_25X", DATATYPE_BYTE, 4, &u8BwLimitFullDefault);
	

	return 0;
}


int i32Fusion_Tek0001(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Tek0001 info...\n");
	
	i32Fusion_attachToFile(fpOut, "tek0001SystemPllSample", DATATYPE_LONG, 2, &u32Tek0001SystemPllSampleDefault);
	i32Fusion_attachToFile(fpOut, "tek0001TrigIfPllSample", DATATYPE_LONG, 2, &u32Tek0001TrigIfPllSampleDefault);
	i32Fusion_attachToFile(fpOut, "tek0001TrigIfPllByteAlignment", DATATYPE_LONG, 2, &u32Tek0001TrigPllByteAlignmentDefault);
	i32Fusion_attachToFile(fpOut, "tek0001TrigIfPllTrigPlacement", DATATYPE_LONG, 2, &u32Tek0001TrigIfPllTrigPlacementDefault);
	
}

int i32Fusion_DigCmpDac(FILE *fpOut){
	if(fpOut == NULL)
		return -1;
	
	printf("Attaching Digital channels info...\n");
	
	
	i32Fusion_attachToFile(fpOut, "DigCmpGain_0_7", DATATYPE_FLOAT, 1, &f32DigCmpGainDefault);
	i32Fusion_attachToFile(fpOut, "DigCmpGain_8_15", DATATYPE_FLOAT, 1, &f32DigCmpGainDefault);
	i32Fusion_attachToFile(fpOut, "DigCmpFacOffset_0_7", DATATYPE_LONG, 1, &u32DigCmpOffsetDefault);
	i32Fusion_attachToFile(fpOut, "DigCmpFacOffset_8_15", DATATYPE_LONG, 1, &u32DigCmpOffsetDefault);
	i32Fusion_attachToFile(fpOut, "DigCmpSpcOffset_0_7", DATATYPE_LONG, 1, &u32DigCmpOffsetDefault);
	i32Fusion_attachToFile(fpOut, "DigCmpSpcOffset_8_15", DATATYPE_LONG, 1, &u32DigCmpOffsetDefault);
	
	return 0;
}
