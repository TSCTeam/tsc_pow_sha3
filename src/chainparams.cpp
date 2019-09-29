// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"
#include "consensus/consensus.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>
#include <cstdio>
#include "chainparamsseeds.h"

///////////////////////////////////////////// // tsc
#include <libdevcore/SHA3.h>
#include <libdevcore/RLP.h>
#include "arith_uint256.h"
//#include "uint128_t.h"
/////////////////////////////////////////////

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint64_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 00 << 488804799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashStateRoot = uint256(h256Touint(dev::h256("e965ffd002cd6ad0e2dc402b8044de833e06b23127ea8c3d80aec91410771495"))); // tsc
    genesis.hashUTXORoot = uint256(h256Touint(dev::sha3(dev::rlp("")))); // tsc
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint64_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Time is Money ，Space is Life ，Time-Space is Future";
    const CScript genesisOutputScript = CScript() << ParseHex("040d61d8653448c98731ee5fffd303c15e71ec2057b77f11ab3601979728cdaff2d68afbba14e4fa0bc44f2072b0b23ef63717f8cdfbe58dcd33f32b6afe98741a") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 985500; // tsc halving every 4 years
        consensus.BIP34Height = 0;
    //  consensus.BIP34Hash = uint256S("0x0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063");
    //  TSC-POW-SHA3 @ 2019
        consensus.BIP34Hash = uint256S("0x000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc");
        consensus.BIP65Height = 0; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 0; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
		// change time  [12/11/2018 taoist]
        consensus.nPowTargetTimespan =2 * 5 * 2016 ; // 2016个块调整一次难度
        consensus.nPowTargetSpacing = 2 * 5;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
       // consensus.fPoSNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000000000000c5"); // tsc

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0xbfbbfc2c3be3d4e085082aff2e4e73a4e21dbf6205bc41b84b38ffac0a8bc114"); //453354

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf4;
        pchMessageStart[1] = 0x6d;
        pchMessageStart[2] = 0xa9;
        pchMessageStart[3] = 0xd5;
        nDefaultPort = 9997;
        nPruneAfterHeight = 100000;

