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
openssl req -newkey rsa:512 -nodes -out commonname_new.req -keyout commonname_new.key -config rsa.config -days 3650

REM Sign the certificate with ROOT CA cert certificate
openssl x509 -req -days 3650 -md5 -extfile rsa.config -extensions v3_ca  -CAserial commonname_new.srl -CAcreateserial -in commonname_new.req -CA ..\root.cer -CAkey ..\root.key -out commonname_new.cer
openssl x509 -in commonname_new.cer -outform DER -out commonname_new.der

REM Convert the key to der format
REM openssl rsa -in commonname_new.key -outform DER -out commonname_der_new.key
openssl pkcs8 -in commonname_new.key -topk8 -nocrypt -outform DER -out commonname_der_new.key

del /Q commonname_new.srl