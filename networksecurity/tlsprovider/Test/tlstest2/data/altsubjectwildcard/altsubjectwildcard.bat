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
@echo off

del /Q *.req *.cer *.der *.srl

openssl req -newkey rsa:512 -nodes -out altsubjectwildcard.req -keyout altsubjectwildcard.key -config rsa.config -days 3650
openssl x509 -req -in altsubjectwildcard.req -out altsubjectwildcard.cer -CA ..\root.cer -CAkey ..\root.key -CAserial altsubjectwildcard.srl -CAcreateserial -days 3650 -extfile rsa.config -extensions v3_ca
openssl x509 -in altsubjectwildcard.cer -outform DER -out altsubjectwildcard.der

del /Q altsubjectwildcard.srl

copy altsubjectwildcard.der \EPOC32\RELEASE\WINSCW\UDEB\Z\tlstest2\certificates
