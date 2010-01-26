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

\epoc32\tools\netperf\bin\netperf.pl netperf_loopback.ini emulator generate

mkdir ..\scripts
grep -v UCCControlTE tefscripts_temp\testdata\scripts\netperf\loopback_udp_1500.script > ..\scripts\loopback_udp_1500.script
grep -v UCCControlTE tefscripts_temp\testdata\scripts\netperf\loopback_tcp_1500.script > ..\scripts\loopback_tcp_1500.script

mkdir ..\configs
copy tefscripts_temp\testdata\configs\netperf\netperf.xml ..\configs
copy tefscripts_temp\testdata\configs\netperf\netperfte.ini ..\configs
