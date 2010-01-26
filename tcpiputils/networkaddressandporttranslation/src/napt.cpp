// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// implementation of UDP and TCP tranlation
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
#include "naptconfigtable.h"
#include <posthook.h>
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
#include "naptutil.h"
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#include <es_prot_internal.h>

const TInt KIapUnDefined = 0;


void Panic(TNAPTPanic aPanic)
	{
	User::Panic(_L("NAPTHook panic"), aPanic);
	}

_LIT(KProtocolNaptName, "napt");


//Timer class  Functions..
CNaptTimer* CNaptTimer::NewL(CProtocolNapt* aNaptProtocol)
	/**
	*
    * Create an instance of the timer
   	*
    */
     {
     CNaptTimer* timer = new(ELeave)CNaptTimer();
     CleanupStack::PushL(timer);
     timer->ConstructL();
     timer->iNaptProtocol = aNaptProtocol;
     timer->iMapMgrPtr = 	&(aNaptProtocol->iNaptMapMgr);
     CleanupStack::Pop(timer);
     return timer;
     }


void CNaptTimer::StartTimer()
	/**
	*
    * Start Timer & Add it Active Scheduler
    *
    **/
	{
	ASSERT(!IsAdded());
	CActiveScheduler::Add(this);
	//it should be in Micro Seconds..
	CTimer::After(iNaptTableScanInterval*1000000);
	}


void CNaptTimer::RunL()
	/**
	*	Timer is complete...
	*	Delete the TimedOut Entries from the TranslationTable.
	*	Check Any Packets are there in TranslationTable to be converted.
	*	Restart Timer if they are present by taking iIndexedCount
	*	Else Stop the timer
	*
	*	@internalTechnology
	**/
	{

	//delete timeout transactions..
	iMapMgrPtr->TimerComplete();

	//reset timer
	if (iNaptProtocol->iNaptMapMgr.GetIndexedPortCount())
		{
		CTimer::After(iNaptTableScanInterval*1000000);
		}
	else
		{
		Cancel();
		}
	}


void CNaptTimer::Cancel()
	/**
	*
	* Remove Timer from Active Scheduler
	*
	**/
	{
	CTimer::DoCancel();
	if (IsAdded())
		{
        Deque();
        }
	}

CNaptTimer::~CNaptTimer()
	/**
    *
    * Timer destructor
    *
    */
     {
     Cancel();
     }


/**
---------------------------------------------------------------------------------------
								CSapNapt
---------------------------------------------------------------------------------------
 
This class is derived from CServProviderBase.CSapNapt is the service class for sockets
loading NAPT.But here only one socket will be able to load protocol.If protocol once
loaded other socket cannot service protocol by opening socket.
 
 */

CSapNapt::CSapNapt()
	{
	iConfigInfo = NULL;
	}


TNaptConfigInfo* CSapNapt::GetConfigInfo()
/** 
 * This method is used to get the NAPT related configuration for this Service provider
 * access point.
 * @param None
 * return - TNaptConfigInfo*
**/
	{
	return iConfigInfo;
	}
	
void CSapNapt::SetConfigInfo(TNaptConfigInfo *aConfigInfo)	
/** 
 * This method is used to set the NAPT related configuration for this Service provider
 * access point.
 * @param None
 * return - TNaptConfigInfo*
**/
	{
	iConfigInfo = aConfigInfo;
	}

TInt CSapNapt::SetOption(TUint aLevel ,TUint aName ,const TDesC8& anOption)
/** 
 * This class is used to set interface index information to be used by NAPT.
 * level will contain Downlink information and name will contain private interface
 * index. This method will also bind the protocol to the TCP/IP stack if it has not been
 * done yet.
 * @param aDownlink-- Downlink Interface Index.
 * @param aPrivateInterface - Private interface Index.
 * return - KErrNone if no value is assigned else KErrPermissionDenied
**/

	{
	TInt err = KErrNone;
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION

	const TInterfaceLockInfo* info= reinterpret_cast<const TInterfaceLockInfo* >(anOption.Ptr());
	if(aLevel==KSolNapt &&  aName == KSoNaptSetup)
		{
		if(anOption.Length()!=sizeof(TInterfaceLockInfo))
			{
			//this means that Block is not filled properly
			return KErrArgument;
			}
		err = iConfigMgr->UpdateConfigL(info, this);
		}//check if option and size
	else
		{
		//options level or Name wrong
		return KErrArgument; 
		}
		
#else //SYMBIAN_NETWORKING_ADDRESS_PROVISION
		if(aLevel==KSolNapt)
		{
		switch(aName)
			{
			case KSoNaptSetup:
			case KSoNaptProvision:
				{
				LOG(Log::Printf(_L("CSapNapt::SetOption(): KSoNaptSetup ")));
				const TInterfaceLockInfo* info = reinterpret_cast<const TInterfaceLockInfo* >(anOption.Ptr());
				if(anOption.Length()!= sizeof(TInterfaceLockInfo))
					{
					//this means that Block is not filled properly
					return KErrArgument;
					}
				err = iConfigMgr->UpdateConfigL(info, this) ;
				}
				break;

			default:
				{
				LOG(Log::Printf(_L("//options level or Name wrong")));
				//options level or Name wrong
				return KErrArgument;
				}
			}//switch ends here
		}
	else
		{
		//options level or Name wrong
		return KErrArgument; 
		}
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
    // Bind the protocol to TCP/IP stack and this will result in actually attach of the hook
    DoBindToL(err);
    
    return err;
	}//end main function


