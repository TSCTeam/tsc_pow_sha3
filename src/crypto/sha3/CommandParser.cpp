#include "stdafx.h"
#include "CommandParser.h"
#include "Keccak.h"
#include "ParserCommon.h"
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>

unsigned int hashType = 0;
unsigned int hashWidth = 224;  //hash 默认
unsigned int shakeDigestLength = 512;
unsigned int sha3widths[] = {224, 256, 384, 512, 0};
unsigned int keccakwidths[] = {224, 256, 384, 512, 0};
unsigned int shakewidths[] = {128, 256, 0};

unsigned int bufferSize = 1024 * 4;
char *buf = NULL;

template<typename F>
int readFileIntoFunc(const char *fileName, F f) 
{
	FILE *fHand = fopen(fileName, "rb");
	if (!fHand)
	{
		printf("Unable to open input file: %s\n", fileName);
		return 0;
	}
	fseek(fHand, 0, SEEK_SET);
	buf = new char[bufferSize];
	while (true)
	{
		unsigned int bytesRead = fread(buf, 1, bufferSize, fHand);
		f((uint8_t*)buf, bytesRead);
		if (bytesRead < bufferSize)
		{
			break;
		}
	}
	delete[] buf;

	fclose(fHand);
	return 1;
};

template<typename F>
int readDataIntoFunc(const char *data, size_t len, F f)
{
	buf = new char[bufferSize];// 申明新的BUF
	unsigned int pos = 0;
	unsigned int bytesRead  = bufferSize;
	while (true)
	{
		if (len - pos < bufferSize)
		{
	    	bytesRead = len - pos;
		}
		memcpy(buf, data+pos,bufferSize); // 依次读入buffsize = 4096 个数据
		f((uint8_t*)buf, bytesRead);  // 将buf update到sha3的buffer中
		if (bytesRead  < bufferSize)
		{
			break;
		}
		pos= pos+ bufferSize;
	}
	delete[] buf;
	return 1;
};


template<typename F1, typename F2>
int hashFile(const char *fileName, const char *hashName, F1 initFunc, F2 finalFunc)
{
	unsigned int hashSize = hashWidth;
	auto *st = initFunc(hashSize);

	if (readFileIntoFunc(fileName, [st](uint8_t* buf, unsigned int bLength){ keccakUpdate(buf, 0, bLength, st); }) == 0)
	{
		return 0;
	}

	vector<unsigned char> op = finalFunc(st);

	printf("%s-%u file : ", hashName, hashSize);
	for (auto &oi : op)
	{
		printf("%.2x", oi);
	}
	printf("\n");
	return 1;
}

int doFile(const char *fileName)
{
	if(hashType==0)
	{
		//  SHA-3

		return hashFile(fileName, "SHA-3", 
			[](unsigned int hs){ return keccakCreate(hs);  }, 
			[](keccakState *st){ return sha3Digest(st); });
	}
	else if (hashType == 1)
	{
		// Keccak

		return hashFile(fileName, "Keccak", 
			[](unsigned int hs){ return keccakCreate(hs);  }, 
			[](keccakState *st){ return keccakDigest(st); });
	}
	else if (hashType == 2)
	{
		// SHAKE

		return hashFile(fileName, "SHAKE", 
			[](unsigned int hs){ return shakeCreate(hs, shakeDigestLength);  }, 
			[](keccakState *st){ return shakeDigest(st); });

	}
	return 0;
}


template<typename F1, typename F2>
int hashData(const char *data, size_t len , const char *hashName, F1 initFunc, F2 finalFunc)
{
	unsigned int hashSize = hashWidth;
	auto *st = initFunc(hashSize);

	if (readDataIntoFunc(data, len, [st](uint8_t* buf, unsigned int bLength){ keccakUpdate(buf, 0, bLength, st); }) == 0)
	{
		return 0;
	}

	vector<unsigned char> op = finalFunc(st);

	printf("%s-%u text : ", hashName, hashSize );
	for (auto &oi : op)
	{
		printf("%.2x", oi);
	}
	printf("\n");
	return 1;
}


int doData(const char *data , size_t len)
{
	if(hashType==0)
	{
		//  SHA-3

		return hashData(data,len, "SHA-3",
			[](unsigned int hs){ return keccakCreate(hs);  },
			[](keccakState *st){ return sha3Digest(st); });
	}
	else if (hashType == 1)
	{
		// Keccak

		return hashData(data,len, "Keccak",
			[](unsigned int hs){ return keccakCreate(hs);  },
			[](keccakState *st){ return keccakDigest(st); });
	}
	else if (hashType == 2)
	{
		// SHAKE

		return hashData(data,len, "SHAKE",
			[](unsigned int hs){ return shakeCreate(hs, shakeDigestLength);  },
			[](keccakState *st){ return shakeDigest(st); });

	}
	return 0;
}


int FileExists(const char *fileName)
{
	FILE *fHand = fopen(fileName, "r");
	if (!fHand)
	{
		return false;
	}
	fclose(fHand);
	return true;
}


