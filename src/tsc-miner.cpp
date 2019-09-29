#include <stdio.h>
#include <iostream>
#include <primitives/block.h>
#include "ticketpool.h"
#include "base58.h"
#include <stdio.h>
#include <vector>
//#include<fstream>
//#define ELPP_THREAD_SAFE
//
//#include<iostream>


using namespace std;
INITIALIZE_EASYLOGGINGPP

int  getLuckyIndex1(int height, int temp = 0)
{  // std::cout<<"1"<<std::endl;
	temp = temp % 24;
	std::string u1 = "0000000000000000001a4913fa3802f19d12dfa9dd92d9dba8497d2514632ab6";
	//"000000012655a42d1468ec90510f68dd98f8c28f7b1f4801dc53fd54b5a2b7b8";
				      

	const char * p = u1.c_str();
	char t1[5] = { 0 };
	char t2[5] = { 0 };
	char t3[5] = { 0 };
	char t4[5] = { 0 };
	char t5[5] = { 0 };

	p = p + 63-temp;
	//cout << "p = " << *p << endl;
	unsigned int  idata1, idata2, idata3, idata4, idata5;
	memcpy(t1, p-4, 4);
	sscanf(t1, "%x", &idata1);
	memcpy(t2, p - 8, 4);
	memcpy(t3, p -12, 4);
	memcpy(t4, p -16, 4);
	memcpy(t5, p -20, 4);
	sscanf(t2, "%x", &idata2);
	sscanf(t3, "%x", &idata3);
	sscanf(t4, "%x", &idata4);
	sscanf(t5, "%x", &idata5);
	printf("t1 = %s | t2 = %s | t3 = %s | t4 = %s | t5 = %s \n", t1, t2, t3, t4, t5);
	printf("t1 = %d | t2 = %d | t3 = %d | t4 = %d | t5 = %d \n", idata1, idata2, idata3, idata4, idata5);


	return 1;

}
int main(int argc, char* argv[])
{
	printf("hello world!\n");
	cout << "=====================================================================" << endl;

	for (int i =0 ; i<24 ; i++)
	{
		getLuckyIndex1(0,  i);
		cout << " ---------------------------------------------------- " << endl;
	}
	cout << "=====================================================================" << endl;
    return 0;

}
