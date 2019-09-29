// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "chain.h"
#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/tx_verify.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "hash.h"
#include "validation.h"
#include "net.h"
#include "policy/feerate.h"
#include "policy/policy.h"
#include "pow.h"
#include "pos.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"
#include "validationinterface.h"
#include "wallet/wallet.h"

#include <algorithm>
#include <queue>
#include <utility>

#include "base58.h"
//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest fee rate of a transaction combined with all
// its ancestors.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockWeight = 0;
int64_t nLastCoinStakeSearchInterval = 0;
unsigned int nMinerSleep = STAKER_POLLING_PERIOD;

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);

    return nNewTime - nOldTime;
}

BlockAssembler::Options::Options() {
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
}

BlockAssembler::BlockAssembler(const CChainParams& params, const Options& options) : chainparams(params)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit weight to between 4K and dgpMaxBlockWeight-4K for sanity:
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(dgpMaxBlockWeight - 4000, options.nBlockMaxWeight));
}

static BlockAssembler::Options DefaultOptions(const CChainParams& params)
{
    // Block resource limits
    // If neither -blockmaxsize or -blockmaxweight is given, limit to DEFAULT_BLOCK_MAX_*
    // If only one is given, only restrict the specified resource.
    // If both are given, restrict both.
    BlockAssembler::Options options;
    options.nBlockMaxWeight = gArgs.GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    if (gArgs.IsArgSet("-blockmintxfee")) {
        CAmount n = 0;
        ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n);
        options.blockMinFeeRate = CFeeRate(n);
    } else {
        options.blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    }
    return options;
}

