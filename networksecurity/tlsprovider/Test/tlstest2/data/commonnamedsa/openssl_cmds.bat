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
REM Creates intermdiate cert of type dsa and signs with root CA cert

REM Create DSA Param file
openssl dsaparam -out commonnamedsa_new.param 512

REM Create DSA Key file
openssl gendsa -out commonnamedsa_new.key commonnamedsa_new.param

REM Generate CSR to request
openssl req -config dsa.config -new -key commonnamedsa_new.key -out commonnamedsa_new.req

REM Sign the certificate with ROOT CA cert certificate
openssl x509 -req -days 3650 -CAserial commonnamedsa_new.srl -CAcreateserial -extfile dsa.config -extensions v3_ca -in commonnamedsa_new.req -CA ..\root.cer -CAkey ..\root.key -out commonnamedsa_new.cer
openssl x509 -in commonnamedsa_new.cer -outform DER -out commonnamedsa_new.der

openssl pkcs8 -in commonnamedsa_new.key -topk8 -nocrypt -outform DER -out commonnamedsa_new.pk8

REM del /Q commonnamedsa_new.srl