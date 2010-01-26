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

openssl req -newkey rsa:1024 -nodes -out EKUCertificateRequest_new.req -keyout EKUPrivateKey_new.key -config rsa.config -days 9999

openssl x509 -req -days 9999 -md5 -extfile rsa.config -CAserial EKUAbsent.srl -CAcreateserial -in EKUCertificateRequest_new.req -CA ..\root.cer -CAkey ..\root.key -out EKUAbsent_new.pem
openssl x509 -req -days 9999 -md5 -extfile rsa.config -extensions Signing_Extensions_Failure -CAserial InvalidEKUPresent.srl -CAcreateserial -in EKUCertificateRequest_new.req -CA ..\root.cer -CAkey ..\root.key -out InvalidEKUPresent_new.pem
openssl x509 -req -days 9999 -md5 -extfile rsa.config -extensions Signing_Extensions_Success -CAserial ValidEKUPresent.srl -CAcreateserial -in EKUCertificateRequest_new.req -CA ..\root.cer -CAkey ..\root.key -out ValidEKUPresent_new.pem


openssl x509 -in EKUAbsent_new.pem -outform DER -out EKUAbsent_new.der
openssl x509 -in InvalidEKUPresent_new.pem -outform DER -out InvalidEKUPresent_new.der
openssl x509 -in ValidEKUPresent_new.pem -outform DER -out ValidEKUPresent_new.der

REM To show cert in human readable format
REM openssl x509 -in EKUAbsent.pem -inform pem -text -out EKUAbsent.txt

echo off
del /Q *.srl
echo on