#include "ticketpool.h"
#include "tinyformat.h"
#include <iostream>
#include "validation.h"

Ticket::Ticket()
{
	
}
Ticket::Ticket(CTransaction tx,int height)
{
	this->ticketTxHash = tx.GetHash();

	this->ticketHeight = height;
	if (tx.vout[0].scriptPubKey.HasOpSStx())
	{
		this->ticketPrice = tx.vout[0].nValue;
	}

	CTxDestination address;
	ExtractDestination(tx.vout[1].scriptPubKey, address);
	CBitcoinAddress destAdress(address);

	this->ticketAddress = destAdress;

}
Ticket::Ticket(const Ticket&  ticket)
{
	this->ticketAddress = ticket.ticketAddress;
	this->ticketHeight = ticket.ticketHeight;
	this->ticketPrice = ticket.ticketPrice;
	this->ticketTxHash = ticket.ticketTxHash;
}

Ticket::Ticket(int ticketHeight, uint256 ticketTxHash, CAmount ticketPrice, CBitcoinAddress ticketAddress)
{
	this->ticketAddress =ticketAddress;
	this->ticketHeight = ticketHeight;
	this->ticketPrice = ticketPrice;
	this->ticketTxHash = ticketTxHash;
}


Ticket::~Ticket()
{
}

std::string Ticket::toString()
{
	std::string ticketStr;
	ticketStr += strprintf("ticket info:\t ticketAddress = %s \t  ticketHeight = %d \t   ticketPrice = %d \t  ticketTxHash = %s \n",
		this->ticketAddress.ToString(),
		this->ticketHeight,
		this->ticketPrice,
		this->ticketTxHash.ToString());

	return ticketStr;
}

ticketPool::ticketPool()
{
	load_height = 0;
}

ticketPool::~ticketPool()
{
}

bool ticketPool::ticketPush(Ticket& ticket)
{	
	WriteLock wlock(ticketPoolLocker);
	this->ticket_pool.insert(ticket);
	return true;
}

bool ticketPool::ticketDel(Ticket  ticket)
{
	ticketpool_set::index<ticket_tx_hash>::type & ticket_tx_hash_pool = this->ticket_pool.get<ticket_tx_hash>();
    
	Ticketiter ticketiter = ticket_tx_hash_pool.find(ticket.ticketTxHash);
	if (ticketiter != ticket_tx_hash_pool.end())
	{		
		WriteLock wlock(ticketPoolLocker);
		ticket_tx_hash_pool.erase(ticketiter);
		return true;
	}

	return false;
}

bool ticketPool::ticketDelByHeight(int height)
{
	ticketpool_set::index<ticket_height>::type& indexOfHeight = this->ticket_pool.get<ticket_height>();

	ticketpool_set::index<ticket_height>::type::iterator pbegin = indexOfHeight.lower_bound(height);
	if (pbegin == indexOfHeight.end() || pbegin->ticketHeight > height)
	{
		return true;
	}
	ticketpool_set::index<ticket_height>::type::iterator pend = indexOfHeight.upper_bound(height);
	
	WriteLock wlock(ticketPoolLocker);
	this->ticket_pool.get<ticket_height>().erase(pbegin, pend);
	return true;
}

int ticketPool::getTicketpoolSize()
{
	return ticket_pool.size();
}

std::vector<Ticket> ticketPool::getTicketByHeight(int height)
{	
	ticketpool_set::index<ticket_height>::type& indexOfHeight = this->ticket_pool.get<ticket_height>();
	std::vector<Ticket> vTicketByHeight;
	
	ticketpool_set::index<ticket_height>::type::iterator pbegin = indexOfHeight.lower_bound(height);
	ticketpool_set::index<ticket_height>::type::iterator pend = indexOfHeight.upper_bound(height);
	ReadLock rLock(ticketPoolLocker);
	while (pbegin != pend)
	{
		vTicketByHeight.push_back(*pbegin);
		pbegin++;
	}
	return vTicketByHeight;
}

Ticket ticketPool::ticketAt(unsigned int index)
{
	LogPrint(BCLog::TICKETPOOL,"index :%d   ,this->ticket_pool.size(): %d\n",index,this->ticket_pool.size());

	if (index >= this->ticket_pool.size()  )
	{
		throw std::runtime_error("ticket index error");
	}

	ReadLock rLock(ticketPoolLocker);
	Ticketiter  ticketiter = this->ticket_pool.get<ticket_tx_hash>().begin();
	for (unsigned int i = 0 ; i < index ; i++)
	{
		ticketiter++;
	}

	if (ticketiter->ticketHeight < 0)
	{
		LogPrint(BCLog::TICKETPOOL, "ticketAt : error ticket, index:%d, height:%d   ,hash: %s\n", 
						index, ticketiter->ticketHeight, ticketiter->ticketTxHash.GetHex());
		return *(this->ticket_pool.get<ticket_tx_hash>().begin());
	}
	return *ticketiter;

}
luckyTicket::luckyTicket()
{
}
luckyTicket::~luckyTicket()
{
}
std::vector<unsigned int > luckyTicket::getLuckyIndex(int height,int seed)
{  // std::cout<<"1"<<std::endl;
	CBlockIndex* pblockindex = chainActive[height - TICKETPOOL_HEIGHT_LATENCY];

	std::string u1 = pblockindex->GetBlockHash().GetHex();
	//std::cout << "ticketpoool height:" << height - TICKETPOOL_HEIGHT_LATENCY <<", load_height:" << ticketpool.getLoadHeight() << " ,pool_size:" <<ticketpool.ticket_pool.size() << " ,hash:" << u1 << std::endl;
	seed = seed % 24;
	const char * p = u1.c_str();
	char t1[5] = { 0 };
	char t2[5] = { 0 };
	char t3[5] = { 0 };
	char t4[5] = { 0 };
	char t5[5] = { 0 };
	p = p + 63 - seed;
	//printf("%s,%s,%s,%s,%s \n", t1, t2, t3, t4, t5);

	unsigned int  idata1, idata2, idata3, idata4, idata5;
	memcpy(t1, p - 4, 4);
	sscanf(t1, "%x", &idata1);
	memcpy(t2, p - 8, 4);
	memcpy(t3, p - 12, 4);
	memcpy(t4, p - 16, 4);
	memcpy(t5, p - 20, 4);
	sscanf(t2, "%x", &idata2);
	sscanf(t3, "%x", &idata3);
	sscanf(t4, "%x", &idata4);
	sscanf(t5, "%x", &idata5);
	//printf(" %d,%d,%d,%d,%d\n", idata1, idata2, idata3, idata4, idata5);
	std::vector<unsigned int> luckyIndex;
	LogPrint(BCLog::TICKETPOOL," ****** ticketpool.ticket_pool.size()   :%d\n",ticketpool.ticket_pool.size());
	//std::cout << "luckyIndex 1  = " << ticketpool.ticket_pool.size() << std::endl;
	if (ticketpool.ticket_pool.size() > 5 )
	{
		luckyIndex.push_back(idata1 % (ticketpool.ticket_pool.size() -5 ));
		luckyIndex.push_back(idata2 % (ticketpool.ticket_pool.size() -5 ));
		luckyIndex.push_back(idata3 % (ticketpool.ticket_pool.size() -5 ));
		luckyIndex.push_back(idata4 % (ticketpool.ticket_pool.size() -5 ));
		luckyIndex.push_back(idata5 % (ticketpool.ticket_pool.size() -5 ));
	}

	return luckyIndex;
}




