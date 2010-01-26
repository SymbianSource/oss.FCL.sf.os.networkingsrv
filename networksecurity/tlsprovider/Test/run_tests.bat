@rem
@rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
@rem All rights reserved.
@rem This component and the accompanying materials are made available
@rem under the terms of "Eclipse Public License v1.0"
@rem which accompanies this distribution, and is available
@rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
@rem
@rem Initial Contributors:
@rem Nokia Corporation - initial contribution.
@rem
@rem Contributors:
@rem
@rem Description:
@rem

@ECHO OFF
rem Runs this component's tests for udeb and cleans up afterwards, results stored in \logs\winscw\.  
rem This script requires ExecTimeOut.exe to be downloaded through Perforce from //PR/share/DABSRelease/buildscripts/ and the environment variable SECURITYSOURCEDIR to be set (though this should have been set prior to a build) to the root of the security source tree.
rem See test_launcher.pl -help for more information.
IF (%SECURITYSOURCEDIR%)==() (
ECHO ERROR: Environment variable SECURITYSOURCEDIR has not been set.  This should point to the root of your Security source tree.
) ELSE (
%SECURITYSOURCEDIR%\os\security\cryptomgmtlibs\securitytestfw\test\autotesting\test_launcher.pl -components tlsprovider -testreleases udeb -exectimeout \ExecTimeOut.exe -resultsoutput \logs\winscw
)
