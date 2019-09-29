// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"

// ppcoin: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    //CBlockIndex will be updated with information about the proof type later
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

inline arith_uint256 GetLimit(const Consensus::Params& params, bool fProofOfStake)
{
    return fProofOfStake ? UintToArith256(params.posLimit) : UintToArith256(params.powLimit);
}

//                                           chainActive.Tip()                    pblock                            params [12/11/2018 taoist]
//unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
	unsigned int  nTargetLimit = UintToArith256(params.powLimit).GetCompact();

	// Only change once per difficulty adjustment interval [12/12/2018 taoist]
	//如果新区块不需要进行难度调整，即新区块的高度不能被2016整除，则用父区块的难度目标即可
	 if ((pindexLast->nHeight + 1 ) % params.DifficultyAdjustmentInterval() != 0 )
	 {

			// min difficulty
		   if (params.fPowAllowMinDifficultyBlocks)
		   {
		       // Special difficulty rule for testnet:
		       // If the new block's timestamp is more than 2* 10 minutes
		       // then allow mining of a min-difficulty block.
		       if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
		           return nTargetLimit;
		       else
		       {
		           // Return the last non-special-min-difficulty-rules-block
		           const CBlockIndex* pindex = pindexLast;
		           while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nTargetLimit)
		               pindex = pindex->pprev;
		           return pindex->nBits;
		       }
		   }
		   return pindexLast->nBits;
	 }
	 // The first of a cycle 一个周期的第一个 [12/12/2018 taoist]
	 int nChangeFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval() - 1);
	 assert(nChangeFirst >= 0);
	 const CBlockIndex* pindexFrist =pindexLast->GetAncestor(nChangeFirst) ;
	 assert(pindexFrist != nullptr);
	 unsigned int temp= CalculateNextWorkRequired(pindexLast, pindexFrist->GetBlockTime(), params);
	return temp;
}

//unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params, bool fProofOfStake)
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&params)
{
	// not change nbit [12/12/2018 taoist]
	if (params.fPowNoRetargeting)
	{
		return pindexLast->nBits;
	}
    // Limit adjustment step
	int64_t nTargetTimeSpacing = params.nPowTargetTimespan;
    int64_t nActualTimeSpacing = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimeSpacing < nTargetTimeSpacing/4)
		nActualTimeSpacing = nTargetTimeSpacing/4;
    if (nActualTimeSpacing > nTargetTimeSpacing * 4)
		nActualTimeSpacing = nTargetTimeSpacing * 4;

	// Retarget
	const arith_uint256 bnTargetLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
	bnNew *= nActualTimeSpacing;
	bnNew /= nTargetTimeSpacing;

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;
    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params, bool fProofOfStake)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget; 

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

	// 去除判断pos段内容，只应该保留pow部分 [12/6/2018 taoist]
    // Check range
	if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
	{
		return false;
	}
    // Check proof of work matches claimed amount
	if (UintToArith256(hash) > bnTarget)
	{
		return false;
	}

    return true;
}
