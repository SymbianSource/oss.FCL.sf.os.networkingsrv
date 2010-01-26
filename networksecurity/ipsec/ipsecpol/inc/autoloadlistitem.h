// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IpsecPol - autoloadlistitem.h
//



/**
 @internalComponent
*/
#ifndef _AUTOLOADLISTITEM_H_
#define _AUTOLOADLISTITEM_H_

#include <e32base.h>

class CAutoloadListItem : public CBase  
    {
    public:
        //construct/destruct
        void ConstructL(const TUint32 aPreloadPolicyHandle, const TUint32 aPostloadPolicyHandle, const TUint32 aParentPolicyHandle);
        
        static CAutoloadListItem* NewL(const TUint32 aPreloadPolicyHandle, const TUint32 aPostloadPolicyHandle, const TUint32 aParentPolicyHandle);
        static CAutoloadListItem* NewLC(const TUint32 aPreloadPolicyHandle, const TUint32 aPostloadPolicyHandle, const TUint32 aParentPolicyHandle);
        
        TUint32 GetPreloadPolicyHandle();
        
        TUint32 GetPostloadPolicyHandle();
        
        TUint32 GetParentPolicyHandle();
        
        
        virtual	~CAutoloadListItem();
        
        
        
    private:
        CAutoloadListItem();
    private:
        
        TUint32 iPreloadHandle;
        TUint32 iPostloadHandle;        
        TUint32 iParentPolicyHandle;
        
        
    };

#endif // _AUTOLOADLISTITEM_H_

//End of file