BlockAssembler::BlockAssembler(const CChainParams& params) : BlockAssembler(params, DefaultOptions(params)) {}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    fIncludeWitness = false;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;
}
// 退款？？ [12/7/2018 taoist]
void BlockAssembler::RebuildRefundTransaction(){
    int refundtx=0; //0 for coinbase in PoW
    if(pblock->IsProofOfStake()){
        refundtx=1; //1 for coinstake in PoS
    }
    CMutableTransaction contrTx(originalRewardTx);
    contrTx.vout[refundtx].nValue = nFees + (GetBlockSubsidy(nHeight, chainparams.GetConsensus())/10) * 7;
    contrTx.vout[refundtx].nValue -= bceResult.refundSender;
    //note, this will need changed for MPoS
    int i=contrTx.vout.size();
    contrTx.vout.resize(contrTx.vout.size()+bceResult.refundOutputs.size());
    for(CTxOut& vout : bceResult.refundOutputs){
        contrTx.vout[i]=vout;
        i++;
    }
    pblock->vtx[refundtx] = MakeTransactionRef(std::move(contrTx));
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fMineWitnessTx, bool fProofOfStake, int64_t* pTotalFees, int32_t txProofTime, int32_t nTimeLimit)
 {

    int64_t nTimeStart = GetTimeMicros();

	resetBlock();// set<类型一，类型二> ？？？？？ [12/6/2018 taoist]

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    pblock = &pblocktemplate->block; // pointer for convenience
	LogPrint(BCLog::MINER,"pblock <1> :%s\n", pblock->ToString());
    this->nTimeLimit = nTimeLimit;

    // Add dummy coinbase tx as first transaction
	//先添加一个虚设的coinbase交易作为区块的第一个交易，相当于占位，因为每个区块的第一个交易必须是coinbase交易，后面会初始化这个伪coinbase交易
    pblock->vtx.emplace_back();

    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();//获取当前主链的末端区块，作为新区块的父区块

	// 确定CBlockIndex 不为空 [12/6/2018 taoist]
	assert(pindexPrev != nullptr);

    nHeight = pindexPrev->nHeight + 1;//新区块的高度是=当前主链的高度+1

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetArg("-blockversion", pblock->nVersion);

	pblock->nTime = GetAdjustedTime();//计算并填写区块的时间戳字段
  

    pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());
	LogPrint(BCLog::MINER,"pblock <2> : %s\n",pblock->ToString());
	const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nMedianTimePast
                       : pblock->GetBlockTime();

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization) or when
    // -promiscuousmempoolflags is used.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus()) && fMineWitnessTx;

    int64_t nTime1 = GetTimeMicros();

    nLastBlockTx = nBlockTx;
    nLastBlockWeight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(6);
	LogPrint(BCLog::TICKETPOOL,"coinbaseTx.vout.size=%d\n",coinbaseTx.vout.size());
	//std::cout<<"hello-3"<<std::endl;

	 //luckyticket.getLuckyIndex(chainActive.Height());

	//the miner can get only 70% coinbase reward
	coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;

	coinbaseTx.vout[0].nValue = 1000000000;

	LogPrint(BCLog::TICKETPOOL,"coinbaseTx.vout[0].nValue = %d\n",coinbaseTx.vout[0].nValue);
	
	//tempticketpool 
	ticketPool temppool = ticketpool;

	if (chainActive.Height() >= TICKET_VALVE)
	{
		//lucky ticket
		std::vector<unsigned int > luckyIndex = luckyticket.getLuckyIndex(chainActive.Height(),nNextChoice);
		for (auto index : luckyIndex)
		{
			LogPrint(BCLog::ADDPACKAGETX," index :%d\n" , index);
		}
		std::vector<CBitcoinAddress> vluckyaddress;

		std::vector<CAmount> vluckyaddress_price;
		LogPrint(BCLog::ADDPACKAGETX,"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& \n");
		for (auto index : luckyIndex)
		{
			Ticket lucky_ticket = temppool.ticketAt(index);
			//ticketpool.ticketDel(lucky_ticket);
			CBitcoinAddress lucky_address = lucky_ticket.ticketAddress;
			vluckyaddress_price.push_back(lucky_ticket.ticketPrice);
			vluckyaddress.push_back(lucky_address);
			temppool.ticketDel(lucky_ticket);
			LogPrint(BCLog::TICKETPOOL,"index %d,lucky_ticket = %s\n",index,lucky_ticket.toString());
		}
		

		//LOG_IF(TICKET_POOL_LOG, INFO) << "vluckyaddress_price[0] * COIN " << vluckyaddress_price[0] * COIN;
		coinbaseTx.vout[1].scriptPubKey = GetScriptForDestination(vluckyaddress[0].Get());
		LogPrint(BCLog::TICKETPOOL,"vluckyaddress[0].ToString() = %s\n",vluckyaddress[0].ToString());
		//LOG_IF(TICKET_POOL_LOG, INFO) <<" coinbaseTx.vout[1].scriptPubKey " << coinbaseTx.vout[1].scriptPubKey;
		coinbaseTx.vout[1].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3 + vluckyaddress_price[0] ;
		coinbaseTx.vout[2].scriptPubKey = GetScriptForDestination(vluckyaddress[1].Get());
		coinbaseTx.vout[2].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3 + vluckyaddress_price[1] ;
		coinbaseTx.vout[3].scriptPubKey = GetScriptForDestination(vluckyaddress[2].Get());
		coinbaseTx.vout[3].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3 + vluckyaddress_price[2] ;
		coinbaseTx.vout[4].scriptPubKey = GetScriptForDestination(vluckyaddress[3].Get());;
		coinbaseTx.vout[4].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3 + vluckyaddress_price[3] ;
		coinbaseTx.vout[5].scriptPubKey = GetScriptForDestination(vluckyaddress[4].Get());;
		coinbaseTx.vout[5].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3 + vluckyaddress_price[4] ;
	}
	else
	{
		CBitcoinAddress address(default_pos_miner);
		coinbaseTx.vout[1].scriptPubKey = GetScriptForDestination(address.Get());
		coinbaseTx.vout[1].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3;
		coinbaseTx.vout[2].scriptPubKey = GetScriptForDestination(address.Get());;
		coinbaseTx.vout[2].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3;
		coinbaseTx.vout[3].scriptPubKey = GetScriptForDestination(address.Get());;
		coinbaseTx.vout[3].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3;
		coinbaseTx.vout[4].scriptPubKey = GetScriptForDestination(address.Get());;
		coinbaseTx.vout[4].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3;
		coinbaseTx.vout[5].scriptPubKey = GetScriptForDestination(address.Get());;
		coinbaseTx.vout[5].nValue = (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) / 50) * 3;
	}
	LogPrint(BCLog::TICKETPOOL,"coinbaseTx.vout.size 1 = %d\n",coinbaseTx.vout.size());
	LogPrint(BCLog::TICKETPOOL,"chainActive.Height() = %d\n",chainActive.Height());
	//std::cout<<"hello1"<<std::endl;
    if(chainActive.Height() >= TICKET_POOL_MAX_SIZE)
    {   
		
		std::vector<Ticket> expiredtickets = temppool.getTicketByHeight(chainActive.Height() - TICKET_POOL_MAX_SIZE + 1);
		LogPrint(BCLog::TICKETPOOL," -------------------------------------------- \n");
		LogPrint(BCLog::TICKETPOOL,"ticketpool.ticket_pool.size() = %d\n",ticketpool.ticket_pool.size());
		LogPrint(BCLog::TICKETPOOL,"temppool.ticket_pool.size()=%d \n ", temppool.ticket_pool.size());
		LogPrint(BCLog::TICKETPOOL,"chainActive.Height() - TICKET_POOL_MAX_SIZE + 1 =%d\n ",chainActive.Height() - TICKET_POOL_MAX_SIZE + 1);
		LogPrint(BCLog::TICKETPOOL, "expiredtickets =%d \n ",expiredtickets.size());
		coinbaseTx.vout.resize(6 + expiredtickets.size());
		LogPrint(BCLog::TICKETPOOL,"chainActive.Height()=%d\n ",chainActive.Height());
		LogPrint(BCLog::TICKETPOOL,"coinbaseTx.vout.size 2 = %d\n",coinbaseTx.vout.size());
		LogPrint(BCLog::TICKETPOOL," ------------------------------------------ \n");
		int counts = 0;
		for (auto ticket : expiredtickets)
		{  

			counts ++;
			coinbaseTx.vout[5+counts].scriptPubKey = GetScriptForDestination(ticket.ticketAddress.Get());
		    coinbaseTx.vout[5+counts].nValue = ticket.ticketPrice;
		}
		
	}
	LogPrint(BCLog::TICKETPOOL,"coinbaseTx.vout.size3 = %d\n",coinbaseTx.vout.size());
	//std::cout<<"hello2"<<std::endl;
	// 输出log [12/11/2018 taoist]
	LogPrint(BCLog::MINER,"miner.cpp fee: %d \n",nFees + (GetBlockSubsidy(nHeight, chainparams.GetConsensus()) /10) * 7);
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    originalRewardTx = coinbaseTx;// ？？？？？？？？ [12/6/2018 taoist]
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
	LogPrint(BCLog::MINER," pblock  <3> :%s \n" , pblock->ToString());
    //////////////////////////////////////////////////////// tsc
    TscDGP tscDGP(globalState.get(), fGettingValuesDGP);
    globalSealEngine->setTscSchedule(tscDGP.getGasSchedule(nHeight));
    uint32_t blockSizeDGP = tscDGP.getBlockSize(nHeight);
    minGasPrice = tscDGP.getMinGasPrice(nHeight);
    if(gArgs.IsArgSet("-staker-min-tx-gas-price")) {
        CAmount stakerMinGasPrice;
        if(ParseMoney(gArgs.GetArg("-staker-min-tx-gas-price", ""), stakerMinGasPrice)) {
            minGasPrice = std::max(minGasPrice, (uint64_t)stakerMinGasPrice);
        }
    }
    hardBlockGasLimit = tscDGP.getBlockGasLimit(nHeight);
    softBlockGasLimit = gArgs.GetArg("-staker-soft-block-gas-limit", hardBlockGasLimit);
    softBlockGasLimit = std::min(softBlockGasLimit, hardBlockGasLimit);
    txGasLimit = gArgs.GetArg("-staker-max-tx-gas-limit", softBlockGasLimit);

    nBlockMaxWeight = blockSizeDGP ? blockSizeDGP * WITNESS_SCALE_FACTOR : nBlockMaxWeight;
    
    dev::h256 oldHashStateRoot(globalState->rootHash());
    dev::h256 oldHashUTXORoot(globalState->rootHashUTXO());
    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
	LogPrint(BCLog::MINER," pblock  <4> :%s\n",pblock->ToString());
	//根据交易选择算法从内存池中选择交易打包进区块，这个过程并不会把交易从内存池中移除，移除交易的过程是在后续的处理新区块的方法ProcessNewBlock()里
    int ticketNum = addPackageTxs(nPackagesSelected, nDescendantsUpdated, minGasPrice);
	
	LogPrint(BCLog::MINER," pblock  <5> :%s\n",pblock->ToString());

    pblock->hashStateRoot = uint256(h256Touint(dev::h256(globalState->rootHash())));
	pblock->hashUTXORoot = uint256(h256Touint(dev::h256(globalState->rootHashUTXO())));
	LogPrint(BCLog::MINER," pblock  <6> :%s\n",pblock->ToString());
	globalState->setRoot(oldHashStateRoot);
	globalState->setRootUTXO(oldHashUTXORoot);

