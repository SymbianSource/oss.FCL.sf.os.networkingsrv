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

md c:\logs\
md c:\logs\testresults
md c:\logs\vpnmanager
md c:\logs\kmd
md c:\logs\vpnconnagt
md c:\logs\tunnelnif
md c:\logs\ipspol

copy z:\testdata\configs\ts_ipsec\comsdbg.ini c:\logs\

start scheduletest z:\testdata\scripts\ts_ipsec