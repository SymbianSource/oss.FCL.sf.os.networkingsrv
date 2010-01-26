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
REM This bat file is to be used to run openssl server form OSCP security test server.
REM The openssl server is used with tls_smoke tests

set cert_path = %SECURITYSOURCEDIR%/testframework/testcertificates/tlsprovider/test

call openssl.exe s_server -cipher ALL:eNULL -psk 30313233343536373839 -key %SECURITYSOURCEDIR%/tlstest2/openssl_server_setting/EU-0-key-b.der -certform der 
-cert  %SECURITYSOURCEDIR%/tlstest2/openssl_server_setting/EU-0-cert.der -cert2 %SECURITYSOURCEDIR%/tlstest2/openssl_server_setting/EU-0-cert.der 
 -key2 %SECURITYSOURCEDIR%/tlstest2/openssl_server_setting/EU-0-key-b.der -port 1666 -HTTP -dcert %SECURITYSOURCEDIR%/tlstest2/data/commonnamedsa/commonnamedsa.der -dcertform der -dkey commonnamedsa.key -servername www.symbianfoundation.org -servername_fatal
