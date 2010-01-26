// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include <e32base.h>
#include <es_sock.h> // for BigEndian
#include <e32hal.h>
#include <icmp6_hdr.h>
#include <ip4_hdr.h>
#include "translationtable.h"
#include "hookdefs.h"
#include "naptconfigtable.h"



//NAPT Translation Table class Constructor
CNaptIPPortMap *CNaptIPPortMap:: NewL()
	{
	CNaptIPPortMap *self = new(ELeave)  CNaptIPPortMap;
	return self;
	}

//NAPT Translation Table class Destructor
CNaptIPPortMap::~CNaptIPPortMap()
	{}
	
	
//NAPT Translation Table Manager Constructor..
TNaptTableMapMgr::TNaptTableMapMgr()
	{
	iIcmpQueryId = KNaptIcmpQuery_HIGH;
	iLastPort 	 = KNaptPort_HIGH;
	iBucketsInUse= 0;
	iPublicGatewayIP=0;
	}

//NAPT Translation Table Manager Destructor..
TNaptTableMapMgr::~TNaptTableMapMgr()
	{ }

CNaptIPPortMap* TNaptTableMapMgr::GetIPTranslationNode(TUint16 aDstPortNo)
	/**
	* Gets the translated ip & port number information 
	* Mainly used in incoming packet conversion
	@internalTechnology
	@param aTrDstPort  	- Unique Translated Port Information
	*
	**/ 
	{
	TInt bucket = FindBucket(aDstPortNo);
	CNaptIPPortMap* table = NULL;
	
	TNaptTableIter naptTableIter(iIPPortMap[bucket]);
	naptTableIter.SetToFirst();

 	 while ((table = naptTableIter++) != NULL)
 		{
 		if (table->iTrPort == aDstPortNo)
 			{
 			return table;
 			}
 		}//while
    
	return NULL;
	}


CNaptIPPortMap* TNaptTableMapMgr::AllocateTranslationNodeL(TUint  aProtocolType,const TUint32& aSrcAddr,const TUint32& aDstAddr,const TUint16 aSrcPort ,const TUint16 aDstPort, const TNaptConfigInfo* aConfigInfo, TUint16 aQueryId)
	/**
	*Allocates Translation Node with a new unique translated port number for 
	*translated IP for TCP/UDP connections.  
	@param 	aProtocolType
	@param  aSrcAddr
	@param 	aDstAddr
	@param 	aSrcPort
	@param  aDstPort
      @param  aConfigInfo
	@param	aQueryId
	*
	* The function is called only once when the first packet in the session passes 
	* through NAPT hook.   
	* The function is called only when the corresponding arguments are not present 
	* anywhere in the indexed translation table list.  
	* For TCP/UDP protocols the function generates new unique port number in the 
	* predefined range.  The generated port is used as a key to index in the 
	* translation table.  
	* For ICMP packets like ICMP echo/reply(used in ping application), the function
	* doesnot generates a port number, instead it uses the QueryId of the ICMP 
	* message, ie the QueryId is used as the key to index the translation table.
	*
	* The index is generated based on the key  using the FindBucket Hash Function, 
	* a new node is  linked to the indexed node.
	*
	* returns the filled CNaptIPPortMap Translated Node information.
	**/
	{
	
	TInt index =0;
	CNaptIPPortMap *tableNode=NULL;
	
	if (iLastPort == KNaptPort_LOW) //lower range of the port
		{
		iLastPort = KNaptPort_HIGH;
		}

	
	// we have to allocate memory..so we will allocate in the begining itself 
	// fill all values except translated port number  
	tableNode = CNaptIPPortMap::NewL();

	tableNode->iCurTime= User::NTickCount();
	tableNode->iProtocolType = aProtocolType;
	tableNode->iSrcPort = aSrcPort;	//incase of ICMP echo request it stores original query id
	tableNode->iDstPort = aDstPort;

	//we should be knowing what tranlsated destination address to be filled
	tableNode->iTrIpAddr  = iPublicGatewayIP; 
	tableNode->iSrcIpAddr = aSrcAddr;
	tableNode->iDstIpAddr = aDstAddr;
	tableNode->iConfigInfo = const_cast<TNaptConfigInfo* >(aConfigInfo);	
		
	//for ICMP messages or If full indexed TranslationTable is Full
	if (iBucketsInUse == KTranslationHashTableSize || aProtocolType == KProtocolInetIcmp) // table is full, 
		{
		TInt key;
		if (aProtocolType == KProtocolInetIcmp)
			{
			key = aQueryId; 
			}
		else
			{
			key = iLastPort--;
			}
			
	
		tableNode->iTrPort = key;	
		
		index = FindBucket(key);
		if (iIPPortMap[index].IsEmpty())
			{
			iBucketsInUse++;	
			}
		iIPPortMap[index].AddFirst(*tableNode);

		}
	else
		{
		
		//following algorithm is the Open Addressing Algorithm for finding out the empty slot.
		TInt tempCnt=0;

		index = FindBucket(iLastPort);
		while (! iIPPortMap[index].IsEmpty()  && (tempCnt <= iBucketsInUse) )
			{
			--iLastPort;   // decrementthe iLastPort, hope that next index generated will be empty in translation table.

			if (iLastPort <= KNaptPort_LOW) //lower range of the port
				{
				iLastPort = KNaptPort_HIGH;
				}
		
			++tempCnt;
			index = FindBucket(iLastPort);
		
			}//while
		
			
			//found an empty slot... from above loop
			if (iIPPortMap[index].IsEmpty())
				{
				iBucketsInUse++;
				}
			iIPPortMap[index].AddFirst( *tableNode);
			tableNode->iTrPort  = iLastPort--;
		}//else	
		
	//time.. to start timer		
	if (iBucketsInUse == 1 ) //time.. to start timer	
		{
		iTimerPtr->StartTimer();
		}
	return tableNode;

	}
	
	
