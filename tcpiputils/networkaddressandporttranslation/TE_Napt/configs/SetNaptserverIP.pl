#!/usr/bin/perl -w
#
# SetEnv.pl
# Script file to build Environment for PlatTest depending on test configuration

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
# setenv.pl
# 
#

use Getopt::Long;
use Cwd;


my $word = "hi";
my $NaptTestServer;
my $NapttestServer;
sub getEpocroot
{
    my $epocroot = $ENV{EPOCROOT};
	
    return $epocroot;
}


sub getDrive
{	print $word;
    my $wd = cwd;
    my $drive;
	
    if($wd =~ /^([a-zA-Z]:)/) {
	$drive = $1;
    } else {
	# Perhaps we're on a machine that has no drives.
	$drive = "";
    }
	print $drive;
    return $drive;
}


sub Main()
	{
    my @outLines; 
    use Socket;
    use Sys::Hostname;
    my $host = hostname();
    my $addr;
    my $wd = cwd;
    my $drive;
	
    if($wd =~ /^([a-zA-Z]:)/) {
	$drive = $1;
	print $drive;
	}
   
     $addr = inet_ntoa(scalar gethostbyname($host || 'localhost'));
 
  
 
 	open( NAPTINI, "+<$drive\\epoc32\\release\\winscw\\udeb\\z\\testdata\\configs\\te_naptconfig.ini" );  

    seek( NAPTINI, 0, 0 );
	my $variable;
	 while( $variable=<NAPTINI>)
   		{
			my $NaptTestServer = "NaptTestServer=";
			my $sLine = $variable;			
			if($variable =~/NaptTestServer=/)
			{
			print $variable;
			my $len;
			my $mylocal=$NaptTestServer.$addr;
			$variable = $mylocal;
			print $variable;
			push( @outLines, $variable );
			push( @outLines, "\n" );

			}
			else 
			{
 			push( @outLines, $variable );
			push( @outLines, "\n" );
			}

	}
	seek(NAPTINI,0,0);
	print NAPTINI @outLines;
#print @outLines;
	close(NAPTINI);

 	open( NAPTARM, "+<$drive\\epoc32\\data\\Z\\testdata\\configs\\te_naptconfig.ini" );  
        seek( NAPTARM, 0, 0 );
	my $variableARM;
	 while( $variableARM=<NAPTARM>)
   		{
			my $NaptTestServerARM = "NaptTestServer=";
			my $sLineARM = $variableARM;			
			if($variableARM =~/NaptTestServer=/)
			{
			my $lenARM;
			my $mylocalARM=$NaptTestServerARM.$addr;
			$variableARM = $mylocalARM;
			print $variableARM;
			push( @outLines, $variableARM );
			push( @outLines, "\n" );

			}
			else 
			{
 			push( @outLines, $variableARM );
			push( @outLines, "\n" );
			}

	}
	seek(NAPTARM,0,0);
print NAPTARM @outLines;
#print @outLines;
close(NAPTARM);





}

Main();






