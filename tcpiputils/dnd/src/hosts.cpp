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
// hosts.cpp - name resolver hosts file parser module
//

#include <f32file.h>	// RFs
#include <in_sock.h>	// TInetAddr
#include "hosts.h"	// this

RHostsFile::RHostsFile(RFs &aFs):
	iBuf(NULL), iFs(aFs), iRefreshed(0)
{
}

RHostsFile::~RHostsFile()
{
	if (iBuf)
	{
		delete iBuf;
		iBuf = NULL;
	}
}

//
//
//

TInt RHostsFile::Open()
{
	if (iBuf)
	{
		return KErrNone;
	}

	const TDesC &file_name = HOSTS_FILE;
	TInt err;

	RFile file;
	err = file.Open(iFs, file_name, EFileStreamText | EFileRead | EFileShareAny);

	if (err != KErrNone)
	{
		return err;
	}

	TInt size;
	err = file.Size(size);
	
	if (err != KErrNone)
	{
		file.Close();
		return err;
	}

	iBuf = HBufC::NewMax(size);

	if (iBuf == NULL)
	{
		file.Close();
		return KErrNoMemory;
	}

	// Unicode fix.

	HBufC8 *tempBuf = HBufC8::NewMax(size);

	if (tempBuf == NULL)
		{
		delete iBuf;
		iBuf = NULL;
		file.Close();
		return KErrNoMemory;
		}

	TPtr8 tempPtr(tempBuf->Des());
	err = file.Read(tempPtr);

	if (err != KErrNone)
		{
		delete iBuf;
		delete tempBuf;
		iBuf = NULL;
		file.Close();
		return err;
		}

	TPtr p(iBuf->Des());
	p.Copy(tempPtr);
	delete tempBuf;

	iCharLex = p;
	file.Close();

	return KErrNone;
}

void RHostsFile::Close()
{
	delete iBuf;
	iBuf = NULL;
}

//
//
//

TInt RHostsFile::ReadLn(TPtrC& aLine)
{
	do
	{
		iCharLex.Mark();

		if (iCharLex.Eos())
		{
			return KErrEof;
		}
		
		TChar ch = 0;

		while (!iCharLex.Eos() && (ch = iCharLex.Peek(), ch != '\n' && ch != '\r' && ch != '#' ))
		{
			iCharLex.Inc();
		}

		aLine.Set(iCharLex.MarkedToken());

		if (ch == '#')
		{
			while (!iCharLex.Eos() && (ch = iCharLex.Peek(), ch != '\n' && ch != '\r'))
			{
				iCharLex.Inc();
			}
		}
		
		while (!iCharLex.Eos() && (ch = iCharLex.Peek(), ch == '\n' || ch == '\r'))
		{
			iCharLex.Inc();
		}
	}
	while (!aLine.Length());
	
	return KErrNone;
}


void RHostsFile::Rewind()
{
	if (iBuf)
		{
		TPtr p(iBuf->Des());
		iCharLex = p;
		}
}

void RHostsFile::Refresh()
	{
	const TDesC &file_name = HOSTS_FILE;
	TInt err;

	TTime modified;
	err = iFs.Modified(file_name, modified);
	
	if (err == KErrNone)
		{
		if (iRefreshed < modified)
			{
			Close();
			Open();
			iRefreshed = modified;
			return;
			}
		}
	Rewind();
	}

//
//
//

void RHostsFile::GetByName(TNameRecord& aResult, TRequestStatus& err)
{
	Refresh();
	TInt count = 0;	// ...to select correct result for Next()

	for (;;)
	{
		TPtrC line;
		err = ReadLn(line);
		
		if (err != KErrNone)
		{
			if (err == KErrEof)
			{
				err = KErrNotFound;
			}
			return;
		}
		
		TLex wordLex(line);
		TInetAddr addr;
		TPtrC word;

		word.Set(wordLex.NextToken());
		err = addr.Input(word);

		if (err != KErrNone)
		{
			return;
		}

		TPtrC name;
		name.Set(wordLex.NextToken());

		if (!name.Length())
		{
			err = KErrBadName;
			return;
		}

		err = KErrNotFound;
		
		if (name.CompareF(aResult.iName) == 0 && ++count > aResult.iFlags)
		{
			err = KErrNone;
		}
		else
		{
			TPtrC alias;

			for (
				alias.Set(wordLex.NextToken());
				alias.Length();
				alias.Set(wordLex.NextToken())
			){
				if (alias.CompareF(aResult.iName) == 0 && ++count > aResult.iFlags)
				{
					err = KErrNone;
					break;
				}
			}
		}

		if (err == KErrNone)
		{
			aResult.iAddr = addr;
			aResult.iName = name;
			return;
		}
	}
}

void RHostsFile::GetByAddress(TNameRecord& aResult, TRequestStatus& err)
{
#if 1
	const TInt ipv6 = (aResult.iAddr.Family() == KAfInet6);
	const TUint32 scope = TInetAddr::Cast(aResult.iAddr).Scope();
#endif
	Refresh();

	TInt count = 0; // ...to select correct result for Next() 
	for (;;)
	{
		TPtrC line;
		err = ReadLn(line);
		
		if (err != KErrNone)
		{
			if (err == KErrEof)
			{
				err = KErrNotFound;
			}
			return;
		}
		
		TLex wordLex(line);
		TInetAddr addr;

		TPtrC word;
		word.Set(wordLex.NextToken());
		err = addr.Input(word);

		if (err != KErrNone)
		{
			return;
		}
#if 0
		if (!addr.CmpAddr(aResult().iAddr))
		{
			continue;
		}
#else
		// Plain IPv4 addresses in hosts will match the same
		// address in IPv4mapped format. However, IPv4 mapped
		// in hosts file will not match plain IPv4, if queried
		if (ipv6 && addr.Family() == KAfInet)
			addr.ConvertToV4Mapped();
		// ... if the hosts file specifies the scope,
		// require it to match too.
		if (!addr.Match(aResult.iAddr) ||
			(addr.Scope() != 0 && addr.Scope() != scope))
		{
			continue;
		}
#endif
		TPtrC name;
		name.Set(wordLex.NextToken());
		while (name.Length() > 0)
			{
			if (++count > aResult.iFlags)
				{
				aResult.iAddr = addr;
				aResult.iName = name;
				err = KErrNone;
				return;
				}
			name.Set(wordLex.NextToken());
			}
		err = count == 0 ? KErrBadName : KErrNotFound;
		return;
	}
}

void RHostsFile::Next(TNameRecord& /* aResult */, TRequestStatus& err)
{
#if 0
	TPtrC alias;
	alias.Set(iWordLex.NextToken());

	if (!alias.Length())
	{
		err = KErrNotFound;
		return;
	}

	aResult.iName = alias;
	err = KErrNone;
#else
	// Next() is not used now
	err = KErrNotFound;
#endif
}