CNaptIPPortMap* TNaptTableMapMgr::FindOrCreateNaptEntryL( TUint aProtocolType,const TUint32& aSrcAddr,const TUint32&  aDstAddr,const TUint16 aSrcPort,const TUint16 aDstPort, const TNaptConfigInfo *aConfigInfo)
	/**
	*Finds  Translation Node  if already existing or creates  a new node filled with unique translated port number for 
	*translated IP for TCP/UDP/ICMP connections.  
	@param 	aProtocolType- TCP/UDP/ICMP
	@param  aSrcAddr - Source Address from which packet is originated
	@param 	aDstAddr - Destination Address to which packet is destined
	@param 	aSrcPort - Source Port from which packet is originated
	@param  aDstPort - Destination Port to which packet is destined
      @param aConfigInfo - Configuration to be used for packet translation
	*
	* The function is called each time the packet needs translation passes through NAPT hook 
	* Function First checks any NAPT table node exists with the matched arguments.
	* If the node does not exist, creates a new node.
	* returns the filled CNaptIPPortMap Translated Node information.
	**/
	{
		CNaptIPPortMap *table=NULL;
		for (TInt index=0;index<KTranslationHashTableSize;index++)
			{

			TNaptTableIter naptTableIter(iIPPortMap[index]);
			naptTableIter.SetToFirst();
   		 	while ((table = naptTableIter++) != NULL)
        		{
        		if (table->iSrcPort == aSrcPort && table->iDstPort== aDstPort 
				&& table->iProtocolType == aProtocolType 
				&& table->iSrcIpAddr==aSrcAddr && table->iDstIpAddr==aDstAddr)
					{
					table->iCurTime = User::NTickCount();
					return table;
					}
				}
			}
		//there is no entry in the translation table..make an entry..
		if (aProtocolType == KProtocolInetIcmp)
			{
			if (iIcmpQueryId == KNaptIcmpQuery_LOW)
				{
				iIcmpQueryId=KNaptIcmpQuery_LOW;	
				}
			}
		
		table = AllocateTranslationNodeL(aProtocolType, aSrcAddr,aDstAddr,aSrcPort,aDstPort,aConfigInfo,iIcmpQueryId--);
		
		return table;
	
	}
	
