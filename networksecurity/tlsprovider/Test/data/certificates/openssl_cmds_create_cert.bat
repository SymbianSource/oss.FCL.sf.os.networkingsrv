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

openssl req -newkey rsa:512 -nodes -out root_new.req -keyout root_new.key -config rsa.config -days 3650

openssl req -config rsa.config -new -x509 -days 3650 -md5 -key root_new.key -out root_new.cer

openssl x509 -in root_new.cer -outform der -out root_new.der

openssl rsa -in root_new.key -outform DER -out root_key_new.txt