/*		int test = 0;

		while (1)
		{
			genesis = CreateGenesisBlock(1542793196, test, 0x1f00ffff, 1, 50 * COIN);
			int i = genesis.GetHash().ToString().compare("0000ffff00000000000000000000000000000000000000000000000000000000");
			if (i < 0)
			{
				genesis = CreateGenesisBlock(1542793196, test, 0x1f00ffff, 1, 50 * COIN);
				//LogPrintf("\n\n\n\nthis is genesis block hash  %s\nnouce is   %d\nmerkleroot is   %s\n\n\n", genesis.GetHash().ToString(), test, genesis.hashMerkleRoot.ToString());
				printf("\n\n\n\nthis is genesis block hash  %s\nnouce is   %d\nmerkleroot is   %s\n\n\n", genesis.GetHash().ToString().c_str(), test, genesis.hashMerkleRoot.ToString().c_str());
				consensus.hashGenesisBlock = genesis.GetHash();
				break;
			}

			test++;
		}
*/
		//genesis = CreateGenesisBlock(1542793196, 53706, 0x1f00ffff, 1, 50 * COIN);
		// consensus.hashGenesisBlock = genesis.GetHash();
		//std::cout << genesis.GetHash().ToString() << std::endl;
		//std::cout << genesis.hashMerkleRoot.ToString() << std::endl;
		//assert(consensus.hashGenesisBlock == uint256S("0x0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063"));
		//assert(genesis.hashMerkleRoot == uint256S("0xa498b45f66925eb3f73d05c98177f5189d5eaa33fc5b516affa464943b0d5a34"));

   
    // TSC-POW-SHA3's Genesis 
     	genesis = CreateGenesisBlock(1542793196, 17707, 0x1f00ffff, 1, 50 * COIN);
	    consensus.hashGenesisBlock = genesis.GetHash();
		assert(consensus.hashGenesisBlock == uint256S("0x000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc"));
		assert(genesis.hashMerkleRoot == uint256S("0x9f8166a8e06a59a86b840ae21915439b234f8081b1b9b5a072d7dccbb4034d6c"));
   
   
   
   
        // Note that of those with the service bits flag, most only support a subset of possible options
        //vSeeds.emplace_back("120.77.220.44", false); // Tsc mainnet
        //vSeeds.emplace_back("47.96.231.194", false);
        //vSeeds.emplace_back("39.108.105.89", false); // Tsc mainnet
        //vSeeds.emplace_back("47.96.239.164", false); // Tsc mainnet
		//vSeeds.emplace_back("39.106.167.184",false);
		//vSeeds.emplace_back("47.75.116.91",false); 
		//vSeeds.emplace_back("47.94.173.202",false); 
		//vSeeds.emplace_back("39.106.107.103",false); 
		//vSeeds.emplace_back("106.14.162.188",false); 
		//vSeeds.emplace_back("101.132.104.229",false); 
		//vSeeds.emplace_back("207.148.114.107",false); 
		//vSeeds.emplace_back("47.96.2.131",false); 
		//vSeeds.emplace_back("47.96.191.178",false);  // Tsc mainnet

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,50);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            {
             //  { 0, uint256S("0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063")},
             // TSC-POW-SHA3 @ 2019
                {0, uint256S("000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc")},
            }
        };

        chainTxData = ChainTxData{
            // Data as of block a1bab8db27f26952ce94fff6563931943554e36fc3a23f99cc8513270d685b2c (height 92662)
            1533274512, // * UNIX timestamp of last known number of transactions
            904678, // * total number of transactions between genesis and that timestamp
                            //   (the tx=... number in the SetBestChain debug.log lines)
            0.125 // * estimated number of transactions per second after that timestamp
        };
        consensus.nLastPOWBlock = 5000;
        consensus.nMPoSRewardRecipients = 10;
        consensus.nFirstMPoSBlock = consensus.nLastPOWBlock + 
                                    consensus.nMPoSRewardRecipients + 
                                    COINBASE_MATURITY;

        consensus.nFixUTXOCacheHFHeight=100000;
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 985500; // tsc halving every 4 years
        consensus.BIP34Height = 0;
    //  consensus.BIP34Hash = uint256S("0x0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063");
    //  TSC-POW-SHA3 @ 2019
        consensus.BIP34Hash = uint256S("0x000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc");
        consensus.BIP65Height = 0; // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
        consensus.BIP66Height = 0; // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 16 * 60; // 16 minutes
        consensus.nPowTargetSpacing = 2 * 64;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = true;
        consensus.fPoSNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000000000000c5"); // tsc
        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x39ffa0c5924550db0e75030ff8513c3145d491dff2e17b8e3ea1cea7b4662ff0"); //1079274

        pchMessageStart[0] = 0x4d;
        pchMessageStart[1] = 0x72;
        pchMessageStart[2] = 0x35;
        pchMessageStart[3] = 0x96;
        nDefaultPort = 19876;
        nPruneAfterHeight = 1000;

        /*genesis = CreateGenesisBlock(1533274512, 89273, 0x1f00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00005642a76faafbc16707162d1abd188b9fa6c12e64141f724ccc7093012028"));
        assert(genesis.hashMerkleRoot == uint256S("0xa498b45f66925eb3f73d05c98177f5189d5eaa33fc5b516affa464943b0d5a34"));*/
		    /*genesis = CreateGenesisBlock(1542793196, 53706, 0x1f00ffff, 1, 50 * COIN);
		    consensus.hashGenesisBlock = genesis.GetHash();
		    assert(consensus.hashGenesisBlock == uint256S("0x0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063"));
        assert(genesis.hashMerkleRoot == uint256S("0xa498b45f66925eb3f73d05c98177f5189d5eaa33fc5b516affa464943b0d5a34"));*/
        // TSC-POW-SHA3's Genesis 
       	genesis = CreateGenesisBlock(1542793196, 17707, 0x1f00ffff, 1, 50 * COIN);
	      consensus.hashGenesisBlock = genesis.GetHash();
		    assert(consensus.hashGenesisBlock == uint256S("0x000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc"));
	    	assert(genesis.hashMerkleRoot == uint256S("0x9f8166a8e06a59a86b840ae21915439b234f8081b1b9b5a072d7dccbb4034d6c"));
   

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("tsc4.dynu.net", false); // Tsc testnet

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,120);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,110);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = (CCheckpointData) {
            {
               // {0, uint256S("0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063")},
               // TSC-POW-SHA3 @ 2019
                {0, uint256S("000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc")},
            }
        };

        chainTxData = ChainTxData{
            // Data as of block 493cccf2ba87ffdabd7afc0f3242c1357fdebdc0b8c7e7adc3c6dc2b1c8ca797 (height 79167)
			1533274512, // * UNIX timestamp of last known number of transactions
			904678, // * total number of transactions between genesis and that timestamp
					//   (the tx=... number in the SetBestChain debug.log lines)
			0.125 // * estimated number of transactions per second after that timestamp
        };

        consensus.nLastPOWBlock = 5000;
        consensus.nMPoSRewardRecipients = 10;
        consensus.nFirstMPoSBlock = consensus.nLastPOWBlock + 
                                    consensus.nMPoSRewardRecipients + 
                                    COINBASE_MATURITY;

        consensus.nFixUTXOCacheHFHeight=84500;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP34Height = 0; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests) // activate for tsc