void CSapNapt::DoBindToL(TInt aStatus)
/* This method is used to bind the protocol to TCP/IP stack once the client
 * configures NAPT hook. It checks if the protocol is not already bound in case it is
 * already bound the method does nothing and simply returns. Until and unless Napt is 
 * configured it's not actually bound to TCP/IP stack.
 * @param aStatus -- Status.
 * return - None
 *
 */
	{
	if ((aStatus == KErrNone) && (iProtocol->iBoundFlag == EFalse))
		{
		iProtocol->iBoundFlag = ETrue;
		iProtocol->BindToL(iProtocol->iProtBase);
		}
	}

TInt CSapNapt::SecurityCheck(MProvdSecurityChecker* aChecker)
	/**
	* Capability check for the NAPT sockets.
	*
	* NAPT sockets require the NetworkControl capability.
	*
	* @param aChecker The policy checker.
	* @return The result of the policy check.
	*/
	{
	//	This method is called when a SAP is created and when a socket is transferred between sessions.  The SAP is
	//required to check whether the originating client process has enough privileges to request services from the SAP.
	//The MProvdSecurityChecker class instance is used to perform security policy checks.  The SAP may choose
	//to perform a security policy check in its SecurityCheck(...) method, or it may choose to store the
	//MProvdSecurityChecker class instance argument and perform checking later (i.e. when subsequent
	//SAP methods are called). 
	_LIT_SECURITY_POLICY_C1(KPolicyNetworkControl, ECapabilityNetworkControl);
	return aChecker->CheckPolicy(KPolicyNetworkControl, "NAPT policy failed.");
	}




/*
-------------------------------------------------------------------------------------------------
									CProtocolNapt

-------------------------------------------------------------------------------------------------
	
	
				Protocol loading
				----------------
					
		ESOCK 			
 			\
 		      \ 	
 	       load \
                  \           load
 				CProtocolNapt-------->CProtocolNaptIn
 
   

CProtocolNapt will load CProtocolNaptIn in its constructor. CProtocolNapt will bind itself as forward hook and
will bind CProtocolNaptIn as an InboundHook using NetworkServices.
 
 
 */


CProtocolNapt* CProtocolNapt::NewL()
	{
	CProtocolNapt* self = new(ELeave) CProtocolNapt();
	CleanupStack::PushL(self);
	self->CreateL();
	CleanupStack::Pop();
	return self;
	}


void CProtocolNapt::CreateL()
/*
 *ConstructL part of two phase construction. Allocating memory to Inbound class aand passing 
 * reference to this.
*/
	{
    iNaptMapMgr.iLastPort = KNaptPort_HIGH;
    
    
    iConfigMgr = CNaptClientConfigMgr::NewL(*this);
        
    //Allocating memory for Inbound Hook. No need for cleanup stack here its
    //a leaving function, so everything should be traped above.
    iProtInbound = new(ELeave) CProtocolNaptIn(this);
   
   	//Create Timer Instance
   	if (!iTimer) 
		{
		iTimer = CNaptTimer::NewL(this);
		iNaptMapMgr.iTimerPtr = iTimer;
		}
    iBoundFlag = EFalse;
    }

CProtocolNapt::~CProtocolNapt()
/** 
 * Destructor.This is called when NAPT is destroyed. Protocol should unbind it self
 * from the Network Layer.
**/

	{
	CNaptIPPortMap *table=NULL;
	if (iTimer )
		{
		//running timer will be stopped in destructor
		delete iTimer;	
		}
	for (TInt index=0;index<KTranslationHashTableSize;index++)
		{
		TNaptTableIter naptTableIter(iNaptMapMgr.iIPPortMap[index]);
		naptTableIter.SetToFirst();
		while((table=naptTableIter++)!=NULL)
			{
			iNaptMapMgr.iIPPortMap[index].Remove(*table);
			delete table;
			}
		}
	// Protocol is being unloaded and hence clear all SAP references
	// We are not really needed to delete the objects as esock will take care of
	// deleting all the SAPs.	
    iSapList.Reset();		
    //Delete the client config manager.
    //This in turn destroys all the configurations stored in the list		
	delete iConfigMgr;
	
	if(iBoundFlag != EFalse)
		{
		//Unbind Hooks.iProtInbound is the inbound hook which should be unbind from
		//stack before unbinding main protocol.Should unbind using network service
		//which quite important.If unable to unbind then check for network Services.
		NetworkService()->Protocol()->Unbind((CProtocolBase*)iProtInbound,0);

		NetworkService()->Protocol()->Unbind(this,0);

		}
	
	if(iProtInbound!=NULL)
  		{
		delete iProtInbound;
		iProtInbound=NULL; //just safe step
		}
#ifdef __DEBUG
//UnMarking heap which is marked when protocol is loaded in Family.cpp
__UHEAP_MARKEND;
#endif

	}