/*
	if (chainActive.Height() > TICKET_VALVE &&  ticketpool.getTicketpoolSize() < MIN_TICKET_SIZE)
	{
		if (ticketNum <= 7)
		{
			LogPrint(BCLog::MINER," error: Number of ticket %d smaller than 7 " , ticketNum);
			return nullptr;
		}
	}
*/

	LogPrint(BCLog::MINER," pblock  <7> :%s\n",pblock->ToString());

	//this should already be populated by AddBlock in case of contracts, but if no contracts
	//then it won't get populated
	RebuildRefundTransaction();
	////////////////////////////////////////////////////////

	LogPrint(BCLog::MINER," pblock  <7> :%s\n",pblock->ToString());
	// 去除pos 添加pow [12/6/2018 taoist]
    //pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus(), fProofOfStake);
	pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus());
    
	pblocktemplate->vTxFees[0] = -nFees;
	
    LogPrintf("CreateNewBlock(): block weight: %u txs: %u fees: %ld sigops %d\n", GetBlockWeight(*pblock), nBlockTx, nFees, nBlockSigOpsCost);

    // The total fee is the Fees minus the Refund
    if (pTotalFees)
        *pTotalFees = nFees - bceResult.refundSender;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    pblock->nNonce         = 0;

	pblock->seed = nNextChoice;
	//std::cout<<"nNextChoice "<<nNextChoice<<std::endl;

	
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);
	LogPrint(BCLog::MINER," pblock  <8> :%s\n",pblock->ToString());
    CValidationState state;
	// 去除pos检测了只保留pow部分 [12/6/2018 taoist]
    //if (!fProofOfStake && !TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
    //    throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    //}


	//if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
	//	throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
	//}
    int64_t nTime2 = GetTimeMicros();

    LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));
	LogPrint(BCLog::TICKETPOOL,"coinbaseTx.vout.size =%d\n",coinbaseTx.vout.size());
    return std::move(pblocktemplate);
}

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateEmptyBlock(const CScript& scriptPubKeyIn, bool fMineWitnessTx, bool fProofOfStake, int64_t* pTotalFees, int32_t nTime)
{
    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    pblock = &pblocktemplate->block; // pointer for convenience

	// 可能加上 [12/7/2018 taoist]
	//this->nTimeLimit = nTimeLimit;

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
	// 去除pos [12/7/2018 taoist]
    //// Add dummy coinstake tx as second transaction
    //if(fProofOfStake)
    //    pblock->vtx.emplace_back();

    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex* pindexPrev = chainActive.Tip();
    nHeight = pindexPrev->nHeight + 1;

    pblock->nVersion = ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = gArgs.GetArg("-blockversion", pblock->nVersion);

    uint32_t txProofTime = nTime == 0 ? GetAdjustedTime() : nTime;
	//  [12/7/2018 taoist]
  /*  if(fProofOfStake)
        txProofTime &= ~STAKE_TIMESTAMP_MASK;*/
    pblock->nTime = txProofTime;
    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                       ? nMedianTimePast
                       : pblock->GetBlockTime();

    // Decide whether to include witness transactions
    // This is only needed in case the witness softfork activation is reverted
    // (which would require a very deep reorganization) or when
    // -promiscuousmempoolflags is used.
    // TODO: replace this with a call to main to assess validity of a mempool
    // transaction (which in most cases can be a no-op).
    fIncludeWitness = IsWitnessEnabled(pindexPrev, chainparams.GetConsensus()) && fMineWitnessTx;

    nLastBlockTx = nBlockTx;
    nLastBlockWeight = nBlockWeight;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);

	//  [12/7/2018 taoist]
    //if (fProofOfStake)
    //{
    //    // Make the coinbase tx empty in case of proof of stake
    //    coinbaseTx.vout[0].SetEmpty();
    //}
    //else
    //{
    //    coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
    //    coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    //}
	coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
	coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());

    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    originalRewardTx = coinbaseTx;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));

	// 不存在pos奖励交易 [12/7/2018 taoist]
    // Create coinstake transaction.
    //if(fProofOfStake)
    //{
    //    CMutableTransaction coinstakeTx;
    //    coinstakeTx.vout.resize(2);
    //    coinstakeTx.vout[0].SetEmpty();
    //    coinstakeTx.vout[1].scriptPubKey = scriptPubKeyIn;
    //    originalRewardTx = coinstakeTx;
    //    pblock->vtx[1] = MakeTransactionRef(std::move(coinstakeTx));

    //    //this just makes CBlock::IsProofOfStake to return true
    //    //real prevoutstake info is filled in later in SignBlock
    //    pblock->prevoutStake.n=0;
    //}

    //////////////////////////////////////////////////////// tsc
    //state shouldn't change here for an empty block, but if it's not valid it'll fail in CheckBlock later
    pblock->hashStateRoot = uint256(h256Touint(dev::h256(globalState->rootHash())));
    pblock->hashUTXORoot = uint256(h256Touint(dev::h256(globalState->rootHashUTXO())));

    RebuildRefundTransaction();
    ////////////////////////////////////////////////////////

	// 去除pos 添加pow [12/6/2018 taoist]
   // pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus(), fProofOfStake);
	pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus());
	pblocktemplate->vTxFees[0] = -nFees;

    // The total fee is the Fees minus the Refund
    if (pTotalFees)
        *pTotalFees = nFees - bceResult.refundSender;

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    if (!fProofOfStake)
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
	// 去除pos [12/7/2018 taoist]
    //pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus(),fProofOfStake);
	pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    CValidationState state;

	//  [12/7/2018 taoist]
    //if (!fProofOfStake && !TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
    //    throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    //}
	// 去掉检查 [12/10/2018 taoist]
	//if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
	//	throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
	//}
    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost)
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (nBlockSigOpsCost + packageSigOpsCost >= (uint64_t)dgpMaxBlockSigOps)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    for (const CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
        if (!fIncludeWitness && it->GetTx().HasWitness())
            return false;
    }
    return true;
}

