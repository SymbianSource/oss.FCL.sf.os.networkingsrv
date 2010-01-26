// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// PPP CFProtocol Provisioning Message class definitions
// 
//

/**
 @file
 @internalTechnology
*/


#ifndef PPPCONFIG_H
#define PPPCONFIG_H

#include <cdbcols.h>
#include <metadatabase.h>			// for TMDBElementId
#include <comms-infras/agentmessages.h>
#include <comms-infras/linkprovision.h>


namespace ESock
	{
	class CCommsDatIapView;
	}


class CPppLcpConfig : public CBase, public Meta::SMetaData
/**

@internalTechnology
@released Since 9.4
*/
	{
public:

enum TPppLinkMode
	{
	EPppLinkIsUnknown,
	EPppLinkIsAuto,
	EPppLinkIsClient,
	EPppLinkIsServer
	};

    enum
    {
    EUid = 0x1000022C,
    ETypeId = 1,
    };

	IMPORT_C static CPppLcpConfig* NewLC(ESock::CCommsDatIapView* aIapView);

    //getters
    inline TPppLinkMode GetIfServerMode() const;
    inline TInt GetIfCallbackType() const;
    inline const TDesC& GetIfParams() const;
    inline const TDesC& GetIfCallbackInfo() const;
    inline TBool GetIfCallbackEnabled() const;
    inline TBool GetEnableLcpExtensions() const;
    inline TBool GetEnableSwComp() const;
#if defined (_DEBUG)
    inline const TDesC& GetISPName() const;
    inline void  SetISPName(HBufC* aISPName);
#endif
    //setters
    inline void  SetIfServerMode(TPppLinkMode aIfServerMode);
    inline void  SetIfCallbackType(TInt aIfCallbackType);
    inline void  SetIfParams(HBufC* aIfParams);
    inline void  SetIfCallbackInfo(HBufC* aIfCallbackInfo);
    inline void  SetIfCallbackEnabled(TBool aIfCallbackEnabled);
    inline void  SetEnableLcpExtensions(TBool aEnableLcpExtensions);
    inline void  SetEnableSwComp(TBool aEnableSwComp);

public:
    EXPORT_DATA_VTABLE_AND_FN

protected:
	~CPppLcpConfig()
		{
#if defined (_DEBUG)
	    iISPName.Close();
#endif
	    }

	void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);

protected:
    TPppLinkMode  iIfServerMode;
    TInt  iIfCallbackType;
    RBuf  iIfParams;
    RBuf  iIfCallbackInfo;
    TUint iIfCallbackEnabled:1;
    TUint iEnableLcpExtensions:1;
    TUint iEnableSwComp:1;
#if defined (_DEBUG)
    RBuf  iISPName;
#endif
};

class CPppAuthConfig : public CBase, public Meta::SMetaData
/**

@internalTechnology
@released Since 9.4
*/
	{
public:
    enum
    {
    EUid = 0x1000022C,
    ETypeId = 2,
    };

    IMPORT_C static CPppAuthConfig* NewLC(ESock::CCommsDatIapView* aIapView);

    //getters
    inline TBool GetServiceEnableSwComp() const;
    //setters
    inline void  SetServiceEnableSwComp(TBool aServiceEnableSwComp);

public:
    EXPORT_DATA_VTABLE_AND_FN

protected:
	void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);

protected:
	TUint iServiceEnableSwComp:1;
};



class CPppProvisionInfo : public Meta::SMetaData
	{
public:
    enum
    {
    EUid = 0x1000022C,
    ETypeId = 3,
    };

    IMPORT_C ~CPppProvisionInfo();

	inline const TDesC8& ExcessData() const;
	inline TInt IsDialIn() const;
	inline TAny* NotificationData() const;

	inline TInt SetExcessData(const TDesC8& aData);
	inline void SetIsDialIn(TInt aValue);
	inline void SetNotificationData(TAny* aNotificationData);

private:
	// void* argument of last Notification(TAgentToNifEventType, void*) call from Agent
	TAny* iNotificationData;
	// result of GetExcessData() call on Agent
	RBuf8 iExcessData;
	// result of Notification(ENifToAgentEventTypeQueryIsDialIn)
	TInt iIsDialIn;

public:
	EXPORT_DATA_VTABLE_AND_FN
	};

class CPppTsyConfig : public Meta::SMetaData
	{
public:
    enum
    {
    EUid = 0x1000022C,
    ETypeId = 4,
    };

    IMPORT_C static CPppTsyConfig* NewLC(ESock::CCommsDatIapView* aIapView);
	
	inline const TName& TsyName() const ;
	inline void SetTsyName(const TName& aTsyName);
	
protected:
	void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);

	
private:
	TName iTsyName;

public:
	EXPORT_DATA_VTABLE_AND_FN
	};

#include <networking/pppconfig.inl>
#endif
// PPPCONFIG_H
