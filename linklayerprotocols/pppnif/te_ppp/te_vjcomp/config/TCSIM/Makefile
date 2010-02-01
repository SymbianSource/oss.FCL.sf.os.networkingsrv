# Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the License "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description:
# Makefile for generating Van Jacobson test data using tcsim
# For GNU make
# 
#

all: $(patsubst %.tcsim,%.pcap,$(wildcard *.tcsim))

%.pcap: %.tcsim
	tcsim $< | ./tcsim2pcap > $@

clean:
	-rm $(patsubst %.tcsim,%.pcap,$(wildcard *.tcsim))
