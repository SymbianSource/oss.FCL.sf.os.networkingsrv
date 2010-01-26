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
//

#include "tlscacheutils.h"

void TlsCacheUtil::BuildCacheFileNameL(TDes& aFileName, TUid aSid, RFs& aFs)
	{
	TDriveUnit drive(SystemDrive());
	TDriveName driveName(drive.Name());
	aFileName.Append(driveName);
	
	TPath path;
	User::LeaveIfError(aFs.PrivatePath(path));
	aFileName.Append(path);
	
	_LIT(KCacheExtension, ".cce");
	aFileName.AppendNum(aSid.iUid, EHex);
	aFileName.Append(KCacheExtension);
	}
