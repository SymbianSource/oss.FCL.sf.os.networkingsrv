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

openssl req -newkey rsa:512 -nodes -out commonnamewildcard_new.req -keyout commonnamewildcard_new.key -config rsa.config -days 3650
openssl x509 -req -extfile rsa.config -extensions v3_ca -in commonnamewildcard_new.req -out commonnamewildcard_new.cer -CA ..\root.cer -CAkey ..\root.key -CAserial commonnamewildcard_new.srl -CAcreateserial -days 3650
openssl x509 -in commonnamewildcard_new.cer -outform DER -out commonnamewildcard_new.der
