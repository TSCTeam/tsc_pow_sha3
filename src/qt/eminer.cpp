#include "eminer.h"

#include "../primitives/block.h"
#include "erpcbase.h"
#include "../core_io.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include "../pow.h"
#include "../chainparams.h"
#include <iostream>
#include "../arith_uint256.h"
#include "../utilstrencodings.h"
#include "../streams.h"

#include <QThread>
#include <QThreadPool>

#include <QMetaType>
#include <QList>

extern QString gloabl_USER;
extern QString gloabl_PASSWD;
extern QString gloabl_URL;
extern QString gloabl_Params;
extern int cpu_core ;
extern bool is_submode ;
extern bool is_buyticket  ;
extern int ticketsnum ;
bool is_stop =false;

QList<ERpcbase*> buylist;

bool is_newblock_found = false;

EMiner::EMiner(QObject *parent) : QObject(parent)
{
	    qRegisterMetaType<uint64_t>("uint64_t");

        timer = new QTimer();
        connect(timer, SIGNAL(timeout()), this, SLOT(updatetime()));
        timer->start(500);
}
void EMiner::updatetime()
{
    if(is_stop)
    {
		is_stop = false;
		getwork();

	}
	

}

subminer::subminer(CBlock blk,int index, int cores_count)
{
    subindex = index;
    sublength = 0xffffffffffffffff/cores_count;
    nblk = blk;


}
void subminer::run()
{
     qDebug()<<"currentThreadId  "<<QThread::currentThreadId()<<"     start mining";
     nblk.nNonce = subindex * sublength;
     while (nblk.nNonce < (subindex + 1)  * sublength && !subCheckProofOfWork(nblk.GetHash(), nblk.nBits) && !is_newblock_found)
     {
         ++nblk.nNonce;
     }
     if (nblk.nNonce == (subindex + 1)  * sublength)
     {
         qDebug()<<"can't find this time"<<"  from ThreadId "<< QThread::currentThreadId();
         return ;
     }
     else if(is_newblock_found == true)
     {
         //qDebug()<<"other partner had found the result! stop the work!"<< QThread::currentThreadId();
         return ;
     }
     else
     {
         qDebug()<<"i am miner "<<subindex<<" i have found the result! stop the work!"<< QThread::currentThreadId();
         qDebug()<<"the block is  "<< QString::fromStdString( nblk.GetHash().ToString());
         Q_EMIT calsuccess(nblk.nNonce);
         return ;
     }

}

void EMiner::subcaler()
{
     Q_EMIT calsuccess();
}
subminer::~subminer()
{
}

int EMiner::getwork()
{
    if(is_buyticket)
    {
		for(int i = ticketsnum; i > 0; i--)
		{
			this->buyticket();
		}			
	}
    is_newblock_found = false;
    rpcreq = new ERpcbase();
    rpcreq->setMethod("getwork");
    rpcreq->setRpcUser(gloabl_USER);
    rpcreq->setRpcPasswd(gloabl_PASSWD);
    rpcreq->setUrl(gloabl_URL);
    //rpcreq->setUrl("http://191.167.2.1:4566");
    //rpcsub->setUrl("http://191.167.2.36:23412");
    rpcreq->sendPost();
    connect(rpcreq, SIGNAL(finished()), this, SLOT(calheader()));
    return 0;

}


