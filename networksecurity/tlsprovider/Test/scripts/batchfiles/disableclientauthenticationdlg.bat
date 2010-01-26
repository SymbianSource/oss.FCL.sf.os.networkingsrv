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
REM @ECHO Off

REM copy the Client Authentication disabled policy file 
copy \epoc32\winscw\c\ttlsprovider\policy\tlsproviderpolicy_disableclientauthdlg.ini \epoc32\release\winscw\udeb\z\resource\tlsproviderpolicy.ini
copy \epoc32\winscw\c\ttlsprovider\policy\tlsproviderpolicy_disableclientauthdlg.ini \epoc32\release\winscw\urel\z\resource\tlsproviderpolicy.ini

REM enable the file certstore
cd \
perl %SECURITYSOURCEDIR%\certman\twtlscert\scripts\batchfiles\certstorePlugins enable filecertstore.dll