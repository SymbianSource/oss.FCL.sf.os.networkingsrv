// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// hosts.h - name resolver hosts file parser module header
//

#ifndef _HOSTS_H_
#define _HOSTS_H_

/**
@file hosts.h
Hosts file handling
@internalComponent	Domain Name Resolver
*/
#include <e32std.h>		// TRequestStatus,. ..
#include <es_sock.h>	// TNameEntry, ..

_LIT(HOSTS_FILE, "hosts");


class RFs;
class RHostsFile
{
public:
	RHostsFile(RFs &aFs);
	virtual ~RHostsFile();

	TInt Open();
	void Close();

	TInt ReadLn(TPtrC& aLine);
	void Rewind();
	void Refresh();

	void GetByName(TNameRecord& aResult, TRequestStatus& err);
	void GetByAddress(TNameRecord& aResult, TRequestStatus& err);
	void Next(TNameRecord& aResult, TRequestStatus& err);

private:
	HBufC* iBuf;
	RFs &iFs;
	TLex iCharLex;
	TTime iRefreshed;
};

#endif