void usage()
{
	puts("\n\nUsage: sha3sum [command]* file*/text  \n"
	"\n"
	" where command is an optional parameter that can set either the algorithm, as\n"
	" there is a slight difference between the bare keccak function and the SHA-3\n"
	" variant.\n"
	"\n" 
	" Algorithm \n"
	"\n" 
	" -a=s   :  Set algorithm to SHA-3 (default).\n"
	" -a=k   :  Set algotithm to Keccak.\n"
	" -a=h   :  Set algotithm to SHAKE.\n"
	"\n" 
	" Size\n"
	" \n"
	" -w=224 :  Set width to 224 bits(default).\n"
	" -w=256 :  Set width to 256 bits.\n"
	" -w=384 :  Set width to 384 bits.\n"
	" -w=512 :  Set width to 512 bits.\n"
	"\n"
	" Digest size (SHAKE)\n"
	"\n"
	" -d=number : Set the SHAKE digest size. Should be less than or equal to the hash size.\n"
	"             should be multiple of 8.\n"
	"             Only relevant for SHAKE - For SHA-3 and keccak, digest size is equal to sponge size.\n"
	"\n"
	"Any number of files can be specified. Files will be processed with the most\n"
	"recently specified options - for example:\n"
	"\n"
	"  ./sha3sum test.txt -a=k -w=384 test.txt -a=h -w=512 test.txt\n"
	"\n"
	"will hash file \"test.txt\" three times - First with 224-bit SHA-3, then with 384-bit\n"
	"keccak, then finally with 512-bit SHAKE.\n"
	"\n"
	" ./sha3sum \"asdfghjklmnb\" -a=k -w=384 \"asdfghjklmnb\" -a=h -w=512 \"asdfghjklmnb\"\n"
	"\n"
	"will hash text \"asdfghjklmnb\"  three times - First with 224-bit SHA-3, then with 384-bit\n"
	"keccak, then finally with 512-bit SHAKE.\n");
}

int parseAlg(const char *param, const unsigned int pSize)
{
	unsigned int index = 0;
	if(param[index] == '=')
	{
		index++;
	}

	if(index + 1 == pSize)
	{
		const char algInitial = param[index];
		if(algInitial == 'k')
		{
			hashType = 1;
			return 1;
		}
		else if(algInitial == 's')
		{
			hashType = 0;
			return 1;
		}
		else if (algInitial == 'h')
		{
			hashType = 2;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

int parseWidth(const char *param, const unsigned int pSize)
{
	unsigned int index = 0;
	if(param[index] == '=')
	{
		index++;
	}

	if(index+3 == pSize)
	{
		if(strncmp(&param[index], "224", pSize-index)==0)
		{
			hashWidth = 224;
			return 1;
		}
		else if(strncmp(&param[index], "256", pSize-index)==0)
		{
			hashWidth = 256;
			return 1;	
		}
		else if(strncmp(&param[index], "384", pSize-index)==0)
		{
			hashWidth = 384;
			return 1;
		}
		else if(strncmp(&param[index], "512", pSize-index)==0)
		{
			hashWidth = 512;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}

}

int parseDigestWidth(const char *param, const unsigned int pSize)
{
	unsigned int index = 0;
	if (param[index] == '=')
	{
		index++;
	}

	if (index + 3 == pSize)
	{
		if (isNumeric(&param[index], pSize - index))
		{
			unsigned int sdl = atoun(&param[index], pSize - index);
			if (sdl % 8 != 0)
			{
				fprintf(stderr, "Error - param: %s is not divisible by 8.\n", param+index);
				return 0;
			}
			if (sdl > hashWidth)
			{
				fprintf(stderr, "Error - param: %s is greater than the hash size.\n", param+index);
				return 0;
			}
			shakeDigestLength = sdl;
			return 1;
		}
		else
		{
			fprintf(stderr, "Error - param: %s is not numeric. \n", param+index);
			return 0;
		}
	}
	else
	{
		fprintf(stderr, "Error - param: %s - expecting a three digit number.\n", param+index);
		return 0;
	}

}

int parseOption(const char *param, const unsigned int pSize)
{
	unsigned int index = 1;

	if(index != pSize)
	{
		const char commandInitial = param[index];
		if(commandInitial == 'h')
		{
			if((index + 1) == pSize)
			{
				usage();
				return 0;
			}
			else
			{
				return 0;
			}
		}
		else if(commandInitial == 'a')
		{
			return parseAlg(&param[index+1], pSize-(index+1));	
		}
		else if(commandInitial == 'w')
		{
			return parseWidth(&param[index+1], pSize-(index+1));
		}
		else if (commandInitial == 'd')
		{
			return parseDigestWidth(&param[index + 1], pSize - (index + 1));
		}
		else
		{
			fprintf(stderr, "Error - Unrecognised option %s.\n", param);
			return 0;	
		}
	}
	else 
	{
		fprintf(stderr, "Error - malformed option.\n");
		return 0;
	}
}

void parseParameter(const char *param)
{
	unsigned int index = 0;
	unsigned int paramSize = 0;

	paramSize = strlen(param);

	// Eat leading whitespace
	for(unsigned int i = index ; i != paramSize ; i++)
	{
		const char posI = param[i];
		if((posI != ' ') && (posI != '\t'))
		{
			index = i;
			break;
		}
	}

	if(index != paramSize)
	{
		if(param[index] != '-')
		{
			if (FileExists(&param[index])==true)
	     	{
		     	doFile(&param[index]);
	     	}
			else
			{
				doData(&param[index], strlen(&param[index]));
			}
		}
		else
		{
			parseOption(&param[index], paramSize-index);	
		}
	}
}

void parseCommandLine(const int argc, char* argv[])
{
	if(argc > 1)
	{
		for(unsigned int i = 1 ; i != argc ; i++)
		{
			parseParameter(argv[i]);
		}	
	}
}

int main(int argc, char* argv[])
{
	parseCommandLine(argc, argv);
	return 0;
}
