// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "crypto/common.h"
#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "version.h"

uint256 CBlockOldHeader::GetHash2(uint256 hash, int nType, int nVersion) const
{
    CHashWriter hasher(nType, nVersion);
    hasher << hash;
    hasher << nNonce;
    hasher << nNonce_ex;
    return hasher.GetHash();
}

uint256 CBlockOldHeader::GetHash() const
{
    //return SerializeHash(*this);
    return GetHash2(SerializeHash(*this), SER_GETHASH, PROTOCOL_VERSION);
}

uint256 CBlockOldHeader::GetHashWithoutSign() const
{
    //return SerializeHash(*this, SER_GETHASH | SER_WITHOUT_SIGNATURE);
    return GetHash2(SerializeHash(*this, SER_GETHASH | SER_WITHOUT_SIGNATURE), SER_GETHASH | SER_WITHOUT_SIGNATURE, PROTOCOL_VERSION);
}

int CBlockHeader::index()
{
    if (sig1.empty())
        return 0;
    if (sig2.empty())
        return 1;
    if (sig3.empty())
        return 2;
    if (sig4.empty())
        return 3;
    if (sig5.empty())
        return 4;
    return 5;
}


std::string CBlock::ToString() const
{
    std::stringstream s;
    //s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, hashStateRoot=%s, hashUTXORoot=%s, blockSig=%s, proof=%s, prevoutStake=%s, vtx=%u)\n",
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, nNonce_ex=%u,hashStateRoot=%s, hashUTXORoot=%s, proof=%s, vtx=%u ,seed=%u  /n",
        GetHash().ToString(),
        nVersion,                        //ver=0x%08x
        hashPrevBlock.ToString(),        //hashPrevBlock
        hashMerkleRoot.ToString(),       //hashMerkleRoot
        nTime, nBits, nNonce, nNonce_ex, // nTime=%u, nBits=%08x, nNonce=%u,, nNonce_ex=%u,
        hashStateRoot.ToString(),        // tsc hashStateRoot
        hashUTXORoot.ToString(),         // tsc hashUTXORoot
                                         // HexStr(vchBlockSig), //blockSig
        IsProofOfStake() ? "PoS" : "PoW",
        //prevoutStake.ToString(),//prevoutStake
        vtx.size(),
        seed);
    for (const auto& tx : vtx) {
        if (tx) {
            s << "  " << tx->ToString() << "\n";
        }
    }
    return s.str();
}
