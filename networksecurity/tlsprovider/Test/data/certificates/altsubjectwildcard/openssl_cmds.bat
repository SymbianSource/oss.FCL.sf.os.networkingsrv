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
REM Creates intermdiate cert and signs with root CA cert

openssl req -newkey rsa:512 -nodes -out altsubjectwildcard_new.req -keyout altsubjectwildcard_new.key -config rsa.config -days 3650
openssl x509 -req -extfile rsa.config -extensions v3_ca -in altsubjectwildcard_new.req -out altsubjectwildcard_new.cer -CA ..\root.cer -CAkey ..\root.key -CAserial altsubjectwildcard_new.srl -CAcreateserial -days 3650
openssl x509 -in altsubjectwildcard_new.cer -outform DER -out altsubjectwildcard_new.der

del /Q altsubjectwildcard_new.srl