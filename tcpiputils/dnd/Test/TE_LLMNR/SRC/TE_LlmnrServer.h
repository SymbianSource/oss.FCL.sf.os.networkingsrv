/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file TE_LlmnrServer.h
*/

#ifndef __TE_LLMNR_SERVER_H__
#define __TE_LLMNR_SERVER_H__

#include <testexecuteserverbase.h>
#include <es_sock.h>
#include <in_sock.h>


/**
* Represents a node of the network
*/
class TNodeInfo
    {
    public:
        TNodeInfo();
        
    public:    
        
        THostName   iHostName;  ///< Host Name
        TInetAddr   iAddr;      ///< Host IP address
        TInt        iCntTrials; ///< number of trials remaining during name resolution
        TBool       iAlive;     ///< ETrue if this host's name has been resolved. Otherwise it is supposed to be unreachable.
        TBool       iLocal;     ///< ETrue if this is the LLMNR entry on the local host and IP address belongs to one of the local network interfaces
        TBool       iIpUnique;  ///< ETrue if the IP address in unique in the container (see above)
        TBool       iClientConnected;
    };

typedef RArray<TNodeInfo>   RNodeInfoArray;
typedef RArray<TInetAddr>   RIPAddrArray;

/**
* Represents network as a set of nodes
*/
class TNetworkInfo
    {
    public:
        TNetworkInfo();
        ~TNetworkInfo();
        
        void Reset();
        
        /** @return Number of nodes in array*/
        TInt NodesCount() const {return iNodes.Count();} 
        
        /** append node to array. @return ETrue on success */
        TInt AppendNode(const TNodeInfo& aNode)  {return iNodes.Append(aNode);}
        
        /** access to the node by index */
        TNodeInfo& operator[] (TInt aNodeIndex) {return iNodes[aNodeIndex];}
        const TNodeInfo& operator[] (TInt aNodeIndex) const {return iNodes[aNodeIndex];}
        
        TInt    FindIP(const TInetAddr& aIPaddr) const;
        
        TInt    FindHostName(const TDesC16& aHostName) const;
        TInt    FindHostName(const TDesC8& aHostName) const;
        
        /** @return ETrue if the network info has been initialized */
        TBool   Initialized() const {return iInitialized;}
        
        /** set network info initialization flag */
        void    SetInitialized(TBool aFlag) {iInitialized = aFlag;}
        
        TInt NodesDataHash(TDes8& aHashBuf) const; 
        
    private:
        TNetworkInfo(const TNetworkInfo& );             //-- illegal copy constructor
        TNetworkInfo& operator=(const TNetworkInfo&);   //-- illegal assign operator
        
    protected:
        RNodeInfoArray  iNodes;         ///< Network nodes array.
        TBool           iInitialized;   ///< ETrue if LLMNR has been initialized successfully and Network information collected
        
    };


class CLlmnrTestServer : public CTestServer
    {
    public:
        static CLlmnrTestServer* NewL();
        virtual CTestStep* CreateTestStep(const TDesC& aStepName);
        
        TInt    RandomizeNum(TInt aMaxNum, TInt aDisp=0) const;
        
        
        //-- public data for using by test steps   
        RSocketServ         iSocketServer;
        RHostResolver       iHostResolver;
        
        TNetworkInfo        iNetworkInfo;///< network representation
        RIPAddrArray        iLocalAddrs; ///< array of IP addresses belonging to this host, bound to all interfaces
        
    private:
        mutable TInt64      iRndSeed;    ///< random seed, for random numbers generating
        
    };


//-- compares 2 IP addresses, for array search.
TBool IPAddrIsEqual(const TInetAddr& aFirst, const TInetAddr& aSecond);


#endif //__TE_LLMNR_SERVER_H__
