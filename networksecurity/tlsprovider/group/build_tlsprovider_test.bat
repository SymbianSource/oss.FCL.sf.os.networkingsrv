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
rem Compilations needed in order to compile tlsprovider test code
rem =========================================================

call cd %SECURITYSOURCEDIR%\tlsprovider\group
call bldmake bldfiles 
call abld test build winscw udeb



rem Additional compilations needed to run tlsprovider tests
rem =======================================================
 
call cd %SECURITYSOURCEDIR%\os\security\cryptomgmtlibs\securitytestfw\group
call bldmake bldfiles 
call abld test build winscw udeb

rem exports tls test certificates
call cd %SECURITYSOURCEDIR%\os\security\cryptomgmtlibs\securitytestfw\testcertificates\group
call bldmake bldfiles 
call abld test build winscw udeb

rem Needed for te_tlsprov test
call cd %SECURITYSOURCEDIR%\cryptotokens\group
call bldmake bldfiles 
call abld test build winscw udeb

rem needed for certman test compile (needs tfiletokens.lib and possibly more)
call cd %SECURITYSOURCEDIR%\filetokens\group
call bldmake bldfiles 
call abld test build winscw udeb

rem needed for swi test compile (needs rtestwrapper.h and possibly more)
call cd %SECURITYSOURCEDIR%\commonutils\group
call bldmake bldfiles 
call abld test build winscw udeb

rem needed for certman test compile (needs testutilclient and possibly more)
call cd %SECURITYSOURCEDIR%\swi\group
call bldmake bldfiles 
call abld test build winscw udeb

rem Certman needs to be compiled in order to run the 
rem te_tlsprov test.
call cd %SECURITYSOURCEDIR%\certman\group
call bldmake bldfiles 
call abld test build winscw udeb




 