bool BlockAssembler::AttemptToAddContractToBlock(CTxMemPool::txiter iter, uint64_t minGasPrice) {
    if (nTimeLimit != 0 && GetAdjustedTime() >= nTimeLimit - BYTECODE_TIME_BUFFER) {
        return false;
    }
    if (gArgs.GetBoolArg("-disablecontractstaking", false))
    {
        return false;
    }
    
    dev::h256 oldHashStateRoot(globalState->rootHash());
    dev::h256 oldHashUTXORoot(globalState->rootHashUTXO());
    // operate on local vars first, then later apply to `this`
    uint64_t nBlockWeight = this->nBlockWeight;
    uint64_t nBlockSigOpsCost = this->nBlockSigOpsCost;

    TscTxConverter convert(iter->GetTx(), NULL, &pblock->vtx);

    ExtractTscTX resultConverter;
    if(!convert.extractionTscTransactions(resultConverter)){
        //this check already happens when accepting txs into mempool
        //therefore, this can only be triggered by using raw transactions on the staker itself
        return false;
    }
    std::vector<TscTransaction> tscTransactions = resultConverter.first;
    dev::u256 txGas = 0;
    for(TscTransaction tscTransaction : tscTransactions){
        txGas += tscTransaction.gas();
        if(txGas > txGasLimit) {
            // Limit the tx gas limit by the soft limit if such a limit has been specified.
            return false;
        }

        if(bceResult.usedGas + tscTransaction.gas() > softBlockGasLimit){
            //if this transaction's gasLimit could cause block gas limit to be exceeded, then don't add it
            return false;
        }
        if(tscTransaction.gasPrice() < minGasPrice){
            //if this transaction's gasPrice is less than the current DGP minGasPrice don't add it
            return false;
        }
    }
    // We need to pass the DGP's block gas limit (not the soft limit) since it is consensus critical.
    ByteCodeExec exec(*pblock, tscTransactions, hardBlockGasLimit);
    if(!exec.performByteCode()){
        //error, don't add contract
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        return false;
    }

    ByteCodeExecResult testExecResult;
    if(!exec.processingResults(testExecResult)){
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        return false;
    }

    if(bceResult.usedGas + testExecResult.usedGas > softBlockGasLimit){
        //if this transaction could cause block gas limit to be exceeded, then don't add it
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        return false;
    }

    //apply contractTx costs to local state
    nBlockWeight += iter->GetTxWeight();
    nBlockSigOpsCost += iter->GetSigOpCost();
    //apply value-transfer txs to local state
    for (CTransaction &t : testExecResult.valueTransfers) {
        nBlockWeight += GetTransactionWeight(t);
        nBlockSigOpsCost += GetLegacySigOpCount(t);
    }

    int proofTx = pblock->IsProofOfStake() ? 1 : 0;

    //calculate sigops from new refund/proof tx

    //first, subtract old proof tx
    nBlockSigOpsCost -= GetLegacySigOpCount(*pblock->vtx[proofTx]);

    // manually rebuild refundtx
    CMutableTransaction contrTx(*pblock->vtx[proofTx]);
    //note, this will need changed for MPoS
    int i=contrTx.vout.size();
    contrTx.vout.resize(contrTx.vout.size()+testExecResult.refundOutputs.size());
    for(CTxOut& vout : testExecResult.refundOutputs){
        contrTx.vout[i]=vout;
        i++;
    }
    nBlockSigOpsCost += GetLegacySigOpCount(contrTx);
    //all contract costs now applied to local state

    //Check if block will be too big or too expensive with this contract execution
    if (nBlockSigOpsCost * WITNESS_SCALE_FACTOR > (uint64_t)dgpMaxBlockSigOps ||
            nBlockWeight > dgpMaxBlockWeight) {
        //contract will not be added to block, so revert state to before we tried
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
        return false;
    }

    //block is not too big, so apply the contract execution and it's results to the actual block

    //apply local bytecode to global bytecode state
    bceResult.usedGas += testExecResult.usedGas;
    bceResult.refundSender += testExecResult.refundSender;
    bceResult.refundOutputs.insert(bceResult.refundOutputs.end(), testExecResult.refundOutputs.begin(), testExecResult.refundOutputs.end());
    bceResult.valueTransfers = std::move(testExecResult.valueTransfers);

    pblock->vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    this->nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    this->nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    for (CTransaction &t : bceResult.valueTransfers) {
        pblock->vtx.emplace_back(MakeTransactionRef(std::move(t)));
        this->nBlockWeight += GetTransactionWeight(t);
        this->nBlockSigOpsCost += GetLegacySigOpCount(t);
        ++nBlockTx;
    }
    //calculate sigops from new refund/proof tx
    this->nBlockSigOpsCost -= GetLegacySigOpCount(*pblock->vtx[proofTx]);
    RebuildRefundTransaction();
    this->nBlockSigOpsCost += GetLegacySigOpCount(*pblock->vtx[proofTx]);

    bceResult.valueTransfers.clear();

    return true;
}