void TNaptTableMapMgr::TimerComplete()
	/**
	*
	*  Deletes the timed out transactions
	*  Function is called after  timer expiry interval.  
	*  If there are no entries present, Stop the Timer.
	*  The functions scans through all translation table entries and deletes the 
	*  node entries which are old ones based on the configured protocol time out entries.
	*
	**/
	{

	CNaptIPPortMap* table=NULL;
	TBool deleteFlag= EFalse;
	TUint32 curTime;
	TInt seconds=0;
	
	for (TInt index=0; index< KTranslationHashTableSize; index++)
		{
		TNaptTableIter naptTableIter(iIPPortMap[index]);
		naptTableIter.SetToFirst();

		while ((table = naptTableIter++) != NULL)
    		{
    		deleteFlag = EFalse;
			curTime = User::NTickCount();
			if (curTime <= table->iCurTime)//checking clock wrap around
   				{
    			seconds = ((KMaxTUint32 - table->iCurTime) + curTime)/1000; 
    			}
			else
    			{
    			seconds = (curTime - table->iCurTime)/1000;
    			}

			//Depending on the protocol type delete transactions
			switch (table->iProtocolType)
				{
				case KProtocolInetTcp:
					if (seconds >= iTimerPtr->iNaptTcpIdleTimeout)//this is for TCP Inactive Connections..
						{
						deleteFlag=ETrue;
						}
					if (seconds >= iTimerPtr->iNaptTcpCloseTimeout && table->iProtocolFlag==KTcpCloseDeletePacket )
						{
						deleteFlag=ETrue;
						}
					if (seconds >= iTimerPtr->iNaptTcpOpenTimeout && table->iProtocolFlag==KTcpCtlSYN)
						{
						deleteFlag=ETrue;
						}
				
				break;
		
				case KProtocolInetUdp:
					if (seconds >= iTimerPtr->iNaptUdpIdleTimeout)
						{
						deleteFlag=ETrue;
						}
				break;
	
				case KProtocolInetIcmp:
					if (seconds >= iTimerPtr->iNaptIcmpIdleTimeout )
						{
						deleteFlag=ETrue;
						}
				break;
				}//end of switch
			if (deleteFlag)
				{
				DeleteNodeInIndexedList(index,table);
				}
			}//end of while loop inside linked list
		}//end of for loop entire index 
	}
	


void TNaptTableMapMgr::DeleteNaptTableNode( CNaptIPPortMap *aTableNode)
	/**
	*
	* Deletes the specified NAPT Translation Node in the Indexed list.
	@param aTableNode - Table Node to be Deleted.
	*
	*/
	{
	TInt index= FindBucket(aTableNode->iTrPort);
	DeleteNodeInIndexedList(index,aTableNode);

	}

void TNaptTableMapMgr::DeleteNodeInIndexedList(TInt aIndex, CNaptIPPortMap *aTableNode)
	/*
	*
	* Deletes the specified NAPT table node in the indexed linked list.
	@param----aIndex - index at which aTableNode exist.
	@param----aTableNode - napt table node  to be deleted
	* After deleting the specified node, decrement the iBucketsInUse if the indexed list is empty.
	* Stop the timer, if there are no napt table nodes exists for translation.
	*
	*/
	{
	
	iIPPortMap[aIndex].Remove(*aTableNode);	
	delete aTableNode;
	if (iIPPortMap[aIndex].IsEmpty())//check list is empty.
		{
		iBucketsInUse--;
		}
					
	if (iBucketsInUse == 0) //Stop the Timer as there are no connections to translate
		{
		iTimerPtr->Cancel();
		}
	}
	
TBool TNaptTableMapMgr::VerifySender(const CNaptIPPortMap *aTableNode, const TUint32 &aSrcIp,TUint16 aSrcPort)
	/*
	*
	* Function Matches the Sender IP and Port Number with the Stored NAPT translated node
	@param---aTableNode - Napt Table Node to which the Sender IP & Port Number to be matched.
	@param---aSrcIp -	Sender's source IP Address 
	@param----aSrcPort - Sender's source port number
	*
	*/
	{
	if ( aTableNode->iDstPort == aSrcPort  && aTableNode->iDstIpAddr==aSrcIp)
		{
		return ETrue;
		}
	return EFalse;
	}
	
	
	
