@echo off
rem Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
rem All rights reserved.
rem This component and the accompanying materials are made available
rem under the terms of "Eclipse Public License v1.0"
rem which accompanies this distribution, and is available
rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
rem
rem Initial Contributors:
rem Nokia Corporation - initial contribution.
rem
rem Contributors:
rem
rem Description:
rem Convert pppdump log into libpcap format suitable as input
rem to the VJ compressor test suite.
rem Only packets heading in one direction are converted--the
rem direction is specified by the first command-line parameter.
rem September 30, 2003
rem danfa
rem 
rem

if "%3"=="" echo Usage: %0 0/1 infile outfile
if "%3"=="" goto end
tethereal -R "frame.p2p_dir == %1 && (ppp.protocol == 0x21 || ppp.protocol == 0x2d || ppp.protocol == 0x2f)" -r %2 | perl trimdump.pl %2 %3 > tmptrim.bat
call tmptrim.bat
del tmptrim.bat
:end