// 填充blocktemplate [12/7/2018 taoist]
void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblock->vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx)
{
    int nDescendantsUpdated = 0;
	// 确定多少交易是准备添加到块里面的
	LogPrint(BCLog::ADDPACKAGETX,"UpdatePackagesForAdded alreadyAdded size is : %d\n",alreadyAdded.size());
    for (const CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
		//		it若不在	descendants中那么插入，同时也插入it的孩子
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
		// 将所有后代(尚未处于块中)插入到修改后的集合中。
        for (CTxMemPool::txiter desc : descendants) {
			//alreadyAdded有就不做处理
			if (alreadyAdded.count(desc))
			{
                continue;
			}
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
				//没找到就插入
				LogPrint(BCLog::ADDPACKAGETX,"mit == mapModifiedTx.end() \n" );

                CTxMemPoolModifiedEntry modEntry(desc);
				//it->
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
				LogPrint(BCLog::ADDPACKAGETX,"mit != mapModifiedTx.end() \n");

                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}

// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
//跳过mapTx中已经在块中或mapModifiedTx中存在的条目(这意味着mapTx祖先状态由于块中包含祖先而失效)也跳过了我们已经无法添加的事务。如果我们考虑mapModifiedTx中的事务，并且它失败了，就会发生这种情况：
//然后，在遍历mapTx时，我们可能会再次考虑它。它目前保证再次失败，但作为带和吊带检查，我们把它放在FailedTx和避免重新评估，因为重新评估将使用缓存的大小/西格普斯/费用值，但实际上是不正确的。
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx)
{
    assert (it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}
// 排序快中是事务 [12/7/2018 taoist]
void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, CTxMemPool::txiter entry, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
	//如果事务A依赖于事务B，则A的祖先计数必须大于B”s。因此这足以有效地命令用于块包含的事务。
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
// 该事务选择算法基于事务的发热排序备忘录池，包括所有未确认的祖先。
// 由于我们在选择用于块包含的内存池中时不会删除事务，所以我们需要一种替代的方法
// 来更新事务的狂热，以及它尚未选定的祖先。这是通过在mapModifiedTxs中遍历所选事务的inmempool后代
// 并存储一个临时修改状态来实现的。每次通过循环，
// 我们都会比较mapModifiedTxs中最好的事务和内存池中的下一个事务，以决定下一步要处理哪个事务包。
int BlockAssembler::addPackageTxs(int &nPackagesSelected, int &nDescendantsUpdated, uint64_t minGasPrice)
{
	LogPrint(BCLog::ADDPACKAGETX,"--------------   addPackageTxs  ----------------------\n") ;

    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
	// mapModifiedTx将在修改后存储已排序的包，因为它们的一些txs已经在块中
    indexed_modified_transaction_set mapModifiedTx;

    // Keep track of entries that failed inclusion, to avoid duplicate work
	// 跟踪包含失败的条目，以避免重复工作
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
	// 首先，将以前添加的txs的所有后代添加到mapModifiedTx中，并为他们已经包含的祖先修改它们
    int blocknum=UpdatePackagesForAdded(inBlock, mapModifiedTx);
	LogPrint(BCLog::ADDPACKAGETX,"UpdatePackagesForAdded :%d\n",blocknum);



    CTxMemPool::indexed_transaction_set::index<ancestor_score_or_gas_price>::type::iterator mi = mempool.mapTx.get<ancestor_score_or_gas_price>().begin();
    CTxMemPool::txiter iter;
	//CTxMemPool::indexed_transaction_set::index<ancestor_score_or_gas_price>::type::iterator mi = mempool.mapTx.get<ancestor_score_or_gas_price>().begin();
	CTxMemPool::indexed_transaction_set::index<ancestor_score_or_gas_price>::type::iterator aaa = mempool.mapTx.get<ancestor_score_or_gas_price>().begin();
	int i = 0;
	while (aaa != mempool.mapTx.get<ancestor_score_or_gas_price>().end())
	{
		i++;
		aaa++;
	}
	LogPrint(BCLog::ADDPACKAGETX,"mempool.mapTxs size :%d\n",i );
	
	const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
	//失败次数统计
	int64_t nConsecutiveFailed = 0;
	//-----------------------------------------------------
	// 普通交易和买票交易分开打包 [2/14/2019 taoist]
	std::vector<CTxMemPoolEntry> ticket_pool;
	std::vector<CTxMemPoolEntry> commonly_pool;
	//while (mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end() || !mapModifiedTx.empty())

	int num = 0;
	int ticketNnm = 0;
	while (mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end()|| !mapModifiedTx.empty())
	{

		//测试两个条件flag1 flag2
		bool flag1 = mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end();
		bool flag2 = !mapModifiedTx.empty();
		std::string flags = flag1 ? "mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end()" : "mi == mempool.mapTx.get<ancestor_score_or_gas_price>().end();";
		std::string flags2 = flag2 ? "!mapModifiedTx.empty())" : "is mapModifiedTx.empty()";
		LogPrint(BCLog::ADDPACKAGETX,"flag1 :%s flag2 : %s\n",flags,flags2);


		LogPrint(BCLog::ADDPACKAGETX,"--------------   taoist ma  ticket----------------------\n");
		//LogPrint(BCLog::ADDPACKAGETX," ii ::" << ii << "fee is ::" << mi->GetFee() ;

		const CTransaction tx = mi->GetTx();
		LogPrint(BCLog::ADDPACKAGETX,"num is :%d  ###############\n",num);
		num++;
		
		/*	for (auto out : tx.vout )*/
		if (tx.vout[0].scriptPubKey.HasOpSStx())
		{
			LogPrint(BCLog::ADDPACKAGETX,"num is :%d $$$$$$$$$$$$\n",num);

			ticket_pool.push_back(*mi);
			LogPrint(BCLog::ADDPACKAGETX,"is ticket tx size == %d\n",ticket_pool.size());

			if (nTimeLimit != 0 && GetAdjustedTime() >= nTimeLimit) {
				//no more time to add transactions, just exit
				// 没有时间添加事务，只需退出
				LogPrint(BCLog::ADDPACKAGETX,"****(no more time to add transactions, just exit\n");

				return ticketNnm;
			}
			// First try to find a new transaction in mapTx to evaluate.
			//首先，尝试在mapTx中找到要评估的新事务。
			if (mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end() &&
				SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
				LogPrint(BCLog::ADDPACKAGETX,"SkipMapTxEntry and not is end\n");

				++mi;
				continue;
			}
			// Now that mi is not stale, determine which transaction to evaluate:
			// the next entry from mapTx, or the best from mapModifiedTx?
			// 既然MI没有过时，那么确定要计算哪个事务：mapTx的下一个条目，还是mapModifiedTx中的最佳项？
			bool fUsingModified = false;
			modtxscoreiter modit = mapModifiedTx.get<ancestor_score_or_gas_price>().begin();
			if (mi == mempool.mapTx.get<ancestor_score_or_gas_price>().end()) {
				// We're out of entries in mapTx; use the entry from mapModifiedTx 
				//  mapTx中没有条目；使用mapModifiedTx中的条目
				iter = modit->iter;
				fUsingModified = true;
				LogPrint(BCLog::ADDPACKAGETX,"mi == mempool.mapTx.get<ancestor_score_or_gas_price>().end()\n");

			}
			else {
				// Try to compare the mapTx entry to the mapModifiedTx entry
				// 尝试将mapTx条目与mapModifiedTx条目进行比较
				iter = mempool.mapTx.project<0>(mi);
				LogPrint(BCLog::ADDPACKAGETX,"mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end()\n");

				if (modit != mapModifiedTx.get<ancestor_score_or_gas_price>().end() &&
					CompareModifiedEntry()(*modit, CTxMemPoolModifiedEntry(iter))) {
					// The best entry in mapModifiedTx has higher score
					// than the one from mapTx.
					// Switch which transaction (package) to consider
					LogPrint(BCLog::ADDPACKAGETX,"The best entry in mapModifiedTx has higher score\n");

					iter = modit->iter;
					fUsingModified = true;
				}
				else {
					LogPrint(BCLog::ADDPACKAGETX,"!!!!!The best entry in mapModifiedTx has higher score\n");

					// Either no entry in mapModifiedTx, or it's worse than mapTx.
					// Increment mi for the next loop iteration.
					++mi;
				}
			}
			// We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
			// contain anything that is inBlock.
			// 我们跳过了inBlock的mapTx条目，mapModifiedTx不应该包含任何inBlock。
			if (!inBlock.count(iter))
			{
				LogPrint(BCLog::ADDPACKAGETX,"****** inBlock.count(iter)********* \n") ;

			assert(!inBlock.count(iter));

			}
			LogPrint(BCLog::ADDPACKAGETX,"****** tickeet tx************* \n");
			uint64_t packageSize = iter->GetSizeWithAncestors();
			LogPrint(BCLog::ADDPACKAGETX,"packageSize : %I64u\n",packageSize);
			CAmount packageFees = iter->GetModFeesWithAncestors();
			LogPrint(BCLog::ADDPACKAGETX,"packageFees  : %s\n",packageFees);
			int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
			LogPrint(BCLog::ADDPACKAGETX,"packageSigOpsCost  : %d\n",packageSigOpsCost);
			LogPrint(BCLog::ADDPACKAGETX,"******** tickeet tx***********\n");

			if (fUsingModified) {
				LogPrint(BCLog::ADDPACKAGETX,"Yes fUsingModified \n");

				packageSize = modit->nSizeWithAncestors;
				packageFees = modit->nModFeesWithAncestors;
				packageSigOpsCost = modit->nSigOpCostWithAncestors;
			}
			
			if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
				// Everything else we might consider has a lower fee rate
				// 我们可能考虑的其他任何事情都有一个较低的收费率。
				LogPrint(BCLog::ADDPACKAGETX,"****( Everything else we might consider has a lower fee rate\n");
			
				return ticketNnm;
			}
			
			if (!TestPackage(packageSize, packageSigOpsCost)) 
			{
				LogPrint(BCLog::ADDPACKAGETX,"not TestPackage\n");

				if (fUsingModified) 
				{
					LogPrint(BCLog::ADDPACKAGETX,"not fUsingModified\n ");

					// Since we always look at the best entry in mapModifiedTx,
					// we must erase failed entries so that we can consider the
					// next best entry on the next loop iteration
					// 因为我们总是查看mapModifiedTx中的最佳条目，所以我们必须删除失败的条目，以便在下一个循环迭代中考虑下一个最佳条目。
					mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
					failedTx.insert(iter);
				}
			
				++nConsecutiveFailed;
			
				//判断失败次数 和最大块大小的
				if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
					nBlockMaxWeight - 4000) 
				{
					LogPrint(BCLog::ADDPACKAGETX,"not fUsingModified\n");

					// Give up if we're close to full and haven't succeeded in a while
					//如果我们已经接近满足点，并且有一段时间没有成功的话，那就放弃吧
					break;
				}
				continue;
			}
			LogPrint(BCLog::ADDPACKAGETX,"Yes TestPackage\n");

			
			CTxMemPool::setEntries ancestors;
			uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
			std::string dummy;
			mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);
			
			onlyUnconfirmed(ancestors);
			ancestors.insert(iter);
			
			// Test if all tx's are Final
			// 测试是否所有的TX都是最终的
			if (!TestPackageTransactions(ancestors)) {
				if (fUsingModified) {
					mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
					failedTx.insert(iter);
				}
				continue;
			}
			LogPrint(BCLog::ADDPACKAGETX,"Yes TestPackageTransactions\n");

			
			// This transaction will make it in; reset the failed counter.
			// 此事务将使其进入；重置失败的计数器。
			nConsecutiveFailed = 0;
			
			// Package can be added. Sort the entries in a valid order.
			// 包可以添加。按有效顺序对条目进行排序。
			std::vector<CTxMemPool::txiter> sortedTicketEntries;
			SortForBlock(ancestors, iter, sortedTicketEntries);
			bool wasAdded = true; // TSC_INSERT_LINE
			LogPrint(BCLog::ADDPACKAGETX,"SortForBlock size is  : %d\n",sortedTicketEntries.size());

			//real add tx  to block
			for (size_t i = 0; i < sortedTicketEntries.size(); ++i)
			{
				if (!wasAdded || (nTimeLimit != 0 && GetAdjustedTime() >= nTimeLimit))
				{
					//if out of time, or earlier ancestor failed, then skip the rest of the transactions
					mapModifiedTx.erase(sortedTicketEntries[i]);
					wasAdded = false;
					continue;
				}
				const CTransaction& tx = sortedTicketEntries[i]->GetTx();
				if (wasAdded)
				{
					if (tx.HasCreateOrCall())
					{
						wasAdded = AttemptToAddContractToBlock(sortedTicketEntries[i], minGasPrice);
						if (!wasAdded)
						{
							if (fUsingModified)
							{
								//this only needs to be done once to mark the whole package (everything in sortedEntries) as failed
								mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
								failedTx.insert(iter);
							}
						}
					}
					else
					{
						if (ticketNnm>=20)
						{
							continue;
						}
						LogPrint(BCLog::ADDPACKAGETX," ticket tx real add\n");
						AddToBlock(sortedTicketEntries[i]);
						ticketNnm++;
						LogPrint(BCLog::ADDPACKAGETX," inBlock.size()=%d\n",inBlock.size());
						LogPrint(BCLog::ADDPACKAGETX," *************************************************************\n");

						
					}
				}
				// Erase from the modified set, if present
				// 如果存在，则从修改后的集合中擦除。
				mapModifiedTx.erase(sortedTicketEntries[i]);
			}
			if (!wasAdded) {
				//skip UpdatePackages if a transaction failed to be added (match TestPackage logic)
				continue;
			}
			
			++nPackagesSelected;
			
			// Update transactions that depend on each of these
			// 更新依赖于以下每一项的事务

			//nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);


		}
		else
		{
			mi++;
			continue;
		}

	}
	//std::cout << "--------------   taoist ma  ----------------------" ;
	mi = mempool.mapTx.get<ancestor_score_or_gas_price>().begin();
	while (mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end() || !mapModifiedTx.empty())
	{
		bool flag1 = mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end();
		bool flag2 = !mapModifiedTx.empty();
		std::string flags = flag1 ? "mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end()" : "mi == mempool.mapTx.get<ancestor_score_or_gas_price>().end();";
		std::string flags2 = flag2 ? "!mapModifiedTx.empty())" : "is mapModifiedTx.empty()";
		LogPrint(BCLog::ADDPACKAGETX,"flag1 : %s flag2 : %s\n",flags,flags2);


		LogPrint(BCLog::ADDPACKAGETX,"--------------   taoist ma  average ----------------------\n");
		//LogPrint(BCLog::ADDPACKAGETX," ii ::" << ii << "fee is ::" << mi->GetFee() ;

		const CTransaction tx = mi->GetTx();


		if (tx.vout[0].scriptPubKey.HasOpSStx())
		{
			mi++;
			continue;
		}
		else
		{
			//commonly_pool.mapTx
			LogPrint(BCLog::ADDPACKAGETX,"is commonly_pool tx");
			commonly_pool.push_back(*mi);
			LogPrint(BCLog::ADDPACKAGETX,"is commonly_pool tx size == %d",commonly_pool.size());

			if (nTimeLimit != 0 && GetAdjustedTime() >= nTimeLimit) {
				//no more time to add transactions, just exit
				// 没有时间添加事务，只需退出
				LogPrint(BCLog::ADDPACKAGETX,"****(no more time to add transactions, just exit 222\n");

				return ticketNnm;
			}
			// First try to find a new transaction in mapTx to evaluate.
			//首先，尝试在mapTx中找到要评估的新事务。
			if (mi != mempool.mapTx.get<ancestor_score_or_gas_price>().end() &&
				SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {

				++mi;
				continue;
			}

			// Now that mi is not stale, determine which transaction to evaluate:
			// the next entry from mapTx, or the best from mapModifiedTx?
			// 既然MI没有过时，那么确定要计算哪个事务：mapTx的下一个条目，还是mapModifiedTx中的最佳项？
			bool fUsingModified = false;
			modtxscoreiter modit = mapModifiedTx.get<ancestor_score_or_gas_price>().begin();
			if (mi == mempool.mapTx.get<ancestor_score_or_gas_price>().end()) {
				// We're out of entries in mapTx; use the entry from mapModifiedTx 
				//  mapTx中没有条目；使用mapModifiedTx中的条目
				iter = modit->iter;
				fUsingModified = true;
			}
			else {
				// Try to compare the mapTx entry to the mapModifiedTx entry
				// 尝试将mapTx条目与mapModifiedTx条目进行比较
				iter = mempool.mapTx.project<0>(mi);
				if (modit != mapModifiedTx.get<ancestor_score_or_gas_price>().end() &&
					CompareModifiedEntry()(*modit, CTxMemPoolModifiedEntry(iter))) {
					// The best entry in mapModifiedTx has higher score
					// than the one from mapTx.
					// Switch which transaction (package) to consider
					iter = modit->iter;
					fUsingModified = true;
				}
				else {
					// Either no entry in mapModifiedTx, or it's worse than mapTx.
					// Increment mi for the next loop iteration.
					++mi;
				}
			}
			// We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
			// contain anything that is inBlock.
			// 我们跳过了inBlock的mapTx条目，mapModifiedTx不应该包含任何inBlock。

			if (!inBlock.count(iter))
			{
				LogPrint(BCLog::ADDPACKAGETX,"****** inBlock.count(iter)********* \n");

				assert(!inBlock.count(iter));

			}

			LogPrint(BCLog::ADDPACKAGETX,"****** commonly_pool tx************* \n");
			uint64_t packageSize = iter->GetSizeWithAncestors();
			LogPrint(BCLog::ADDPACKAGETX,"packageSize : %d\n",packageSize);
			CAmount packageFees = iter->GetModFeesWithAncestors();
			LogPrint(BCLog::ADDPACKAGETX,"packageFees  : %s\n",packageFees);
			int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
			LogPrint(BCLog::ADDPACKAGETX,"packageSigOpsCost  : %d/n", packageSigOpsCost);
			LogPrint(BCLog::ADDPACKAGETX,"******** commonly_pool tx*********** \n");

			if (fUsingModified) {
				packageSize = modit->nSizeWithAncestors;
				packageFees = modit->nModFeesWithAncestors;
				packageSigOpsCost = modit->nSigOpCostWithAncestors;
			}

			if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
				// Everything else we might consider has a lower fee rate
				// 我们可能考虑的其他任何事情都有一个较低的收费率。
				LogPrint(BCLog::ADDPACKAGETX,"****( Everything else we might consider has a lower fee rate2222\n");

				return ticketNnm;
			}

			if (!TestPackage(packageSize, packageSigOpsCost)) {
				if (fUsingModified) {
					// Since we always look at the best entry in mapModifiedTx,
					// we must erase failed entries so that we can consider the
					// next best entry on the next loop iteration
					// 因为我们总是查看mapModifiedTx中的最佳条目，所以我们必须删除失败的条目，以便在下一个循环迭代中考虑下一个最佳条目。
					mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
					failedTx.insert(iter);
				}

				++nConsecutiveFailed;

				if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
					nBlockMaxWeight - 4000) {
					// Give up if we're close to full and haven't succeeded in a while
					//如果我们已经接近满足点，并且有一段时间没有成功的话，那就放弃吧
					break;
				}
				continue;
			}

			CTxMemPool::setEntries ancestors;
			uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
			std::string dummy;
			mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

			onlyUnconfirmed(ancestors);
			ancestors.insert(iter);

			// Test if all tx's are Final
			// 测试是否所有的TX都是最终的
			if (!TestPackageTransactions(ancestors)) {
				if (fUsingModified) {
					mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
					failedTx.insert(iter);
				}
				continue;
			}

			// This transaction will make it in; reset the failed counter.
			// 此事务将使其进入；重置失败的计数器。
			nConsecutiveFailed = 0;

			// Package can be added. Sort the entries in a valid order.
			// 包可以添加。按有效顺序对条目进行排序。
			std::vector<CTxMemPool::txiter> sortedCommonlyEntries;
			SortForBlock(ancestors, iter, sortedCommonlyEntries);
			bool wasAdded = true; // TSC_INSERT_LINE

								  //real add tx  to block
			for (size_t i = 0; i < sortedCommonlyEntries.size(); ++i)
			{
				if (!wasAdded || (nTimeLimit != 0 && GetAdjustedTime() >= nTimeLimit))
				{
					//if out of time, or earlier ancestor failed, then skip the rest of the transactions
					mapModifiedTx.erase(sortedCommonlyEntries[i]);
					wasAdded = false;
					continue;
				}
				const CTransaction& tx = sortedCommonlyEntries[i]->GetTx();
				if (wasAdded)
				{
					if (tx.HasCreateOrCall())
					{
						wasAdded = AttemptToAddContractToBlock(sortedCommonlyEntries[i], minGasPrice);
						if (!wasAdded)
						{
							if (fUsingModified)
							{
								//this only needs to be done once to mark the whole package (everything in sortedEntries) as failed
								mapModifiedTx.get<ancestor_score_or_gas_price>().erase(modit);
								failedTx.insert(iter);
							}
						}
					}
					else
					{
						LogPrint(BCLog::ADDPACKAGETX,"Commonly real add\n");
						AddToBlock(sortedCommonlyEntries[i]);
					}
				}
				// Erase from the modified set, if present
				// 如果存在，则从修改后的集合中擦除。
				mapModifiedTx.erase(sortedCommonlyEntries[i]);
			}
			//if (!wasAdded) {
			//	//skip UpdatePackages if a transaction failed to be added (match TestPackage logic)
			//	continue;
			//}

			++nPackagesSelected;

			// Update transactions that depend on each of these
			// 更新依赖于以下每一项的事务
			LogPrint(BCLog::ADDPACKAGETX,"ancestors=%d\n",ancestors.size());
			//nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);

		}

	}
	return ticketNnm;
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}


