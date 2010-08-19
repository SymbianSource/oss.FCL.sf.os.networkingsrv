// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contain the implementation of the class for generic CSD agent test
// 
//
#include <f32file.h>
#include "CsdAgentTestSteps.h"

CsdAgentTestStep::CsdAgentTestStep()
	{
		
	}
	
CsdAgentTestStep::~CsdAgentTestStep()
	{
		
	}	
	
enum TVerdict CsdAgentTestStep::doTestStepPreambleL()
	{
		enum TVerdict result = EPass;
		
		RSocketServ ss;
		ss.Connect();
		CleanupClosePushL(ss);
		
		if(KErrNone == WaitForAllInterfacesToCloseL(ss))
			result = EPass;
		else
			result = EFail;
		
		ss.Close();
		CleanupStack::Pop();
		return result;				
	}	
	
TInt CsdAgentTestStep::WaitForAllInterfacesToCloseL(RSocketServ& ss)
	{
			TInt err;
			TUint numOfConnections;
			TUint count =0;
			
			RConnection conn;
			
			err = OpenConnection(conn,ss);
			TESTEL(KErrNone == err,err);
			CleanupClosePushL(conn);
			
			err = EnumerateConnections(conn,numOfConnections);
			TESTEL(KErrNone == err,err);
			
			while((0 !=numOfConnections) && (count <60))
			{
				count++;
				User::After(1000000); 
				err = EnumerateConnections(conn,numOfConnections);
			}
			
			CloseConnection(conn);
			
			CleanupStack::Pop();
				
			if(numOfConnections !=0)
				{
					return KErrTimedOut;
				}
			return KErrNone;
		}			
	
	TInt CsdAgentTestStep::OpenConnection(RConnection& conn, RSocketServ& ss)
/*
 * Open the connection using the socket server too
 * @param conn the connection to open
 * @param ss the socket server within which the connection is to be opened
 * @return system wide error code
 */
{
	return (conn.Open(ss));
}

TInt CsdAgentTestStep::EnumerateConnections(RConnection& conn, TUint& num)
/*
 * Read how many connections (==interfaces?) exist at the moment
 * @param conn - to be used to read the count
 * @param num - on completion holds the number of connections
 * @return system wide error code
 */
{
	return (conn.EnumerateConnections(num));
}

void CsdAgentTestStep::CloseConnection(RConnection& conn)
/*
 * Close a connection
 * @param conn the connection to close
 * @return system wide error code
 */
{
	conn.Close();
}