void CProtocolNapt::Identify(TServerProtocolDesc* aProtocolDesc)const 
	{
	Describe(*aProtocolDesc);
	}



void CProtocolNapt::BindL(CProtocolBase* /*protocol*/, TUint /*id*/)
	{
	// We should not overwrite the existing esk files

	}



void CProtocolNapt::BindToL(CProtocolBase* aProtocol)
	/**
	* The protocol binds itself to TCP/IP stack. The bind has been delibrately deferred
	* here to facilitate opening of multiple sockets without actually loading the hook.
	* The hook will be actually loaded when someone configured the setup information in NAPT
	* 
	**/
	{
	if(iBoundFlag != EFalse)
	{
		const TUint id = (TUint)DoBindToL(aProtocol);
	}
	else
		iProtBase = aProtocol;
	}


void CProtocolNapt::NetworkAttachedL()
	/**
	* The TCP/IP stack has been attached to this plugin.
	*
	* The CProtocolPosthook impelements the basic BindL/BindToL and Unbind
	* processing. The NetworkAttached is called when the TCP/IP
	* is connected with this protocol module.
	*
	* This function installs a hook for forwarded packets. The function
	* ApplyL will be called for each received packet that enters the
	* forward path (before actual forwarding decision).
	*
	* Could also install any other hooks to pull packets.
	*/
	{
	//Read Timer Configuration Parameters
	ReadConfigurationFile();
			
	NetworkService()->BindL(this, BindForwardHook());
	
	//Binding Inbound hook as hook all. This will take packets from all the interfaces
	//and translate only thise which are required.This seperate class optimise RAM usage. 
	//and implemented to avoid usage of heavy calls like IsForMeAddress.
	NetworkService()->BindL((CProtocolBase*)iProtInbound, BindHookAll());

	iManager = NetworkService()->Interfacer();

	}


void CProtocolNapt::ReadConfigurationFile()
	/*
	* Timer values are stored in Timer Class.	
	* iNaptTableScanInterval value should be less than 2147 seconds, as it is an argument
	* to CTimer::After(TTimeIntervalMicroSeconds32).	
	*/
	{
	iTimer->iNaptTableScanInterval	= GetIniValue(NAPT_INI_TIMER,NAPT_INI_TABLESCANINTERVAL,KTableScanIntervalDefault,1,KTimerMaxSeconds);
	iTimer->iNaptTcpIdleTimeout		= GetIniValue(NAPT_INI_TIMER,NAPT_INI_TCPIDLETIMEOUT,KTcpIdleTimeOutDefault,1,KMaxTInt);
	iTimer->iNaptUdpIdleTimeout		= GetIniValue(NAPT_INI_TIMER,NAPT_INI_UDPIDLETIMEOUT,KUdpIdleTimeoutDefault,1,KMaxTInt);
	iTimer->iNaptIcmpIdleTimeout	= GetIniValue(NAPT_INI_TIMER,NAPT_INI_ICMPIDLETIMEOUT,KIcmpIdleTimeoutDefault,1,KMaxTInt);
	iTimer->iNaptTcpCloseTimeout	= GetIniValue(NAPT_INI_TIMER,NAPT_INI_TCPCLOSETIMEOUT,KTcpCloseTimeoutDefault,1,KMaxTInt);
	iTimer->iNaptTcpOpenTimeout		= GetIniValue(NAPT_INI_TIMER,NAPT_INI_TCPOPENTIMEOUT,KTcpOpenTimeoutDefault,1,KMaxTInt);
	}
	

TInt CProtocolNapt::GetIniValue(const TDesC& aSection, const TDesC& aName, TInt aDefault, TInt aMin, TInt aMax)
	/*
	* Timer values are read from the configuration file-napt.ini
	* If the configuration values are not present default values are stored
	*/
	{
	TInt value;
	CESockIniData* config = NULL;
	
	TRAP_IGNORE(config = CESockIniData::NewL(NAPT_INI_DATA));
    if (config==NULL || !config->FindVar(aSection, aName, value))
    	{
        value = aDefault;
    	}
     else if (value < aMin || value > aMax)
         {
         value = aDefault;
         }
    delete config;
    return value;	
	}
	

