// Copyright (c) 2019 The TSC-POW-SHA3 developers

#ifndef BITCOIN_CRYPTO_SHA3_H
#define BITCOIN_CRYPTO_SHA3_H

#include <crypto/sha3/Keccak.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>


/** A hasher class for SHA-3. */
class CSHA3
{
private:
	unsigned int bufferSize = 4096;
	unsigned int hashWidth = 256;  //默认是256输出位数
    char *buf = NULL;
    unsigned int hs = 0;
    struct keccakState *Kst;
public:
    static const size_t OUTPUT_SIZE = 32;  // 输出BUFF的字节数
    CSHA3& Write(const unsigned char* data, size_t len);  //写入需要HASH的数据和长度
    void Finalize(unsigned char hash[OUTPUT_SIZE]);   // HASH并输出256bit的HASH值
    CSHA3& Init();  //初始化
};

/** Autodetect the best available SHA3 implementation.
 *  Returns the name of the implementation.
// 测试性能使用的函数 , 暂时没什么必要使用, 因为SHA3只用在block header的hash上 ,netaddress tx hash 和 signer还是使用sha256,故保留原有测试环境
// 涉及如下文件
 *   src/bench/bench_bitcoin.cpp
 *   src/test/test_bitcoin.cpp
 *   src/init.cpp

 **/
std::string SHA3AutoDetect();
#endif // BITCOIN_CRYPTO_SHA3_H
