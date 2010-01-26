#!/bin/perl -w
# Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
#

use warnings;
use strict;
use Getopt::Long;

my $EPOC_C="\\epoc32\\winscw\\c";
my $CERT="$EPOC_C\\tcertstore\\data";
my $KEY="$EPOC_C\\tkeystore\\data";
my $VERBOSE="";

&main;
exit(0);


sub CreateCertificate
{
	my ($issuer, $prefix, $i) = @_;
	my $im = sprintf "%s-%d", $prefix, $i;

	print "***************************************\n";
	print "Issuer: $issuer, Subject: $prefix-$i\n";

	my $cmd1 = "openssl req -newkey rsa:1024 -nodes -out $im.req -keyout $im.key -config $im.config -days 3650";
	my $cmd2 = "openssl x509 -req -in $im.req -out $im.cer -CA $issuer.cer -CAkey $issuer.key -CAserial $im.srl -CAcreateserial -days 3650 -extfile $im.config -extensions v3_ca";
	my $cmd3 = "openssl x509 -in $im.cer -outform DER -out $im-cert.der";
	my $cmd4 = "openssl pkcs8 -in $im.key -topk8 -nocrypt -outform DER -out $im-key.txt";

	if ( "$VERBOSE" eq "1" ) { print "$cmd1\n"; }
	system($cmd1);
	if ( "$VERBOSE" eq "1" ) { print "$cmd2\n"; }
	system($cmd2);
	if ( "$VERBOSE" eq "1" ) { print "$cmd3\n"; }
	system($cmd3);
	if ( "$VERBOSE" eq "1" ) { print "$cmd4\n"; }
	system($cmd4);
	print "***************************************\n\n\n";
}

sub main() {

GetOptions('key=s' => \$KEY,
		   'cert=s' => \$CERT,
		   'verbose=s' => \$VERBOSE);

print "Generating intermediate certificates...\n";
CreateCertificate("..\\root","IM",1);
CreateCertificate("..\\root","IM",2);
CreateCertificate("im-1","IM",11);
CreateCertificate("im-1","IM",12);
CreateCertificate("im-2","IM",21);
CreateCertificate("im-2","IM",22);
CreateCertificate("im-11","EU",0);

system("echo off");
system("del /Q *.srl *.req *.cer");
system("echo on");
}