TInt CProtocolNapt::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
/*
 * This function is called for all the packets coming from interfce. This instance of 
 * ApplyL will be called for forwarding hook as well as inbound hook. Packets are 
 * translated for both inbound and forwarding packets. Scope is being set for packet 
 * destined to private interface.
 * @param aPacket
 * @param aInfo
*/
	{
	const TInetAddr& dest = aInfo.iDstAddr;
	const TInetAddr& src= aInfo.iSrcAddr;
	const TUint32 ifindex = aInfo.iInterfaceIndex;
	TInetAddr addr(aInfo.iSrcAddr);
	LOG(Log::Printf(_L("packet received from interface index=[%d] and source ip address is:%x"),ifindex,addr.Address()));
	
	iCurConfigInfo = iConfigMgr->FindConfig(ifindex);
	if(!iCurConfigInfo)
		{
		
		LOG(Log::Printf(_L("No configuration information for interface index=[%d]"),ifindex));
		return KIp6Hook_PASS;
		}
		
	//This Flag will make code to traverse extra check.If this is true then code 
	//will see if scope of packet and private interface match or not.
	TBool checkScopeFlag= EFalse;

	//This bit will be used in checking the scope of packet
	const TIp6Addr& packetScopeIp = dest.Ip6Address();
	TUint packetScope=KIapUnDefined;
	
	
	LOG(Log::Printf(_L("I belong to protocol family--version=[%d]"),aInfo.iVersion));

	//check for IP version.
	if(aInfo.iVersion==4)
		{
		LOG(Log::Printf(_L("\tInterface Index --Private IAP=[%d],Public IAP=[%d]"), iCurConfigInfo->iPrivateIap ,iCurConfigInfo->iPublicIap));
	
		const TIp6Addr& source = iCurConfigInfo->iPrivateIp.Ip6Address();

		//Find NET ID i.e. scope for Private Interface. explanation given below.
		iCurConfigInfo->iScopeSrc = iManager->RemoteScope(source ,iCurConfigInfo->iPrivateIap ,EScopeType_IAP);
		

	
		//giving iPublicGatewayIP information to tranlsation table manager
		iNaptMapMgr.iPublicGatewayIP=iCurConfigInfo->iPublicGatewayIP.Address();

		const TIp6Addr& downlink = iCurConfigInfo->iPublicGatewayIP.Ip6Address();
			
		//Take public interface scope.This will be use in Routing.IAP and Interface IP will 
		//select scope i.e. NET ID.
		/*
		Each interface is assigned 16 scope identifiers, one for each scope level (1..16). 
		The set of scope identifiers in each of the interfaces defines a virtual forest of trees (of depth 16),
		where the actual interfaces are the leaf nodes. 
		Each scope id at net level (16) defines it's own tree.
			
		scope=16	net=1                                 		net=1                
						  |                                           |
		scope=14	Global=1                                    Global=1
			|		  |                                          / \
			|		  |                                         |   |
			|		  |	                                        |   |
		scope=2		IAP = 1                                   IAP=2 IAP=3
				    /  \                                      	|     |                                      
		scope=1	 if=1   if=2                                   if=3  if=4  
		      		                                              
		  
		 */
		iCurConfigInfo->iScopedest = iManager->RemoteScope(downlink ,iCurConfigInfo->iPublicIap ,EScopeType_IAP);



		//Check if source IP of packet and destination IP is same. If yes then extra check
		//to find if scope of packet and private Interface match or not
		if((iCurConfigInfo->iPublicGatewayIP.Address())==(src.Address()))
			{
			checkScopeFlag=ETrue;
			}
		if(checkScopeFlag)
			{
			//Extract scope of the packet.This will help in deciding what type of packet neet translation
			//This fundamental will be quite helpful when subnet for private and public IP will be same
			packetScope =iManager->RemoteScope(packetScopeIp , aInfo.iInterfaceIndex ,EScopeType_IF);
			}
		
		//translate accoring to the private range and netmask specified.Extra check is added for the packets coming from 
		//private interface to be translated.This check is that packet coming from private scope with relevant
		//subnet should be translated.the check is if(packetScope!=iSrcScope) then dont translate.This will resolve issue
		//of private and public IP being same
		if(iCurConfigInfo->iPrivateIp.Match(aInfo.iSrcAddr,iCurConfigInfo->iNetMaskLength)&& !(dest.Match(iCurConfigInfo->iPrivateIp,iCurConfigInfo->iNetMaskLength)))
			{
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
			if((src.Address() != iCurConfigInfo->iProvisionedIp) || ! iCurConfigInfo->iUplinkAccess)
				{
				LOG(Log::Printf(_L("No forward translation %d:"), KIp6Hook_PASS));
				return KIp6Hook_PASS;
				}
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
			LOG(
			TBuf<70> tmpSrc;
			TBuf<70> tmpDst;
			TInetAddr::Cast(aInfo.iSrcAddr).OutputWithScope(tmpSrc);
			TInetAddr::Cast(aInfo.iDstAddr).OutputWithScope(tmpDst);
			LOG(Log::Printf(_L("\t Hi I am packet coming from private interface and need translation\n src=[%S] dst=[%S] proto=%d"), &tmpSrc, &tmpDst, aInfo.iProtocol));
			LOG(Log::Printf(_L("I am in Forwarding hook of NAPT")));	
			);

			//Intelligence maintained if public and private ip are same
		
			if(aInfo.iProtocol==KProtocolInetUdp ||aInfo.iProtocol== KProtocolInetTcp)
				{
				if(checkScopeFlag)
					{
					//No translatation will happen if Pubilc and private IP are same. Stack panics if
					//this happens. No need for translation just pass it.
					LOG(Log::Printf(_L("public IP is same as  client IP. Sorry no translation")));	

					//check if scope for packet matches scope of private interface ot not. If not then dont
					//manipulate
					if(packetScope!=iCurConfigInfo->iScopeSrc)
						{
						return KIp6Hook_PASS; 
						}
					}
				//Packet manipulation TCP/UDP
				ForwardManipulation(aPacket,aInfo);
				}//udp and tcp
		
			else if(aInfo.iProtocol == KProtocolInetIcmp)
				{
				if(checkScopeFlag)
					{
					//No translatation will happen if Pubilc and private IP are same. Stack panics if
					//this happens. No need for translation just pass it.
					LOG(Log::Printf(_L("public IP is same as  client IP. Sorry no translation")));	

					//check if scope for packet matches scope of private interface ot not. If not then dont
					//manipulate
					if(packetScope!=iCurConfigInfo->iScopeSrc)
						{
						return KIp6Hook_PASS; 
						}
					}
				else if(dest.Address()== iCurConfigInfo->iPublicGatewayIP.Address())
					{
					//This is quite special case PINGING Public Gateway wont require NAPT
					return KIp6Hook_PASS;
					}
					
				//Packet manipulation ICMP
				IcmpHandlerForward(aPacket,aInfo);
				} //ICMP
			} //end check for packets coming from private interface.
		else if(iCurConfigInfo->iPrivateIp.Match(aInfo.iDstAddr,iCurConfigInfo->iNetMaskLength)&& !(iCurConfigInfo->iPrivateIp.Match(aInfo.iSrcAddr,iCurConfigInfo->iNetMaskLength)))
			{
	 	   // Setting the scope to 0 will cause the stack to automatically fill in the correct scope itself
			TInetAddr::Cast(aInfo.iSrcAddr).SetScope(0);

			//Setting Network ID of private Interfae as a scope. Route will be searched according 
			//to the Destination Id which is set as the scope of private interface.
			TInetAddr::Cast(aInfo.iDstAddr).SetScope(iCurConfigInfo->iScopeSrc);
			}
		//Pass all packets which dont need translation
		else
			{
			return KIp6Hook_PASS;
			}
			
		} // Closing If of Version Check

	return KIp6Hook_PASS;
	} //Function close





