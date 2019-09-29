#pragma once

#include "uint256.h"
#include "base58.h"
#include "amount.h"
#include "script/standard.h"
#include "util.h"
#include "chain.h"
//#include "validation.h"
#include <list>
#include <string>
#include <fstream>
#include <iostream>
#include "stdint.h"



#include "boost/multi_index/identity.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/thread/locks.hpp"
#include "boost/thread/shared_mutex.hpp"


#include "boost/multi_index_container.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/hashed_index.hpp"
#include <boost/multi_index/sequenced_index.hpp>

using boost::multi_index_container;
using namespace boost::multi_index;


class Ticket
{
public:

	int ticketHeight;//块高
	uint256 ticketTxHash;//票hash
	CAmount ticketPrice;//当前票的票价
	CBitcoinAddress ticketAddress;//买票的地址


	Ticket();
	Ticket(CTransaction tx,int height);
	Ticket(const Ticket& ticket);
	Ticket(int ticketHeight, uint256 ticketTxHash, CAmount ticketPrice, CBitcoinAddress ticketAddress);
	void operator=(Ticket & ticket)
	{
		this->ticketAddress = ticket.ticketAddress;
		this->ticketHeight = ticket.ticketHeight;
		this->ticketPrice = ticket.ticketPrice;
		this->ticketTxHash = ticket.ticketTxHash;
	}
	bool operator==(Ticket ticket)
	{
		if (this->ticketAddress == ticket.ticketAddress &&
			this->ticketHeight == ticket.ticketHeight &&
			this->ticketTxHash == ticket.ticketTxHash &&
			this->ticketPrice == ticket.ticketPrice)
		{
			return true;
		}
		return false;
	}
	std::string toString();
	~Ticket();
private:

};

typedef boost::shared_mutex Lock;                  
typedef boost::unique_lock< Lock >  WriteLock;
typedef boost::shared_lock< Lock >  ReadLock;

class ticketPool
{
public:
	ticketPool();
	~ticketPool();
	struct ticket_tx_hash {};
	struct ticket_height {};


	typedef
		boost::multi_index_container<
		Ticket,
		indexed_by<
		ordered_unique<
		tag<ticket_tx_hash>, BOOST_MULTI_INDEX_MEMBER(Ticket, uint256, ticketTxHash)>,
		ordered_non_unique<
		tag<ticket_height>, BOOST_MULTI_INDEX_MEMBER(Ticket, int, ticketHeight)>
		>
		> ticketpool_set;

	ticketpool_set ticket_pool;	
	Lock ticketPoolLocker;
	
	typedef ticketpool_set::index<ticket_tx_hash>::type::iterator Ticketiter;
	ticketPool(ticketPool & ticketPool2)
	{
		ReadLock rLock(ticketPool2.ticketPoolLocker);
		WriteLock wlock(ticketPoolLocker);
		ticket_pool = ticketPool2.ticket_pool;
	}
	
	bool ticketPush(Ticket& ticket);
	bool ticketDel(Ticket ticket);
	bool ticketDelByHeight(int height);
	int getTicketpoolSize();

	std::vector<Ticket> getTicketByHeight(int height);

	int getLoadHeight()
	{
		return load_height;
	}
	void updateLoadHeight( int height )
	{
		load_height = height;
	}
	void print_ticket()
	{
		fs::path ticket_logfile = GetDataDir(false)/"ticket.log";
		std::ofstream LogFile(ticket_logfile.string());
		ReadLock rLock(ticketPoolLocker);
		for (ticketpool_set::index<ticket_height>::type::iterator ticketiter = this->ticket_pool.get<ticket_height>().begin(); ticketiter != this->ticket_pool.get<ticket_height>().end(); ticketiter++)
		{
			Ticket  ticket = *ticketiter;
			LogFile << ticket.toString() << std::endl;
		}
		LogFile.close();
	}
	//

	Ticket ticketAt(unsigned int index);



private:
	int load_height;
};

class luckyTicket
{
public:
	luckyTicket();
	~luckyTicket();
	std::vector<unsigned int> getLuckyIndex(int height,int seed = 0);

private:
	std::vector<CBitcoinAddress> vaddress;
};



