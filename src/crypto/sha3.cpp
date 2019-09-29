// Copyright (c) 2019 The TSC-POW-SHA3 developers

#include "crypto/sha3/Keccak.h"
#include "crypto/sha3.h"
#include "crypto/common.h"
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string>


CSHA3& CSHA3::Init()
{
    Kst = keccakCreate(hashWidth);
	return *this;
}

CSHA3& CSHA3::Write(const unsigned char* data, size_t len)
{
	buf = new char[bufferSize];
	unsigned int pos = 0;
	unsigned int bytesRead  = bufferSize;
	while (true)
	{
		if (len - pos < bufferSize)
		{
	    	bytesRead = len - pos;
		}
		memcpy(buf, data+pos, bytesRead);
		keccakUpdate((uint8_t*)buf, 0, bytesRead, Kst);
		if (bytesRead  < bufferSize)
		{
			break;
		}
		pos= pos+ bytesRead;
	}
	delete[] buf;
    return *this;
}

void CSHA3::Finalize(unsigned char hash[OUTPUT_SIZE])
{
	memcpy(hash,&*sha3Digest(Kst).begin(),OUTPUT_SIZE);
	delete[] Kst->A;
	delete[] Kst->buffer;
	delete Kst;
}

std::string SHA3AutoDetect()
{
    return "standard";
}

