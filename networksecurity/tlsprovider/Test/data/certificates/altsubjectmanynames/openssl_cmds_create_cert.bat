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

del /Q *new.req *new.cer *new.der *new.srl

openssl req -newkey rsa:512 -nodes -out altsubjectmanynames_new.req -keyout altsubjectmanynames_new.key -config rsa.config -days 3650
openssl x509 -req -in altsubjectmanynames_new.req -out altsubjectmanynames_new.cer -CA ..\root.cer -CAkey ..\root.key -CAserial altsubjectmanynames_new.srl -CAcreateserial -days 3650 -extfile rsa.config -extensions v3_ca
openssl x509 -in altsubjectmanynames_new.cer -outform DER -out altsubjectmanynames_new.der

del /Q altsubjectmanynames_new.srl