void EMiner::calheader()
{
    static const uint32_t nInnerLoopCount = 0xffffffff;
    qDebug()<<rpcreq->RpcResult;
    QString Block;

    //解析json字符串获取hexstr和height
    QByteArray test = rpcreq->RpcResult;
    QJsonParseError json_error;
    QJsonDocument parse_document = QJsonDocument::fromJson(test, &json_error);
	bool is_ready = false;
    if(json_error.error == QJsonParseError::NoError)
    {
        if(parse_document.isObject())
        {
            QJsonObject obj = parse_document.object();
            if(obj.contains("result"))
            {
                QJsonValue result_value = obj.take("result");
                if(result_value.isObject())
                {
                    QJsonObject result = result_value.toObject();

					 if(result.contains("miner_ready"))
                    {
                        QJsonValue miner_ready = result.take("miner_ready");
                        is_ready = miner_ready.toBool();
                    }
					if(is_ready)
					{
						if(result.contains("strhex"))
						{
							QJsonValue strhex = result.take("strhex");
							Block = strhex.toString();
						}
						else
						{
							qDebug()<<"json_parse_strhex_error";
							 rpcreq->disconnect();
    						 rpcreq->deleteLater();
							 is_stop = true;
							return;
						}
						if(result.contains("height"))
						{
							QJsonValue height = result.take("height");
							qDebug()<<"\ntrying to mining block "<<height.toInt();
							minheight = height.toInt();
						}
						else
						{
							qDebug()<<"json_parse_height_error";
							 rpcreq->disconnect();
    						 rpcreq->deleteLater();
							 is_stop = true;
							return ;
						}


					}
					else
					{
						qDebug()<<"the node is not ready";
						is_stop = true;
						rpcreq->disconnect();
    					rpcreq->deleteLater();
						return;
					}

					 

                }
            }
            else
            {
                 qDebug()<<"json_parse_result_error";	
				 rpcreq->disconnect();
    			 rpcreq->deleteLater();
				 is_stop = true;
                 return ;
            }
        }
    }
    else
    {
        qDebug()<<"json_parse_error";
        qDebug()<<rpcreq->RpcResult;
		rpcreq->disconnect();
    	rpcreq->deleteLater();
		is_stop = true;
        return ;
    }

    //CBlock nblock;
    if (!DecodeHexBlk(nblock, Block.toStdString())) {
           qDebug()<<"Block decode failed";
		   is_stop = true;
		   return;
        }
    if(is_submode)
    {
        for(int i = 0; i < cpu_core ; i++)
        {
            subminer *hello = new subminer(nblock,i,cpu_core);
            connect(hello, SIGNAL(calsuccess(uint64_t)), this, SLOT(calsubmit(uint64_t)));
            QThreadPool::globalInstance()->start(hello);
        }

    }
    else
    {
        while (nblock.nNonce < nInnerLoopCount && !ECheckProofOfWork(nblock.GetHash(), nblock.nBits))
        {
            ++nblock.nNonce;
        }
        if (nblock.nNonce == nInnerLoopCount)
        {
            qDebug()<<"can't find this time : block "<<minheight;
			rpcreq->disconnect();
    		rpcreq->deleteLater();
            return ;
        }
        else
        {
            //std::cout<<"block detail:  \n"<<nblock.GetHash().GetHex()<<std::endl;
            CDataStream ssBlock(SER_NETWORK, 70016);
            ssBlock << nblock;
            std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
            qDebug()<<"mined block "<<minheight<<"success ! submitblock ......";
            submitblock(strHex);
             // std::cout <<"blockdetail:          \n"<<strHex<< std::endl;
        }
    }




    //销毁rpcreq信号和实例
    rpcreq->disconnect();
    rpcreq->deleteLater();

}
int EMiner::submitblock(std::string hex)
{
    rpcsub = new ERpcbase();
    rpcsub->setMethod("submitblock");
    rpcsub->setRpcUser(gloabl_USER);
    rpcsub->setRpcPasswd(gloabl_PASSWD);
    rpcsub->setUrl(gloabl_URL);
    QString params;
    QString Qhex;
    params.append("\"");
    params.append(Qhex.fromStdString(hex));
    params.append("\"");
    rpcsub->setParams(params);
    //rpcsub->setUrl("http://191.167.2.1:4566");
    //rpcsub->setUrl("http://191.167.2.36:23412");
    rpcsub->sendPost();
    connect(rpcsub, SIGNAL(finished()), this, SLOT(submitfinished()));
    
    for (ERpcbase* temp:buylist) {
        temp->disconnect();
        temp->deleteLater();
        buylist.removeOne(temp);
        //qDebug()<<"remove";
    }

    return 0;
}
bool EMiner::ECheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // 去除判断pos段内容，只应该保留pow部分 [12/6/2018 taoist]
    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")))
        return false;
    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
bool subminer::subCheckProofOfWork(uint256 hash, unsigned int nBits)
{

    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // 去除判断pos段内容，只应该保留pow部分 [12/6/2018 taoist]
    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")))
        return false;
    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

void EMiner::buyticket()
{
	    ERpcbase * rpcreq = new ERpcbase();
            buylist.append(rpcreq);
		rpcreq->setMethod("buyticket");
		rpcreq->setRpcUser(gloabl_USER);
		rpcreq->setRpcPasswd(gloabl_PASSWD);
		rpcreq->setUrl(gloabl_URL);
		rpcreq->setParams(gloabl_Params);
		rpcreq->sendPost();
        //qDebug() <<rpcreq->RpcResult;

}
void EMiner::submitfinished()
{
       QByteArray test = rpcsub->RpcResult;
    QJsonParseError json_error;
    QJsonDocument parse_document = QJsonDocument::fromJson(test, &json_error);
    if(json_error.error == QJsonParseError::NoError)
    {
        if(parse_document.isObject())
        {
            QJsonObject obj = parse_document.object();
            if(obj.contains("result"))
            {
                QJsonValue result_value = obj.take("result");
				/*qDebug()<<result_value;
                if(result_value.toString().isEmpty())
                {
                    qDebug()<<"you mined one block!!!";
                }
                else
                {
                    qDebug()<<"not mined this time";
                }*/


            }
        }
    }
    rpcsub->disconnect();
    rpcsub->deleteLater();
    qDebug()<<"submit finished";
    qDebug()<<"\n****************************************";
    getwork();
}

void EMiner::calsubmit(uint64_t num)
{

    mutex.lock();
    if(is_newblock_found)
    {
        mutex.unlock();
        qDebug()<<"SomeThing happend !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<num;
        return;
    }
    is_newblock_found = true;
    mutex.unlock();
    nblock.nNonce = num;
    CDataStream ssBlock(SER_NETWORK, 70016);
    ssBlock << nblock;
    std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
    qDebug()<<"mined block "<<minheight<<"success ! submitblock ......";
    submitblock(strHex);

}

