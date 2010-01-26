// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "TlsProvTestStep.h"
#include <test/testexecutelog.h>



TVerdict CTlsProvTestActive::doTest2_0L( CTlsProvStep*  )
	{
	
	CTLSProvider* tlsProv = CTLSProvider::ConnectL();
	
	
	TBuf8<KTLSServerClientRandomLen> rnd1;
	rnd1.SetLength( KTLSServerClientRandomLen );
	rnd1.FillZ();
	TBuf8<KTLSServerClientRandomLen> rnd2;
	rnd2.SetLength( KTLSServerClientRandomLen );
	rnd2.FillZ();
	TBuf8<KTLSServerClientRandomLen> rnd3;
	rnd3.SetLength( KTLSServerClientRandomLen );
	rnd3.FillZ();
	
	tlsProv->GenerateRandom( rnd1);
	tlsProv->GenerateRandom( rnd2);
	tlsProv->GenerateRandom( rnd3);
	
	if( KTLSServerClientRandomLen != rnd1.Length() )
		{
		iLogInfo.Format( _L("	2.0:  CTLSProvider::GenerateRandom - wrong number of bytes generated for rnd1 = %d"), rnd1.Length() );
		return EFail;
		}
	if( KTLSServerClientRandomLen != rnd2.Length() )
		{
		iLogInfo.Format( _L("	2.0:  CTLSProvider::GenerateRandom - wrong number of bytes generated for rnd2 = %d"), rnd2.Length() );
		return EFail;
		}
	if( KTLSServerClientRandomLen != rnd3.Length() )
		{
		iLogInfo.Format( _L("	2.0:  CTLSProvider::GenerateRandom - wrong number of bytes generated for rnd3 = %d"), rnd3.Length() );
		return EFail;
		}
	
	if( (0 == rnd1.Compare( rnd2 ) ) ||
		(0 == rnd2.Compare( rnd3 ) ) ||
		(0 == rnd3.Compare( rnd1 ) ) )
		{
		iLogInfo.Copy( _L("	2.0:  CTLSProvider::GenerateRandom - not very random") );
		return EFail;
		}
		
	if ( (rnd1[0] == rnd1[1]) && (rnd1[1] == rnd1[2]) && (rnd1[2] == rnd1[3]) )
		{
		iLogInfo.Copy( _L("	2.0:  CTLSProvider::GenerateRandom - not very random") );
		return EFail;
		}
		
	if ( (rnd2[0] == rnd2[1]) && (rnd2[1] == rnd2[2]) && (rnd2[2] == rnd2[3]) )
		{
		iLogInfo.Copy( _L("	2.0:  CTLSProvider::GenerateRandom - not very random") );
		return EFail;
		}
		
	if ( (rnd3[0] == rnd3[1]) && (rnd3[1] == rnd3[2]) && (rnd3[2] == rnd3[3]) )
		{
		iLogInfo.Copy( _L("	2.0:  CTLSProvider::GenerateRandom - not very random") );
		return EFail;
		}
	
	iLogInfo.Copy( _L("	2.0:  OK") );	
	return EPass;
	
	} 


	
	
	