// 去除stake miner [12/7/2018 taoist]
//////////////////////////////////////////////////////////////////////////////
//
// Proof of Stake miner
//

//
// Looking for suitable coins for creating new block.
//

//bool CheckStake(const std::shared_ptr<const CBlock> pblock, CWallet& wallet)
//{
//    uint256 proofHash, hashTarget;
//    uint256 hashBlock = pblock->GetHash();
//
//    if(!pblock->IsProofOfStake())
//        return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex());
//
//    // verify hash target and signature of coinstake tx
//    CValidationState state;
//    if (!CheckProofOfStake(mapBlockIndex[pblock->hashPrevBlock], state, *pblock->vtx[1], pblock->nBits, pblock->nTime, proofHash, hashTarget, *pcoinsTip))
//        return error("CheckStake() : proof-of-stake checking failed");
//
//    //// debug print
//    LogPrint(BCLog::COINSTAKE, "CheckStake() : new proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex(), proofHash.GetHex(), hashTarget.GetHex());
//    LogPrint(BCLog::COINSTAKE, "%s\n", pblock->ToString());
//    LogPrint(BCLog::COINSTAKE, "out %s\n", FormatMoney(pblock->vtx[1]->GetValueOut()));
//
//    // Found a solution
//    {
//        LOCK(cs_main);
//        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
//            return error("CheckStake() : generated block is stale");
//
//        for(const CTxIn& vin : pblock->vtx[1]->vin) {
//            if (wallet.IsSpent(vin.prevout.hash, vin.prevout.n)) {
//                return error("CheckStake() : generated block became invalid due to stake UTXO being spent");
//            }
//        }
//
//        // Process this block the same as if we had received it from another node
//        bool fNewBlock = false;
//        if (!ProcessNewBlock(Params(), pblock, true, &fNewBlock))
//            return error("CheckStake() : ProcessBlock, block not accepted");
//    }
//
//    return true;
//}
//
//void ThreadStakeMiner(CWallet *pwallet)
//{
//    SetThreadPriority(THREAD_PRIORITY_LOWEST);
//
//    // Make this thread recognisable as the mining thread
//    RenameThread("tsccoin-miner");
//
//    CReserveKey reservekey(pwallet);
//
//    bool fTryToSync = true;
//    bool regtestMode = Params().GetConsensus().fPoSNoRetargeting;
//    if(regtestMode){
//        nMinerSleep = 30000; //limit regtest to 30s, otherwise it'll create 2 blocks per second
//    }
//
//    while (true)
//    {
//        while (pwallet->IsLocked())
//        {
//            nLastCoinStakeSearchInterval = 0;
//            MilliSleep(10000);
//        }
//        //don't disable PoS mining for no connections if in regtest mode
//        if(!regtestMode && !gArgs.GetBoolArg("-emergencystaking", false)) {
//            while (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0 || IsInitialBlockDownload()) {
//                nLastCoinStakeSearchInterval = 0;
//                fTryToSync = true;
//                MilliSleep(1000);
//            }
//            if (fTryToSync) {
//                fTryToSync = false;
//                if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) < 3 ||
//                    pindexBestHeader->GetBlockTime() < GetTime() - 10 * 60) {
//                    MilliSleep(60000);
//                    continue;
//                }
//            }
//        }
//        //
//        // Create new block
//        //
//        if(pwallet->HaveAvailableCoinsForStaking())
//        {
//            int64_t nTotalFees = 0;
//            // First just create an empty block. No need to process transactions until we know we can create a block
//            std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateEmptyBlock(reservekey.reserveScript, true, true, &nTotalFees));
//            if (!pblocktemplate.get())
//                return;
//            CBlockIndex* pindexPrev =  chainActive.Tip();
//
//            uint32_t beginningTime=GetAdjustedTime();
//            beginningTime &= ~STAKE_TIMESTAMP_MASK;
//            for(uint32_t i=beginningTime;i<beginningTime + MAX_STAKE_LOOKAHEAD;i+=STAKE_TIMESTAMP_MASK+1) {
//
//                // The information is needed for status bar to determine if the staker is trying to create block and when it will be created approximately,
//                static int64_t nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp
//                // nLastCoinStakeSearchInterval > 0 mean that the staker is running
//                nLastCoinStakeSearchInterval = i - nLastCoinStakeSearchTime;
//
//                // Try to sign a block (this also checks for a PoS stake)
//                pblocktemplate->block.nTime = i;
//                std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>(pblocktemplate->block);
//                if (SignBlock(pblock, *pwallet, nTotalFees, i)) {
//                    // increase priority so we can build the full PoS block ASAP to ensure the timestamp doesn't expire
//                    SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);
//
//                    if (chainActive.Tip()->GetBlockHash() != pblock->hashPrevBlock) {
//                        //another block was received while building ours, scrap progress
//                        LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid");
//                        break;
//                    }
//                    // Create a block that's properly populated with transactions
//                    std::unique_ptr<CBlockTemplate> pblocktemplatefilled(
//                            BlockAssembler(Params()).CreateNewBlock(pblock->vtx[1]->vout[1].scriptPubKey, true, true, &nTotalFees,
//                                                                    i, FutureDrift(GetAdjustedTime()) - STAKE_TIME_BUFFER));
//                    if (!pblocktemplatefilled.get())
//                        return;
//                    if (chainActive.Tip()->GetBlockHash() != pblock->hashPrevBlock) {
//                        //another block was received while building ours, scrap progress
//                        LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid");
//                        break;
//                    }
//                    // Sign the full block and use the timestamp from earlier for a valid stake
//                    std::shared_ptr<CBlock> pblockfilled = std::make_shared<CBlock>(pblocktemplatefilled->block);
//                    if (SignBlock(pblockfilled, *pwallet, nTotalFees, i)) {
//                        // Should always reach here unless we spent too much time processing transactions and the timestamp is now invalid
//                        // CheckStake also does CheckBlock and AcceptBlock to propogate it to the network
//                        bool validBlock = false;
//                        while(!validBlock) {
//                            if (chainActive.Tip()->GetBlockHash() != pblockfilled->hashPrevBlock) {
//                                //another block was received while building ours, scrap progress
//                                LogPrintf("ThreadStakeMiner(): Valid future PoS block was orphaned before becoming valid");
//                                break;
//                            }
//                            //check timestamps
//                            if (pblockfilled->GetBlockTime() <= pindexPrev->GetBlockTime() ||
//                                FutureDrift(pblockfilled->GetBlockTime()) < pindexPrev->GetBlockTime()) {
//                                LogPrintf("ThreadStakeMiner(): Valid PoS block took too long to create and has expired");
//                                break; //timestamp too late, so ignore
//                            }
//                            if (pblockfilled->GetBlockTime() > FutureDrift(GetAdjustedTime())) {
//                                if (gArgs.IsArgSet("-aggressive-staking")) {
//                                    //if being agressive, then check more often to publish immediately when valid. This might allow you to find more blocks, 
//                                    //but also increases the chance of broadcasting invalid blocks and getting DoS banned by nodes,
//                                    //or receiving more stale/orphan blocks than normal. Use at your own risk.
//                                    MilliSleep(100);
//                                }else{
//                                    //too early, so wait 3 seconds and try again
//                                    MilliSleep(3000);
//                                }
//                                continue;
//                            }
//                            validBlock=true;
//                        }
//                        if(validBlock) {
//                            CheckStake(pblockfilled, *pwallet);
//                            // Update the search time when new valid block is created, needed for status bar icon
//                            nLastCoinStakeSearchTime = pblockfilled->GetBlockTime();
//                        }
//                        break;
//                    }
//                    //return back to low priority
//                    SetThreadPriority(THREAD_PRIORITY_LOWEST);
//                }
//            }
//        }
//        MilliSleep(nMinerSleep);
//    }
//}
//
//void StakeTscs(bool fStake, CWallet *pwallet)
//{
//    static boost::thread_group* stakeThread = NULL;
//
//    if (stakeThread != NULL)
//    {
//        stakeThread->interrupt_all();
//        delete stakeThread;
//        stakeThread = NULL;
//    }
//
//    if(fStake)
//    {
//        stakeThread = new boost::thread_group();
//        stakeThread->create_thread(boost::bind(&ThreadStakeMiner, pwallet));
//    }
//}
