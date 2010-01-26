#!perl
# Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description:
# Create a batch file to select certain records from a libpcap file.
# Called from cvtppplog.bat
# 
#

$, = ' ';		# set output field separator
$\ = " ";		# set output record separator

print 'editcap','-r',$ARGV[0],$ARGV[1],;
while (<STDIN>) {
    ($Fld1) = split(' ', $_, 9999);
    print $Fld1;
}