void CProtocolNapt::ForwardManipulation(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
	{
		
	 LOG(Log::Printf(_L("CProtocolNapt::ForwardManipulation called for outgoing packets to global interface")));
	//retrieve information from aInfo. This information will be maintained in the transation 
	//table.
	TUint lSrcPort;
	TUint lDstPort;
  
  	//Declarations of IP to be stored in table.
 	const TUint32 sourceIP =(TInetAddr::Cast(aInfo.iSrcAddr)).Address();
    const TUint32 destinationIP=(TInetAddr::Cast(aInfo.iDstAddr)).Address();
   	const TUint32 targetIP= iCurConfigInfo->iPublicGatewayIP.Address();
	TUint translatedPort;
	
	TInet6Checksum<TInet6HeaderIP4> lIp(aPacket);

	//IP header Length ....used to access UDP header
	TInt lengthIP	= lIp.iHdr->HeaderLength();
   	
   	CNaptIPPortMap* table=NULL;

	if(aInfo.iProtocol==KProtocolInetUdp)
		{
		//UDP header 
		TInet6Packet<TInet6HeaderUDP> lUdp(aPacket,lengthIP);
		lSrcPort=lUdp.iHdr->SrcPort();
		lDstPort =lUdp.iHdr->DstPort();
	

		//Check if entry is needed or not.If result is KErrNone then record exsists in the table.
		TRAPD(ret,table=iNaptMapMgr.FindOrCreateNaptEntryL(KProtocolInetUdp ,sourceIP, destinationIP,lSrcPort ,lDstPort, iCurConfigInfo));
		
		if(table==NULL || (ret == KErrNoMemory))
			{
			LOG(Log::Printf(_L("I am UDP packet from private Interface and I need translation")));
			return;
			} 
	
		//Get translated port from the table.	
		TUint translatedPort = table->iTrPort;
		
		//Set translated port in the UDP header
		lUdp.iHdr->SetSrcPort(translatedPort);
		aInfo.iSrcAddr.SetPort(translatedPort);
		

		//Set source IP of packet as public IP (public refer to Gateway)
		lIp.iHdr->SetSrcAddr(targetIP);
		TInetAddr::Cast(aInfo.iSrcAddr).SetV4MappedAddress(targetIP);
		
		//Setting source scope i,e networkID as zero this will allow stack to to set source scope 
		//for packet.
		TInetAddr::Cast(aInfo.iSrcAddr).SetScope(0);

		
		//Setting Network ID of desired interface as a scope. Route will be searched according 
		//to the Destination Id which is set as the scope of desired interface.

		TInetAddr::Cast(aInfo.iDstAddr).SetScope(iCurConfigInfo->iScopedest);
               
		//Compute checksum zero for UDP
		lUdp.iHdr->SetChecksum(0);

		//Compute IP checksum
		lIp.ComputeChecksum();
		aInfo.iFlags &= ~KIpAddressVerified;
	
		}//Protocol Check UDP

	// Section for manipulating TCP packets.
	else if(aInfo.iProtocol==KProtocolInetTcp)
		{
		TInet6Checksum<TInet6HeaderTCP> lTcp(aPacket,lengthIP);
		lSrcPort = lTcp.iHdr->SrcPort();
		lDstPort = lTcp.iHdr->DstPort();

		//Finds  Translation Node,if already existing or creates  a new node filled with unique translated port number for 
		//translated IP for TCP/UDP/ICMP connections.  
		TRAPD(ret,table=iNaptMapMgr.FindOrCreateNaptEntryL(KProtocolInetTcp ,sourceIP, destinationIP,lSrcPort ,lDstPort, iCurConfigInfo));

		if(table==NULL || (ret==KErrNoMemory))
			{
			LOG(Log::Printf(_L("I am TCP packet from private Interface and I need translation")));
			return ;
			}
	

		//Get last translated port
		translatedPort = table->iTrPort;
        
		lTcp.iHdr->SetSrcPort(translatedPort);
		aInfo.iSrcAddr.SetPort(translatedPort);
		
		lIp.iHdr->SetSrcAddr(targetIP);
		TInetAddr::Cast(aInfo.iSrcAddr).SetV4MappedAddress(targetIP);

 	  // Setting the scope to 0 will cause the stack to automatically fill in the correct scope itself
		TInetAddr::Cast(aInfo.iSrcAddr).SetScope(0);
		
		//Setting Network ID of desired interface as a scope. Route will be searched according 
		//to the Destination Id which is set as the scope of desired interface.
		TInetAddr::Cast(aInfo.iDstAddr).SetScope(iCurConfigInfo->iScopedest);
		
		//this one is to avoid armv5 errors and calculate checksum right
		RMBufChain& payload = static_cast<RMBufChain&>(aPacket);
		
		//checksum calculations
   		lTcp.ComputeChecksum(payload,&aInfo,lengthIP);
		lIp.ComputeChecksum();
		
		aInfo.iFlags &= ~KIpAddressVerified;
		
	   		
	   	iNaptMapMgr.HandleTcpConnectionPhases(lTcp,table,KTcpClosePacketOUT);
		}//Protocol Check TCP

	}


	
void CProtocolNapt::Describe(TServerProtocolDesc& anEntry)
/* Protocol Description
 * @param anEntry
*/
	{
	anEntry.iName=KProtocolNaptName;
	anEntry.iAddrFamily=KAfInet;
	anEntry.iSockType=KSockDatagram;
	anEntry.iProtocol=KProtocolNAPT;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=KSIDatagram | KSIConnectionLess;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=KSocketNoSecurity;
	anEntry.iMessageSize=0xffff;
	anEntry.iServiceTypeInfo=0;
	anEntry.iNumSockets=KUnlimitedSockets;
	anEntry.iServiceTypeInfo=ESocketSupport | EInterface;

	}




/* 
----------------------------------------------------------------------------------------------------
							CProtocolNaptIn code
----------------------------------------------------------------------------------------------------
The following code is for inbound Hook.This hook will be bind for all the packets coming from the 
all the interfaces.But it will trigger for only packets whose entry is there in translation table.
*/



CProtocolNaptIn::CProtocolNaptIn(CProtocolNapt* aNapt)
/*
 * Consturctor
*/	
	
	{
	//giving pointer to CProtocolNapt for table manipulation. Now Pointer check 
	//is quite important here
	if(aNapt!=NULL)
		{
		iNapt=aNapt;
		iConfigMgr = iNapt->iConfigMgr;
		}
	else
		{
		Panic(ENAPTPanic_BadCall);

		}		
	}// end constructor
	

CProtocolNaptIn::~CProtocolNaptIn()
/*
 * Destructor
 */
	{
	}



TInt CProtocolNaptIn::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
/**
* Function for incoming packets coming from all the interface.
* Only translate packets whos entry is there in Tanslation table.The query is made to translation table
* for entry if not found then PASS packets else Tanslate packets and then pass.
* @param aPacket
* @param aInfo
**/
	{
	TInt ret=KIp6Hook_PASS;

	//only manipulate packets with IPv4 header other should not be tempered.
	if(aInfo.iVersion==4)
		{
		LOG(Log::Printf(_L("Hi I am packet .I am in Inbound hook of NAPT")));
		if(aInfo.iProtocol==KProtocolInetUdp || aInfo.iProtocol== KProtocolInetTcp)
			{
			//udp and tcp packets coming from all interfaces 
			ret=IncomingPacketManipulation(aPacket,aInfo);
			}
		else if(aInfo.iProtocol == KProtocolInetIcmp)
			{
			//ICMP packets.
			ret=IcmpHandler(aPacket , aInfo);
			}
		}
	return ret;
	}

TInt CProtocolNaptIn::IncomingPacketManipulation(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
/**
* Function for incoming packets coming from all the interface.
* Only translate packets whos entry is there in Tanslation table.The query is made to translation table
* for entry if not found then PASS packets else Tanslate packets and then pass.
* @param aPacket
* @param aInfo
**/
	{

	TUint lDstPort;
    const TUint32 srcIp = (TInetAddr::Cast(aInfo.iSrcAddr)).Address();
 
    // This flag states that packets have been translated and that KIp6Hook_DONE should be used as return value
	TBool translatedFlag = EFalse;
	
	CNaptIPPortMap* table= NULL;
	TNaptConfigInfo* info = NULL;

    TInet6Checksum<TInet6HeaderIP4> lIp(aPacket);
    TInt lengthIP = lIp.iHdr->HeaderLength();

	//if node exist and if destination IP is same as targetIp ( this is just to cross check)
	if(aInfo.iProtocol==KProtocolInetUdp)
		{
	 
	 	//UDP packet. Took port information in second line.
	  	TInet6Packet<TInet6HeaderUDP> lUdp(aPacket,lengthIP);
	    lDstPort= lUdp.iHdr->DstPort();
		
	   	//get translated table node
	   	table = iNapt->iNaptMapMgr.GetIPTranslationNode(lDstPort);
	   
	   	if (table)
	   		{
	   		//Take original IP and Port number from the table.
	  		TUint32 originalIP;

	  		originalIP=table->iSrcIpAddr;
			TUint orgPort = table->iSrcPort;
			info = table->iConfigInfo;
			
			if (iNapt->iNaptMapMgr.VerifySender(table,srcIp,lUdp.iHdr->SrcPort()))
				{
				LOG(Log::Printf(_L("Udp packet translated...see IP's below")));	

				lUdp.iHdr->SetDstPort(orgPort);
				aInfo.iDstAddr.SetPort(orgPort);

				lIp.iHdr->SetDstAddr(originalIP);
				TInetAddr::Cast(aInfo.iDstAddr).SetV4MappedAddress(originalIP);
			
				//Compute checksum zero for UDP
				lUdp.iHdr->SetChecksum(0);

				//Compute IP checksum
				lIp.ComputeChecksum();

				//This flag states that packets are translated and should be Done
				translatedFlag = ETrue;
				}//verify sender
	   		}//table
		}//udp
		
	   else	if(aInfo.iProtocol==KProtocolInetTcp)
   		{
 		//TCP packet.Took Port information in second line.
   		TInet6Checksum<TInet6HeaderTCP> lTcp(aPacket,lengthIP);
   		lDstPort =lTcp.iHdr->DstPort();
   	
   		//get translated table node
	   	table = iNapt->iNaptMapMgr.GetIPTranslationNode(lDstPort);
	   	if (table)
	   		{
	   		TUint32 originalIP;
	  		originalIP=table->iSrcIpAddr;
			TUint orgPort = table->iSrcPort;
			info = table->iConfigInfo;

			//check for match
			if (iNapt->iNaptMapMgr.VerifySender(table,srcIp,lTcp.iHdr->SrcPort()))
				{
				LOG(Log::Printf(_L("Tcp packet translated...see IP's below")));	

				//Change port information. Replace translated port by original port.
	   			lTcp.iHdr->SetDstPort(orgPort);
	   			aInfo.iDstAddr.SetPort(orgPort);
	   		
				//Change IP address.Replace translate IP by original IP.	   	 
	   			lIp.iHdr->SetDstAddr(originalIP);
	   			TInetAddr::Cast(aInfo.iDstAddr).SetV4MappedAddress(originalIP);
	   		
				//this one is to avoid armv5 errors and calculate checksum right
				RMBufChain& payload = static_cast<RMBufChain&>(aPacket);	   		
	   			
	   			//Compute checksum	
	   			lTcp.ComputeChecksum(payload,&aInfo,aInfo.iOffset);
	   			lIp.ComputeChecksum();
		   
				//This flag states that packets are translated and should be Done
				translatedFlag = ETrue;	 
				
				iNapt->iNaptMapMgr.HandleTcpConnectionPhases(lTcp,table,KTcpClosePacketIN);
	
	   				
	   			}//verify sender
	   		}//table
   		}//tcp
 	
	if(	translatedFlag)
		{
		aInfo.iFlags &= ~KIpAddressVerified;

		LOG(
			TBuf<70> tmpSrc;
			TBuf<70> tmpDst;
			TInetAddr::Cast(aInfo.iSrcAddr).OutputWithScope(tmpSrc);
			TInetAddr::Cast(aInfo.iDstAddr).OutputWithScope(tmpDst);
			LOG(Log::Printf(_L("\t I am translated packet values are after translation \n src=[%S] dst=[%S] proto=%d"), &tmpSrc, &tmpDst, aInfo.iProtocol));
			LOG(Log::Printf(_L("I am in Inbound hook of NAPT")));	
			);
	   // Setting the scope to 0 will cause the stack to automatically fill in the correct scope itself
		TInetAddr::Cast(aInfo.iSrcAddr).SetScope(0);

		//Setting Network ID of private Interfae as a scope. Route will be searched according 
		//to the Destination Id which is set as the scope of private interface.
		if(!iNapt->iCurConfigInfo)
			LOG(Log::Printf(_L("I am in Inbound hook of NAPT, but no Current Configuration for incoming packet")));	
		TInetAddr::Cast(aInfo.iDstAddr).SetScope(info->iScopeSrc);

		return KIp6Hook_DONE;
		}
	return KIp6Hook_PASS;
	}



CServProviderBase* CProtocolNapt::NewSAPL(TUint aProtocol)
/** 
 * Service Access point for the Protocol. Service Access Point supports sockets and allow them 
 * to manipulate protocol. aProtocol is the instance of protocol for which SAP is required.
**/

	{
    //avoid warning.
    (void)aProtocol;
    
    CSapNapt *nsap = NULL;

    nsap = new(ELeave) CSapNapt;
	nsap->iProtocol=this;
	nsap->iConfigMgr = this->iConfigMgr;
	iSapList.AppendL(nsap);		
	return nsap;
	}


void CProtocolNapt::CancelSap(CSapNapt *aSap)
/** 
 * This method clears the instance of SAP contained in the SAP table of the protocol.
 * @param aSap - intance to be set to NULL
 * @return None
 * 
**/

	{
	LOG(Log::Printf(_L("CProtocolNapt::CancelSap...Deleting SAP:%d"), aSap));
	TInt index = iSapList.Find(aSap);
	
	//Delete the config and set the reference of the SAP being deleted to NULL
	if(index != KErrNotFound)
		{
	    iSapList[index]->iConfigMgr->DeleteConfig(aSap->iConfigInfo);
		iSapList[index] = NULL;
		iSapList.Compress();		
		}
	}
	
/*
------------------------------------------------------------------------------------------

                    	SAP UNUSED FUNTION SECTION
------------------------------------------------------------------------------------------

SAP definion which are not being used.These functions are not doing anything instead they are 
returning nothing from it.

*/

void CSapNapt::Ioctl(TUint /*level*/,TUint /*name*/,TDes8*/*anOption*/)
	{}

void CSapNapt::Start()
	{}

void CSapNapt::Shutdown(TCloseType /*option*/)
	{}

void CSapNapt::LocalName(TSockAddr& /*anAddr*/) const
	{}

TInt CSapNapt::SetLocalName(TSockAddr& /*anAddr*/)
	{
	return KErrNotSupported;
	}
	
void CSapNapt::RemName(TSockAddr& /*anAddr*/) const 
	{}

TInt CSapNapt::SetRemName(TSockAddr& /*anAddr*/) 
	{ 
	return KErrNotSupported;
	}

TInt CSapNapt::GetOption(TUint aLevel, TUint aName, TDes8& aOption)const
/** 
 * This implements GetOption method for Napt specific service provider.
 * @param aLevel
 * @param aName
 * @param anOption
 * @return KErrNone in case of success
**/
	{
	TInt err = KErrNone;

 	const TUplinkInfo* info= reinterpret_cast<const TUplinkInfo* >(aOption.Ptr());
	if(aLevel == KSolNapt &&  aName == KSoNaptUplink)
		{
		if(aOption.Length()!=sizeof(TUplinkInfo))
			{
			//this means that Block is not filled properly
			return KErrArgument;
			}
		TNaptConfigInfo* conf = iConfigMgr->FindConfig(info->iPrivateIap, EFalse);
		if(conf != NULL)
			{
			TUplinkInfo upinfo;
			upinfo.iPrivateIap = info->iPrivateIap;
			upinfo.iPublicIap = conf->iPublicIap;
			aOption.SetLength(sizeof(TUplinkInfo));
			aOption.Copy((TUint8 *)&upinfo, sizeof(TUplinkInfo));
			 
			}
		else
			err = KErrNotFound;
		}//For Uplink Information Option
	else
		{
		//Wrong socket option name /level
		err = KErrArgument;
		}
	 return err;
	}

void CSapNapt::ActiveOpen()
 	 {}
 	 
TInt CSapNapt::PassiveOpen(TUint /*aQueSize*/)
	 {
	 return KErrNotSupported;
	 }
	 
void CSapNapt::Shutdown(TCloseType /*option*/,const TDesC8& /*aDisconnectionData*/)
	 {}
	 
void CSapNapt::AutoBind()
	 {}

TInt CSapNapt::PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/)
	{
	return KErrNotSupported;
	}

void CSapNapt::ActiveOpen(const TDesC8& /*aConnectionData*/)
	{}

void CSapNapt::CancelIoctl(TUint /*aLevel*/,TUint /*aName*/)
	{}

CSapNapt::~CSapNapt()
	{
	//Cancel configuration in sap.This will reset all bits (public & private IAP and IP)
	iProtocol->CancelSap(this);
	}
	


