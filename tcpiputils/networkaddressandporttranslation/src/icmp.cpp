// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// napt.cpp
// implementation of ICMP tranlation
// 
//

/**
 @file
 @internalTechnology 
*/

#include "hookdefs.h"
#include <e32std.h>
#include <in6_opt.h>
#include <udp_hdr.h>
#include <icmp6_hdr.h>
#include <udp_hdr.h>
#include <in_pkt.h>
#include <ext_hdr.h>
#include <in_chk.h>
#include <tcp_hdr.h>
#include <icmp6_hdr.h>
#include "naptlog.h"
#include "panic.h"
#include "napt_ini.h"
#include <posthook.h>


/*
*----------------------------------------------------------------------------------------
*						ICMP FILE
*  Every ICMP message is supported except ICMP Redirect message. NAPT will not temper
*  Other messages are taken care.
*------------------------------------------------------------------------------------------
*/


TInt CProtocolNaptIn::IcmpHandler(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
/* This Function will manipulate ICMP packets. It will handle two ICMP cases.
 * First ping request.
 * Second is Destination unreachable
 * @param aPacket
 * @param aInfo
*/
	{
	TUint lSrcPort;
    // This flag states that packets have been translated and that KIp6Hook_DONE should be used as return value
	TBool translatedFlag = EFalse;

	TInet6Checksum<TInet6HeaderIP4> lIp(aPacket);
	TInt lengthIP	= lIp.iHdr->HeaderLength();
    
	//Obtain ICMP packet for checking error type 
    TInet6Checksum<TInet6HeaderICMP_Echo> lIcmpIn(aPacket,lengthIP);
    TUint type = lIcmpIn.iHdr->Type();
	
	//query identifier for particular ICMP echo reply
    TUint16 queryId = lIcmpIn.iHdr->Identifier();
	
	const CNaptIPPortMap* table= NULL;
	TNaptConfigInfo* info = NULL;
 
	switch(type)
		{
			case KInet4ICMP_TimeStampReply:
				LOG(Log::Printf(_L("ICMP type is time stamp reply ")));
			case KInet4ICMP_EchoReply:
						
				LOG(Log::Printf(_L("ICMP type is PING response ")));

				//check whether translation is required or not.Result should be KErrNone
				table =iNapt->iNaptMapMgr.GetIPTranslationNode(queryId);

		    	if (table)
		    		{
		    		lIcmpIn.iHdr->SetIdentifier(table->iSrcPort); //Store Original Query Id from node
		    		info = table->iConfigInfo; //Store Config info in the local pointer
		    	
		    		//recompute checksum as we have changed query id
		    		lIcmpIn.ComputeChecksum(aPacket,NULL);
		    	    		
		    	  	//Take original IP which is translated by NAPT.
		 			TUint32 originalIP;
			  		originalIP=table->iSrcIpAddr;
			            
		            //Set destination IP to the Original IP which was translated
		            lIp.iHdr->SetDstAddr(originalIP);
		            TInetAddr::Cast(aInfo.iDstAddr).SetV4MappedAddress(originalIP);

					//This will set source scope i,e networkID as zero this will allow incoming hook to set scope 
					//by itself.

		            lIp.ComputeChecksum();
		            translatedFlag = ETrue;
		            
		    		}
		    	break;
				
			case KInet4ICMP_SourceQuench:

		 	case KInet4ICMP_TimeExceeded:
				//IcmpHandlerTracert return type is TBool, which is same as TInt, hence it the return value is 
				//directly assigned to translatedFlag. Any change in return value (other than 0 and 1), 
				//this assginment have to be revisited.
		 	   translatedFlag =	IcmpHandlerTracert(aPacket, aInfo, &info);
		 		break;

				
			case KInet4ICMP_ParameterProblem:
			
			case KInet4ICMP_Unreachable:
				LOG(Log::Printf(_L("my type id is %d"),type));

/*------------------------------------------------------------------------------------				
*			 	ICMP destination Unreachable packet format.
*				
*					-------------------------------------------------
*			        |	OUTER	|ICMP	|INNER	|UDP		|PAYLOAD|
*				    |	IP		|HEADER	|IP     |           |       |
*				    |	HEADER	|		|header |HEADER		|		|
*					-------------------------------------------------
*
*  Rare case TYPE 3 CODE 4..NAPT not required 
*
*From RFC 1191 for use when the code is set to 4: 
*when a router is unable to forward a datagram because it exceeds the MTU of the 
*next-hop network and its Don't Fragment bit is set, the router is required to 
*return an ICMP Destination Unreachable message to the source of the datagram,
*with the Code indicating "fragmentation needed and DF set". 
*To support the Path MTU Discovery technique specified in this memo, 
*the router MUST include the MTU of that next-hop network in the low-order 16 bits of
*the ICMP header field that is labelled "unused" in the ICMP specification.
*The high-order 16 bits remain unused, and MUST be set to zero....
*The size in bytes of the largest datagram that could be forwarded, 
*along the path of the original datagram, without being fragmented at this
*router. The size includes the IP header and IP data, and does not include 
*any lower-level headers. This field will never contain a value less than 
*68 since every router must be able to forward a 68 byte datagram without 
*fragmentation.
---------------------------------------------------------------------------------------
*/
	
				//Split ICMP destnation Unreachable packet.
				RMBufChain payload;
				aPacket.SplitL(lengthIP, payload); 
				
				TInet6Checksum<TInet6HeaderICMP> lIcmpIn(payload);

				TInt code = lIcmpIn.iHdr->Code();
				TInt lenHeader=0;
     		    if(code==KInet4ICMP_CODE_DF)
     		    	{
     		    	//rare case TYPE 3, CODE 4 dont need NAPT.Since there is not Transport header
			 		LOG(
						TBuf<70> tmpSrc;
						TBuf<70> tmpDst;
						TInetAddr::Cast(aInfo.iSrcAddr).OutputWithScope(tmpSrc);
						TInetAddr::Cast(aInfo.iDstAddr).OutputWithScope(tmpDst);
						LOG(Log::Printf(_L("\t I am ICMP packet dont reqquire NAPT \n src=[%S] dst=[%S] ICMP code =%d"), &tmpSrc, &tmpDst,code,type));
						);

     		    	aPacket.Append(payload);
     		    	return KIp6Hook_PASS;
     		    	}
     		    
     		    
     		    //Length of ICMP header + 4 is size of Empty + next Hop MTU
				lenHeader= lIcmpIn.iHdr->HeaderLength() + 4 ;
				
				//Inner IP header
				TInet6Checksum<TInet6HeaderIP4> lIpIn(payload, lenHeader);
				lenHeader += lIpIn.iHdr->HeaderLength();
			
				//Find transport protocol it is using.According to this Translation will be made.
				TInt transport;
				transport=lIpIn.iHdr->Protocol();

				LOG(Log::Printf(_L("I am using protocol=%d for communication and my destination is unreachable"),transport));
				if(transport==KProtocolInetUdp)
					{
				
					//Inner UDP header.
					TInet6Packet<TInet6HeaderUDP> lUdpIn(payload,lenHeader);
				
					//check if header length is appropriate
					if(lUdpIn.iHdr==NULL)
						{
						LOG(Log::Printf(_L("Insuffient header length")));
						aPacket.Append(payload);
						return KIp6Hook_PASS;
						}
					lenHeader += lUdpIn.iHdr->HeaderLength();
					lSrcPort = lUdpIn.iHdr->SrcPort();
		 
		 			//Check if there is an entry in the translation table.
					table = iNapt->iNaptMapMgr.GetIPTranslationNode(lSrcPort);
					
					
					if(table)
						{
			    	  	TUint32 originalIP;
			    	  	originalIP = table->iSrcIpAddr;
			    	  	TUint originalPort = table->iSrcPort;
			    	  	info = table->iConfigInfo;

						lIp.iHdr->SetDstAddr(originalIP);
		            
		                //Translate inner IP information
						lIpIn.iHdr->SetSrcAddr(originalIP);
						lIpIn.ComputeChecksum();
					
						//Translate inner Port			
						lUdpIn.iHdr->SetSrcPort(originalPort);
					
					
						lUdpIn.iHdr->SetChecksum(0);
			           
			   			
			            TInetAddr::Cast(aInfo.iDstAddr).SetV4MappedAddress(originalIP);
						
						//This will set source scope i,e networkID of private Interface this will allow incoming hook to set scope 
						//by itself.
						lIp.ComputeChecksum();
		 	            translatedFlag = ETrue;

						}
				
					aPacket.Append(payload);
					//NULL argument id for ICMP v4 packets	
					lIcmpIn.ComputeChecksum(aPacket,NULL,NULL);

				    }//Protocol UDP if
			
				else if(transport==KProtocolInetTcp)
					{
					//Tcp Header
					TInet6Checksum<TInet6HeaderTCP> lTcpIn(payload, lenHeader);
					//check if header length is appropriate
					if(lTcpIn.iHdr==NULL)
						{
						LOG(Log::Printf(_L("Insuffient header length rare case type 3 code 4")));
						aPacket.Append(payload);
						return KIp6Hook_PASS;
						}

					lenHeader += lTcpIn.iHdr->HeaderLength();
					lSrcPort = lTcpIn.iHdr->SrcPort();

					//check whether packet need translation...
					table =iNapt->iNaptMapMgr.GetIPTranslationNode(lSrcPort);
					
					if(table)
						{
			    	  	TUint32 originalIP;
			    	  	originalIP =table->iSrcIpAddr;
			    	  	TUint originalPort = table->iSrcPort;
			    	  	info = table->iConfigInfo;
						
						//outer IP header...
						lIp.iHdr->SetDstAddr(originalIP);
						
						//Translate Inner IP header...
						lIpIn.iHdr->SetSrcAddr(originalIP);
						lIpIn.ComputeChecksum();
										
						//Translate Port....
						lTcpIn.iHdr->SetSrcPort(originalPort);
			   	
			   	
			   			//this one is to avoid armv5 errors and calculate checksum right
						RMBufChain& payload = static_cast<RMBufChain&>(aPacket);	   	
			   			lTcpIn.ComputeChecksum(payload,&aInfo,aInfo.iOffset);
			
			            TInetAddr::Cast(aInfo.iDstAddr).SetV4MappedAddress(originalIP);
			

						lIp.ComputeChecksum();
			            translatedFlag = ETrue;

		 				}
					aPacket.Append(payload);
					//NULL argument id for ICMP v4 packets
				 	lIcmpIn.ComputeChecksum(aPacket,NULL,NULL);
					}//Protocol TCP if
				break;	
	 		}//Switch end
	if(translatedFlag)
		{
		aInfo.iFlags &= ~KIpAddressVerified;
		LOG(
			TBuf<70> tmpSrc;
			TBuf<70> tmpDst;
			TInetAddr::Cast(aInfo.iSrcAddr).OutputWithScope(tmpSrc);
			TInetAddr::Cast(aInfo.iDstAddr).OutputWithScope(tmpDst);
			LOG(Log::Printf(_L("\t I am translated ICMP packet \n src=[%S] dst=[%S] proto=%d"), &tmpSrc, &tmpDst, aInfo.iProtocol));
			);

	    // Setting the scope to 0 will cause the stack to automatically fill in the correct scope itself
		TInetAddr::Cast(aInfo.iSrcAddr).SetScope(0);

		//Setting Network ID of private Interfae as a scope. Route will be searched according 
		//to the Destination Id which is set as the scope of private interface.
		TInetAddr::Cast(aInfo.iDstAddr).SetScope(info->iScopeSrc);
		
		return KIp6Hook_DONE;	
		}
	
	return KIp6Hook_PASS;

	}
	
	
void CProtocolNapt::IcmpHandlerForward(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
/* This Function will manipulate ICMP packets. It will handle two ICMP cases.
 * First ping request.
 * @param aPacket
 * @param aInfo
*/
	{
		
    LOG(Log::Printf(_L(" CProtocolNapt::IcmpHandlerForwardL called for out going packets to global interface")));

	//retrieve information from aInfo. This information will be maintained in the transation 
	//table.
 	TUint32 sourceIP = (TInetAddr::Cast(aInfo.iSrcAddr)).Address();
    TUint32 destinationIP =(TInetAddr::Cast(aInfo.iDstAddr)).Address();
    const TUint32 targetIP= iCurConfigInfo->iPublicGatewayIP.Address();
	
	//IP packet.lengthIP is the length of IP header.
	TInet6Checksum<TInet6HeaderIP4> lIp(aPacket);
	TInt lengthIP	= lIp.iHdr->HeaderLength();
    
	//Obtain ICMP packet. 
    TInet6Checksum<TInet6HeaderICMP_Echo> lIcmpIn(aPacket,lengthIP);
    TUint type = lIcmpIn.iHdr->Type();
    
	//check if the type is ECHO i.e. 8. Then only translate else just pass packets.
    switch(type)
	    {
		
    case KInet4ICMP_TimeStamp:
    
	case KInetICMP_Information_Request:
	
	case KInet4ICMP_AddressMask_Request:

	case KInet4ICMP_DNS_Request:
	    
    case KInet4ICMP_Echo :
		LOG(Log::Printf(_L("my type id is %d"),type));

	    TUint16 orgQueryId = lIcmpIn.iHdr->Identifier();

		const CNaptIPPortMap* table=NULL;

		//Finds  Translation Node,if already existing or creates  a new node filled with unique translated port number for 
		//translated IP for TCP/UDP/ICMP connections.  
		TRAPD(ret,table=iNaptMapMgr.FindOrCreateNaptEntryL(KProtocolInetIcmp ,sourceIP, destinationIP,orgQueryId ,0, iCurConfigInfo));
	
		if(table==NULL || (ret== KErrNoMemory))
			{
			LOG(Log::Printf(_L("No entry for table ")));
			return;
			}
			
		//set new Query Id -extract which is set in table node
		lIcmpIn.iHdr->SetIdentifier(table->iTrPort)	;
		//recompute checksum as we have changed QueryId in IcmpEchoRequest packet.
		lIcmpIn.ComputeChecksum(aPacket, NULL);
						
		lIp.iHdr->SetSrcAddr(targetIP);
	    TInetAddr::Cast(aInfo.iSrcAddr).SetV4MappedAddress(targetIP);

	    // Setting the scope to 0 will cause the stack to automatically fill in the correct scope itself
		TInetAddr::Cast(aInfo.iSrcAddr).SetScope(0);

		//Setting Network ID of desired interface as a scope. Route will be searched according 
		//to the Destination Id which is set as the scope of desired interface.
		TInetAddr::Cast(aInfo.iDstAddr).SetScope(iCurConfigInfo->iScopedest);
		LOG(
			TBuf<70> tmpSrc;
			TBuf<70> tmpDst;
			TInetAddr::Cast(aInfo.iSrcAddr).OutputWithScope(tmpSrc);
			TInetAddr::Cast(aInfo.iDstAddr).OutputWithScope(tmpDst);
			LOG(Log::Printf(_L("\t I am translated ICMP packet OutBound \n src=[%S] dst=[%S] proto=%d"), &tmpSrc, &tmpDst, aInfo.iProtocol));
			);

		//Compute checksum
		lIp.ComputeChecksum();
		aInfo.iFlags &= ~KIpAddressVerified;
		break;
        }
	}


TInt CProtocolNaptIn::IcmpHandlerTracert(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo, TNaptConfigInfo** aConfigInfo)
/*
------------------------------------------------------------------------------------------------
*				Packet format for ICMP type 11(Time exceeded message)
*				
*				 -------------------------------------------------------
*				|IP 	|ICMP	 	|IP			| ICMP					|
*				|Header	|Header	 	|Header		| Header				|
*				|		|Type =11	|			| Type = 8(ping request)|
*				 -------------------------------------------------------
*	The internet header plus the first 8 bytes of the original datagram's data is returned to the sender. 
*	This data is used by the host to match the message to the appropriate process.
*	If a higher level protocol uses port numbers, they are assumed to be in the first 64 data bits of the original datagram's data.
*
*	Tracert could be a application.Clients can send UDP packets for tracert.64 bits(8bytes)might contain transport header.
*	
*
*				 -------------------------------------------------------
*				|IP 	|ICMP	 	|IP			| 	64 bits of data 	|
*				|Header	|Header	 	|Header		| 	which could contain	|
*				|		|Type =11	|			| 	port)				|
*				 -------------------------------------------------------
*
*
-------------------------------------------------------------------------------------------------
*/
	{
	//source port of the packet
	TUint lSrcPort;
	
	const CNaptIPPortMap* table= NULL;
	
	//IP header,this header will be outer most header of the packet. Take header length 
	//and split packet.
	TInet6Checksum<TInet6HeaderIP4> lIp(aPacket);
	TInt lengthIP	= lIp.iHdr->HeaderLength();
	
    // This flag states that packets have been translated and that KIp6Hook_DONE should be used as return value
	TBool translatedFlag= EFalse;

	//Split packet and seperate outer IP and ICMP
	RMBufChain payload;
	aPacket.SplitL(lengthIP, payload); 


	TInet6Checksum<TInet6HeaderICMP> lIcmpTem(payload);

	//Length of ICMP header + 4 is size of Empty + next Hop MTU
	TInt headerLen = lIcmpTem.iHdr->HeaderLength() + 4 ;

	//Inner IP header
	TInet6Checksum<TInet6HeaderIP4> lIpInternal(payload, headerLen);
	headerLen += lIpInternal.iHdr->HeaderLength();
	
	//Find whether after inner header there is transport header
	TInt transport= lIpInternal.iHdr->Protocol();
	
	if(transport==KProtocolInetUdp)
		{
		//Inner UDP header.
		TInet6Packet<TInet6HeaderUDP> lUdpIn(payload,headerLen);

		//check if header length is appropriate
		if(lUdpIn.iHdr==NULL)
			{
			LOG(Log::Printf(_L("Insuffient header length")));
			aPacket.Append(payload);
			return KIp6Hook_PASS;
			}
		headerLen += lUdpIn.iHdr->HeaderLength();
		lSrcPort = lUdpIn.iHdr->SrcPort();

		//Check if there is an entry in the translation table.
		table = iNapt->iNaptMapMgr.GetIPTranslationNode(lSrcPort);

		if(table)
			{
			TUint32 originalIP;
			originalIP = table->iSrcIpAddr;
			TUint originalPort = table->iSrcPort;
			*aConfigInfo = table->iConfigInfo;

			lIp.iHdr->SetDstAddr(originalIP);

			//Translate inner IP information
			lIpInternal.iHdr->SetSrcAddr(originalIP);
			lIpInternal.ComputeChecksum();

			//Translate inner Port			
			lUdpIn.iHdr->SetSrcPort(originalPort);


			lUdpIn.iHdr->SetChecksum(0);

			TInetAddr::Cast(aInfo.iDstAddr).SetV4MappedAddress(originalIP);

			//This will set source scope i,e networkID of private Interface this will allow incoming hook to set scope 
			//by itself.
			lIp.ComputeChecksum();
			translatedFlag = ETrue;
			}
		//Append splitted packet.Means join outer IP and ICMP.
		aPacket.Append(payload);
	
		//NULL argument id for ICMP v4 packets	
		lIcmpTem.ComputeChecksum(aPacket,NULL,NULL);

		}//Protocol UDP if

	else if(transport==KProtocolInetTcp)
		{
		//Tcp Header
		TInet6Checksum<TInet6HeaderTCP> lTcpIn(payload, headerLen);
		//check if header length is appropriate
		if(lTcpIn.iHdr==NULL)
			{
			LOG(Log::Printf(_L("Insuffient header length")));
			aPacket.Append(payload);
			return KIp6Hook_PASS;
			}

		headerLen += lTcpIn.iHdr->HeaderLength();
		lSrcPort = lTcpIn.iHdr->SrcPort();

		//check whether packet need translation...
		table =iNapt->iNaptMapMgr.GetIPTranslationNode(lSrcPort);

		if(table)
			{
			TUint32 originalIP;
			originalIP =table->iSrcIpAddr;
			TUint originalPort = table->iSrcPort;
            *aConfigInfo = table->iConfigInfo;
			//outer IP header...
			lIp.iHdr->SetDstAddr(originalIP);

			//Translate Inner IP header...
			lIpInternal.iHdr->SetSrcAddr(originalIP);
			lIpInternal.ComputeChecksum();
						
			//Translate Port....
			lTcpIn.iHdr->SetSrcPort(originalPort);


			//this one is to avoid armv5 errors and calculate checksum right
			RMBufChain& payload = static_cast<RMBufChain&>(aPacket);	   	
			lTcpIn.ComputeChecksum(payload,&aInfo,aInfo.iOffset);

			TInetAddr::Cast(aInfo.iDstAddr).SetV4MappedAddress(originalIP);


			lIp.ComputeChecksum();
			translatedFlag = ETrue;

			}
		aPacket.Append(payload);//Append splitted packet.Means join outer IP and ICMP.
		//NULL argument id for ICMP v4 packets
		lIcmpTem.ComputeChecksum(aPacket,NULL,NULL);
		}//Protocol TCP if
	else
		{
		TInet6Checksum<TInet6HeaderICMP_Echo> lIcmpPingHeader(payload, headerLen);

		TUint16 queryId = lIcmpPingHeader.iHdr->Identifier();

		table =iNapt->iNaptMapMgr.GetIPTranslationNode(queryId);

		if(table)
			{
			TUint32 originalIP;
			originalIP =table->iSrcIpAddr;
            *aConfigInfo = table->iConfigInfo;
			//outer IP header...
			lIp.iHdr->SetDstAddr(originalIP);

			//Translate Inner IP header...
			lIpInternal.iHdr->SetSrcAddr(originalIP);
			lIpInternal.ComputeChecksum();

			lIp.ComputeChecksum();
			translatedFlag = ETrue;

			lIcmpPingHeader.ComputeChecksum(aPacket,NULL,NULL);
			}
		//Append splitted packet.Means join outer IP and ICMP.
		aPacket.Append(payload);
		lIcmpTem.ComputeChecksum(aPacket,NULL,NULL);

	}//ICMP else
	return translatedFlag;	

}