//        consensus.BIP34Hash = uint256S("0x0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063");
        consensus.BIP34Hash = uint256S("0x000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc");
        consensus.BIP65Height = 0; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 0; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 16 * 60; // 16 minutes
        consensus.nPowTargetSpacing = 2 * 64;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.fPoSNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0x4d;
        pchMessageStart[1] = 0x2d;
        pchMessageStart[2] = 0x6c;
        pchMessageStart[3] = 0x17;
        nDefaultPort = 29876;
        nPruneAfterHeight = 1000;

 //       genesis = CreateGenesisBlock(1533274512 ,65534, 0x207fffff, 1, 50 * COIN);   
	//consensus.hashGenesisBlock = genesis.GetHash();
 //       assert(consensus.hashGenesisBlock == uint256S("0xa498b45f66925eb3f73d05c98177f5189d5eaa33fc5b516affa464943b0d5a34"));
 //       assert(genesis.hashMerkleRoot == uint256S("0xa498b45f66925eb3f73d05c98177f5189d5eaa33fc5b516affa464943b0d5a34"));
//		genesis = CreateGenesisBlock(1542793196, 53706, 0x1f00ffff, 1, 50 * COIN);
//		consensus.hashGenesisBlock = genesis.GetHash();
//		assert(consensus.hashGenesisBlock == uint256S("0x0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063"));
//		assert(genesis.hashMerkleRoot == uint256S("0xa498b45f66925eb3f73d05c98177f5189d5eaa33fc5b516affa464943b0d5a34"));
      // TSC-POW-SHA3's Genesis 
       	genesis = CreateGenesisBlock(1542793196, 17707, 0x1f00ffff, 1, 50 * COIN);
	      consensus.hashGenesisBlock = genesis.GetHash();
		    assert(consensus.hashGenesisBlock == uint256S("0x000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc"));
	    	assert(genesis.hashMerkleRoot == uint256S("0x9f8166a8e06a59a86b840ae21915439b234f8081b1b9b5a072d7dccbb4034d6c"));
   
        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData) {
            {
               // {0, uint256S("0000bc98ff7e84479870baf35c024f7822f55535fdf79bd37d04a84f4ba63063")},
               // TSC-POW-SHA3 @ 2019
                {0, uint256S("000018234c5e94f9df19e2f85b6354c0b9eff3d4b4a4a45418bc9712999d1dfc")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
        consensus.nLastPOWBlock = 0x7fffffff;
        consensus.nMPoSRewardRecipients = 10;
        consensus.nFirstMPoSBlock = 5000;

        consensus.nFixUTXOCacheHFHeight=0;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,120);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,110);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
    }
};

/**
 * Regression network parameters overwrites for unit testing
 */
class CUnitTestParams : public CRegTestParams
{
public:
    CUnitTestParams()
    {
        // Activate the the BIPs for regtest as in Bitcoin
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)

        // TSC have 500 blocks of maturity, increased values for regtest in unit tests in order to correspond with it
        consensus.nSubsidyHalvingInterval = 750;
        consensus.nRuleChangeActivationThreshold = 558; // 75% for testchains
        consensus.nMinerConfirmationWindow = 744; // Faster than normal for regtest (744 instead of 2016)
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    else if (chain == CBaseChainParams::UNITTEST)
        return std::unique_ptr<CChainParams>(new CUnitTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
