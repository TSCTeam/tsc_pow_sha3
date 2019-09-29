// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"
#include <string>
//#include "uint128_t.h"

static const int SER_WITHOUT_SIGNATURE = 1 << 3;

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockOldHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint256 hashStateRoot; // tsc
    uint256 hashUTXORoot; // tsc
	uint32_t seed;
	uint64_t nNonce;
	uint64_t nNonce_ex;
    // proof-of-stake specific fields

	//  [12/13/2018 taoist]
    //COutPoint prevoutStake;
   // std::vector<unsigned char> vchBlockSig;

    CBlockOldHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(hashStateRoot); // tsc
        READWRITE(hashUTXORoot); // tsc
		READWRITE(seed);
	//  TSC-POW-SHA3 @ 2019
		if (!(s.GetType() & SER_GETHASH)){
		READWRITE(nNonce);
		READWRITE(nNonce_ex);
		}
        //READWRITE(prevoutStake);
        //if (!(s.GetType() & SER_WITHOUT_SIGNATURE))
        //    READWRITE(vchBlockSig);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        hashStateRoot.SetNull(); // tsc
        hashUTXORoot.SetNull(); // tsc
		seed = 0;
		nNonce = 0;
		nNonce_ex = 0;
		//  [12/13/2018 taoist]
       // vchBlockSig.clear();
        //prevoutStake.SetNull();
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }
    uint256 GetHash2(uint256 hash,int nType, int nVersion) const;

    uint256 GetHash() const;

    uint256 GetHashWithoutSign() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
    //  [12/13/2018 taoist]
    // ppcoin: two types of block: proof-of-work or proof-of-stake
    virtual bool IsProofOfStake() const //tsc
    {
		return false;
    }

    virtual bool IsProofOfWork() const
    {
        return !IsProofOfStake();
    }
    
    virtual uint32_t StakeTime() const
    {
        uint32_t ret = 0;
        if(IsProofOfStake())
        {
            ret = nTime;
        }
        return ret;
    }

    CBlockOldHeader& operator=(const CBlockOldHeader& other) //tsc
    {
        if (this != &other)
        {
            this->nVersion       = other.nVersion;
            this->hashPrevBlock  = other.hashPrevBlock;
            this->hashMerkleRoot = other.hashMerkleRoot;
            this->nTime          = other.nTime;
            this->nBits          = other.nBits;
            this->nNonce         = other.nNonce;
            this->hashStateRoot  = other.hashStateRoot;
            this->hashUTXORoot   = other.hashUTXORoot;
			this->seed           = other.seed;
			this->nNonce_ex      = other.nNonce_ex;
        //  [12/13/2018 taoist] 
			//     this->vchBlockSig    = other.vchBlockSig;
         //   this->prevoutStake   = other.prevoutStake;
        }
        return *this;
    }
};


class CBlockHeader : public CBlockOldHeader
{
public:
    // header
    std::string sig1;
	std::string sig2;
	std::string sig3;
	std::string sig4;
	std::string sig5; 
    CBlockHeader()
    {
       SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
       READWRITE(*(CBlockOldHeader*)this);
	   READWRITE(sig1);
	   READWRITE(sig2);
	   READWRITE(sig3);
	   READWRITE(sig4);
	   READWRITE(sig5);
    }

    void SetNull()
    {
       CBlockOldHeader::SetNull();
	   sig1.clear();
	   sig2.clear();
	   sig3.clear();
	   sig4.clear();
	   sig5.clear();
    }

    bool IsNull() const
    {
        return CBlockOldHeader::IsNull();
    }

	int index();
    

    CBlockOldHeader GetBlockOldHeader() const
    {
        CBlockOldHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
		block.nNonce_ex      = nNonce_ex;

		// ����Ҫע�� [12/6/2018 taoist]
        block.hashStateRoot  = hashStateRoot; // tsc
        block.hashUTXORoot   = hashUTXORoot; // tsc
		block.seed = seed;
		return block;
    }

    CBlockHeader& operator=(const CBlockHeader& other) //tsc
    {
        if (this != &other)
        {
            this->nVersion       = other.nVersion;
            this->hashPrevBlock  = other.hashPrevBlock;
            this->hashMerkleRoot = other.hashMerkleRoot;
            this->nTime          = other.nTime;
            this->nBits          = other.nBits;
            this->nNonce         = other.nNonce;
			this->nNonce_ex      = other.nNonce_ex;
            this->hashStateRoot  = other.hashStateRoot;
            this->hashUTXORoot   = other.hashUTXORoot;
			this->seed = other.seed;
			this->sig1   = other.sig1;
			this->sig2   = other.sig2;
			this->sig3   = other.sig3;
			this->sig4   = other.sig4;
			this->sig5   = other.sig5;
        //  [12/13/2018 taoist] 
			//     this->vchBlockSig    = other.vchBlockSig;
         //   this->prevoutStake   = other.prevoutStake;
        }
        return *this;
    }
};



class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

	void ChangeHeader(CBlockHeader &header)
    {
		
	    this->nVersion       = header.nVersion;
        this->hashPrevBlock  = header.hashPrevBlock;
        this->hashMerkleRoot = header.hashMerkleRoot;
        this->nTime          = header.nTime;
        this->nBits          = header.nBits;
        this->nNonce         = header.nNonce;
		this->nNonce_ex      = header.nNonce_ex;

		// ����Ҫע�� [12/6/2018 taoist]
        this->hashStateRoot  = header.hashStateRoot; // tsc
        this->hashUTXORoot   = header.hashUTXORoot; // tsc
		this->seed           = header.seed;
        this->sig1           = header.sig1;
		this->sig2           = header.sig2;
		this->sig3           = header.sig3;
		this->sig4           = header.sig4;
		this->sig5           = header.sig5;
	}

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

	//	 [12/13/2018 taoist]
    std::pair<COutPoint, unsigned int> GetProofOfStake() const //tsc
    {
		// return IsProofOfStake()? std::make_pair(prevoutStake, nTime) : std::make_pair(COutPoint(), (unsigned int)0);
		 return std::make_pair(COutPoint(), (unsigned int)0);

    }
    
    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
		block.nNonce_ex      = nNonce_ex;

		// ����Ҫע�� [12/6/2018 taoist]
        block.hashStateRoot  = hashStateRoot; // tsc
        block.hashUTXORoot   = hashUTXORoot; // tsc
		block.seed = seed;
        block.sig1           = sig1;
		block.sig2           = sig2;
		block.sig3           = sig3;
		block.sig4           = sig4;
		block.sig5           = sig5;
        //  [12/13/2018 taoist]
		//block.vchBlockSig    = vchBlockSig;
        //block.prevoutStake   = prevoutStake;
        
		return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
