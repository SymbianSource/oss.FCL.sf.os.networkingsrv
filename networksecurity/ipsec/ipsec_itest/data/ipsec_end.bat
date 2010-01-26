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

md d:\ak_logs\
copy c:\logs\log.txt d:\ak_logs\
copy c:\logs\ced.log d:\ak_logs\
copy c:\logs\testresults\* d:\ak_logs\
copy c:\logs\vpnmanager\* d:\ak_logs\
copy c:\logs\kmd\* d:\ak_logs\
copy c:\logs\vpnconnagt d:\ak_logs\
copy c:\logs\tunnelnif d:\ak_logs\
copy c:\logs\ipspol d:\ak_logs\
copy c:\logs\tcpip6\*.* d:\ak_logs\

md e:\ak_logs\
copy c:\logs\log.txt e:\ak_logs\
copy c:\logs\ced.log e:\ak_logs\
copy c:\logs\testresults\* e:\ak_logs\
copy c:\logs\vpnmanager\* e:\ak_logs\
copy c:\logs\kmd\* e:\ak_logs\
copy c:\logs\vpnconnagt e:\ak_logs\
copy c:\logs\tunnelnif e:\ak_logs\
copy c:\logs\ipspol e:\ak_logs\
copy c:\logs\tcpip6\*.* e:\ak_logs\