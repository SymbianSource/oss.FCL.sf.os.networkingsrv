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
// NAPT (Network Address & Port translation Code )
// 
//

/**
 @file 
 @internalComponent
 @prototype
*/

#ifndef __TRANSLATIONTABLE_H
#define __TRANSLATIONTABLE_H


#include <e32std.h>

#include <e32base.h>
#include <es_sock.h> // for BigEndian
#include <icmp6_hdr.h>
#include <ip4_hdr.h>
#include <tcp_hdr.h>
#include <in_pkt.h>
#include <ext_hdr.h>
#include <in_chk.h>
#include "naptconfigtable.h"

//Hash Table Array Size
const TUint16 KTranslationHashTableSize= 	111;

//flags for TCP session close
const TUint8 KTcpClosePacketOUT			=0x40 ;
const TUint8 KTcpClosePacketIN			=0x80 ;
const TUint8 KTcpCloseDeletePacket = KTcpClosePacketOUT | KTcpClosePacketIN |  KTcpCtlACK ;

//TImer Default Values.. All values in Seconds..
const TUint32 KTimerMaxSeconds				=2147 ;	 //Maximum Timer Interval 
const TUint32 KTableScanIntervalDefault		=3*60  ;   //3 minutes
//Inactive Protcol Timeout Default Values
const TUint32 KTcpIdleTimeOutDefault			=24*60*60; //One day
const TUint32 KUdpIdleTimeoutDefault			=3*60;	   //3 minutes
const TUint32 KIcmpIdleTimeoutDefault			=3*60;	   //3 minutes
//Tcp Close Wait Time for Napt Table Node Deletion
const TUint32 KTcpOpenTimeoutDefault		=30 ; // 30 seconds
const TUint32 KTcpCloseTimeoutDefault		=4*60;	   // 4 minutes

//Napt Port Range
const TUint16  KNaptPort_HIGH 		 		=64000;
const TUint16  KNaptPort_LOW		 		=61000;

//IcmpQueryIdRange
const TUint16  KNaptIcmpQuery_HIGH			=30000;
const TUint16  KNaptIcmpQuery_LOW			=28000;

class CNaptTimer;
class CNaptIPPortMap;

NONSHARABLE_CLASS(CNaptIPPortMap): public CBase
	{

		friend class CProtocolNapt;
	public:
	
		static CNaptIPPortMap* NewL();
    	~CNaptIPPortMap();
    	
	public:     

		TUint16     iSrcPort;    	//original source Address port
		TUint16 	iDstPort;		//just needed to compare in EntryExists function
		TUint16 	iTrPort;		//final translated  port
		TUint8 		iProtocolFlag;  //soft state for tcp session close
		TUint     	iProtocolType;  //type of protocol TCP/UDP/ICMP
		TUint32 	iSrcIpAddr;     //original source address
 		TUint32 	iTrIpAddr;      //final translated ip address
 		TUint32 	iDstIpAddr;     //just needed to compare in EntryExists function
		TUint32		iCurTime;       //used for deleting the entry based on the timout values
		TNaptConfigInfo* iConfigInfo; //used for fetching packet specific configuration in InBound hook
   		TSglQueLink* iLink;
    };  
   
class   TNaptIPPortMap : public TSglQue<CNaptIPPortMap>
	{
public:
	TNaptIPPortMap():
	TSglQue<CNaptIPPortMap>(_FOFF(CNaptIPPortMap, iLink)) {};
	};


typedef class TSglQueIter<CNaptIPPortMap> TNaptTableIter;

class TNaptTableMapMgr
	{
		friend class CProtocolNapt;
		
	public:	
		TNaptTableMapMgr();	
		~TNaptTableMapMgr();
		
		inline TInt FindBucket(TInt aPortNo){	return aPortNo % KTranslationHashTableSize;	}
		inline TInt GetIndexedPortCount(){ return iBucketsInUse;}
		CNaptIPPortMap* FindOrCreateNaptEntryL( TUint aProtocolType,const TUint32& aSrcAddr,const TUint32&  aDstAddr,const TUint16 aSrcPort,const TUint16 aDstPort, const TNaptConfigInfo *aConfigInfo);
		TBool VerifySender(const CNaptIPPortMap* aTableNode, const TUint32& aSrcIp,TUint16 aSrcPort);
		CNaptIPPortMap* GetIPTranslationNode(TUint16 aDstPortNo);
		void DeleteNodeInIndexedList(TInt aIndex, CNaptIPPortMap *aTableNode);
		void TimerComplete();
		void HandleTcpConnectionPhases(	TInet6Checksum<TInet6HeaderTCP> &aPacket, CNaptIPPortMap* aTableNode,const TInt direction);
		
	private :
		CNaptIPPortMap *AllocateTranslationNodeL(TUint  aProtocolType,const TUint32& aSrcAddr,const TUint32& aDstAddr,const TUint16 aSrcPort ,const TUint16 aDstPort, const TNaptConfigInfo *aConfigInfo, TUint16 aQueryId=0);
		void DeleteNaptTableNode( CNaptIPPortMap* aTableNode);

	private : 
		TNaptIPPortMap 		iIPPortMap[KTranslationHashTableSize];
		TUint16  	iLastPort;
		TInt16  	iBucketsInUse;
		TUint16 	iIcmpQueryId;
		TUint32 	iPublicGatewayIP;
		CNaptTimer* iTimerPtr;
	
	};


	

#endif //translation.h