void TNaptTableMapMgr::HandleTcpConnectionPhases(TInet6Checksum<TInet6HeaderTCP>& aTcpPacket, CNaptIPPortMap* aTableNode,const TInt aPacketDirection)
	/*
	*
	* Function Handles Tcp Close Connection Phase mainly.. and open sequence,
	* and open sequence,if it is originated during close sequence.
	@param---aTcpPacket - Tcp Packet to be checked for FIN,ACK,RST & SYN Bits
	@param----TableNode - Napt Table Node at which the TCP packet's above information to be stored. 
	@param-----aPacketDirection - Direction which packet is travelling IN/OUT
	* The NAPT table node will be marked for deletion if corresponding FIN+ACK messages are exchanged.
	* But NAPT table node will be deleted immediately after the RST Flag is received from the private
	*
	*/
	{

	TUint8 synBit = aTcpPacket.iHdr->SYN();
	if (synBit)
	   	{
	   	//Reset the protocol Flag to have only to have Syn Bit..so that if pending fin is cancelled.
	   	//The reset enables SYN to be received, ie new connection initiation request to pass through
	   	// and to cancel the NAPT table node entry node for deletion
		aTableNode->iProtocolFlag = KTcpCtlSYN;
		//its good to return now here as we dont need to check, as other bits like RST/FIN wont 
		//be transmitted along with SYN bit message.
		return;
	   	}	
	else
		{
		//Reset Syn Bit...to come out of TCP SYN open timer..
		aTableNode->iProtocolFlag &= ~KTcpCtlSYN;
		}
	
	TUint8 ackBit = aTcpPacket.iHdr->ACK();
	TUint8 finBit = aTcpPacket.iHdr->FIN();
	TUint8 rstBit = aTcpPacket.iHdr->RST();
	
	//Putting this below condition before the actual FIN bit,  ensures that FIN+ACK in the same packet
	//would not mark the packet for deletion. The node is marked for delete after the receive of FIN 
	//exchanges from both ends. 
	//The NAPT table node is deleted only after close time out period after receiving last ACK which initiated FIN.
	if ((aTableNode->iProtocolFlag == (KTcpClosePacketOUT|KTcpClosePacketIN)) && ackBit )
		{
		//The above condition ensures that ACK is received for the corresponding FIN which has been sent earlier.
		//However it is not checking ACK received  is the one  corresponding ACK for the FIN(can be done by sequence number matching)
		//As we are not deleting NAPT table node immediately, the execption of receiving ACK for the not
		//correct FIN can be ignored and we rely on TCP clien ends resolve the correct sequence number if they are not the same.
		
		
		//Also when FIN is sent by the end client(its in HALF close FIN_WAIT state), it wont send any 
		//further data but it is able to recive data. The next message sent by the closing client,
		//is either FIN(retransmission) or ACK.
	
		//It handles many Close Scenarios like below(list.. is not complete)
		// 1.FIN ->, <- ACK, <- FIN, ACK -> (rare, but possible)[Simulteneous close]
		// 2.FIN+ACK ->, <- FIN+ACK, ACK ->
		// 3.FIN ->, <- FIN+ACK, ACK ->
		// 4.FIN+ACK ->, <- FIN, <- ACK, ACK ->(rare, but possible)[very Simulteneous close]..etc
		// However in last case..the table node is marked before the last ACk is originated. This seems ok, as we are not 
		// deleting any packet 
		aTableNode->iProtocolFlag |= ackBit ; //now it is KTcpCloseDeletePacket	
		
		}
	if (finBit)
		{
		aTableNode->iProtocolFlag |= aPacketDirection;
		return;
		}
		

	//Delete NAPT table node entry only if RST is originated from private subnet
	//RST coming from global interface have to wait for  Close timeout period 
	//for the entry to be deleted.  	
	if (rstBit && (aPacketDirection == KTcpClosePacketOUT))
		{
		DeleteNaptTableNode(aTableNode);
		}  		
	
	}
	
