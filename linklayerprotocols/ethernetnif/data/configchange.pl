#!/usr/bin/perl -w
# Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# CHANGECONFIG.PL
# This script is designed to provide a working ethernet setup
# for EKA1 or EKA2.  It works by running Netcards.exe
# then taking the output from Netcards (ethernet.ini)
# and appending the ethernet settings to epoc.ini for EKA2
# or creating ethermac.dat and etherdriver.dat for EKA1
# The script can
# - create a working ethernet setup with "ethernet" config option
# - restore previous setup with "restore" config option
# Please note: The script assumes that target wins is running on EKA1
# 
#

use Getopt::Long;
use Cwd;

my $config="default";
my $target="default";
my $kernel="default";
my $showhelp='0';

my $etherNif;
my $etherMac;
my $etherSpeed;

# this variable is passed as an argument to netcards.exe
# specifies which interface to use in case of multiple interfaces

my $interface=0;

$/ = "\n";

GetOptions('config=s' => \$config, 'kernel=s' => \$kernel, 'target=s' => \$target, 'variant=s' => \$variant, 'help' => \$showhelp, 'interface:i' => \$interface);

$config = lc( $config );
$kernel = lc( $kernel );
$target = lc( $target );
$variant = lc( $variant );


if( $showhelp || ( $kernel ne "eka1" && $kernel ne "eka2" ) ||
 ( $target ne "wins" && $target ne "winscw" ) ||
 ( $config ne "ethernetwithcommdb" && $config ne "restorecommdb" && $config ne "ethernetnocommdb" ) ||
 ( $variant ne "udeb" && $variant ne "urel" ) )
	{
	print "Command usage: configchange --config [ethernetWithCommDB|restoreCommDB|ethernetNoCommDB] --kernel [EKA1|EKA2] --target [wins|winscw] --variant [UDEB|UREL]\n";
	print "\t\t--config   \tSelect required configuration\n";
	print "\t\t--kernel   \tKernel variant\n";
	print "\t\t--target   \tTarget emulator\n";
	print "\t\t--variant  \tBuild type\n";
	print "\t\t--interface  \tInterface Number\n";
	print "\t\t--help   \tThis text\n";
	exit 0;
	}

my $epocroot = &getEpocroot;
my $drive = &getDrive;

if( $config ne "restorecommdb" )
	{
	# must be creating some ethernet setup
	if( $config eq "ethernetwithcommdb" )
		{
		# write an appropriate commDB setup
		print( "Setting up CommDB for ethernet\n" );
		system( "$drive\\$epocroot\\epoc32\\release\\$target\\$variant\\ceddump" ) == 0 
				or die "Error running ceddump!\n";
		system( "move $drive\\$epocroot\\epoc32\\$target\\c\\cedout.cfg $drive\\$epocroot\\epoc32\\$target\\c\\nonethernetced.cfg" ) == 0
				or die "Failed to rename cedout.cfg!\n";
		system( "$drive\\$epocroot\\epoc32\\release\\$target\\$variant\\ced -dtextshell -- -i c:\\EthernetCed.xml") == 0
				or die "Error running ced!\n";
  		}
  	print( "Running Netcards to obtain adapter info\n" );
	system( "$drive\\$epocroot\\epoc32\\tools\\netcards $interface" ) == 0
			or die "Error running netcards!\n";

	open ( INFILE, "ethernet.ini" ) or die "Can't find netcards output file, ethernet.ini!\n";					 # get from current directory where netcards wrote it to

	if ($kernel eq "eka1")
		{
	   	open ( ETHERMAC, ">$drive\\$epocroot\\epoc32\\$target\\c\\system\\data\\ethermac.dat" ) or die "Can't open $drive\\$epocroot\\epoc32\\$target\\c\\system\\data\\ethermac.dat!\n";
	   	open ( ETHERDRV, ">$drive\\$epocroot\\epoc32\\$target\\c\\system\\data\\etherdriver.dat" ) or die "Can't open $drive\\$epocroot\\epoc32\\$target\\c\\system\\data\\etherdriver.dat!\n";
	   	}

	while( <INFILE> )
		{
		chomp;
	   	my $sLine = $_;

		if( $sLine =~ /ETHER_NIF=(.*)/i )
			{
			#print "Matched ETHER_NIF\n";
			if ($kernel eq "eka1")
				{
				print ETHERDRV "$1";
    			}
			else
				{
				$etherNif = $1;
				}
			}
		elsif( $sLine =~ /ETHER_MAC=(.*)/i )
			{
		#print "Matched ETHER_MAC\n";
			if ($kernel eq "eka1")
				{
				print ETHERMAC "$1";
    			}
			else
				{
				$etherMac = $1;
    			}
			}
		elsif( $sLine =~ /ETHER_SPEED=(.*)/i )
			{
			#print "Matched ETHER_SPEED\n";
			if ($kernel eq "eka2")
				{
				$etherSpeed = $1;
    			}
			}
		#print "line: $sLine\n";
		}
	if ($kernel eq "eka1")
		{
		close( ETHERDRV );
		close( ETHERMAC );
  		}
  	else
  		{
  		# do the insertion to epoc.ini
  		&generateEpocIni;
    	}
	close( INFILE );
	}
else
	{
	if( -f "$drive\\$epocroot\\epoc32\\$target\\c\\nonethernetced.cfg" )
		{
		system( "$drive\\$epocroot\\epoc32\\release\\$target\\$variant\\ced -i c:\\nonethernetced.cfg" ) == 0
				or die "Can't find backup ced file!\n";
		system( "move $drive\\$epocroot\\epoc32\\$target\\c\\nonethernetced.cfg $drive\\$epocroot\\epoc32\\$target\\c\\cedout.cfg" ) == 0
				or die "Can't rename backup ced file!\n";
		}
	else
		{
		print "No restore file found!\n";
		}
	}




#
# Determines, validates, and returns EPOCROOT.
#
sub getEpocroot
{
    my $epocroot = $ENV{EPOCROOT};
    die "ERROR: Must set the EPOCROOT environment variable.\n"
	if (!defined($epocroot));
    $epocroot =~ s-/-\\-go;	# for those working with UNIX shells
    die "ERROR: EPOCROOT must be an absolute path, " .
	"not containing a drive letter.\n" if ($epocroot !~ /^\\/);
    die "ERROR: EPOCROOT must not be a UNC path.\n" if ($epocroot =~ /^\\\\/);
    die "ERROR: EPOCROOT must end with a backslash.\n" if ($epocroot !~ /\\$/);
    die "ERROR: EPOCROOT must specify an existing directory.\n"
	if (!-d $epocroot);
    return $epocroot;
}

#
# Determines and returns the current drive, if any.
#
sub getDrive
{
    my $wd = cwd;
    my $drive;
    if($wd =~ /^([a-zA-Z]:)/) {
	$drive = $1;
    } else {
	# Perhaps we're on a machine that has no drives.
	$drive = "";
    }
    return $drive;
}

#
#
#
sub generateEpocIni
{
	my @outLines;
	my $length = 0;
	my $needToAppend = TRUE;
	my $epocIniAlreadyExists = FALSE;
	my $finished = FALSE;
 	print "generating epoc ini\n";
	if ( -e "$drive\\$epocroot\\epoc32\\data\\epoc.ini" )
	   {
	   $epocIniAlreadyExists = TRUE;
 	   open( EPOCINI, "+<$drive\\$epocroot\\epoc32\\data\\epoc.ini" );
	   seek( EPOCINI, 0, 0 );
	   while( <EPOCINI> )
	   		{
			chomp;
	   		my $sLine = $_;
			if( $sLine =~ /ETHER_NIF=(.*)/i )
				{
				#print "matched etherNIF\n";
				$length = length( $etherNif );
    			substr( $sLine, (index( $sLine, "=" )+1), $length ) = $etherNif;
				$needToAppend = FALSE;
				}
			elsif( $sLine =~ /ETHER_MAC=(.*)/i )
				{
				#print "Matched ETHER_MAC\n";
				$length = length( $etherMac );
				substr( $sLine, (index( $sLine, "=" )+1), $length ) = $etherMac;
				$needToAppend = FALSE;
	    		}
			elsif( $sLine =~ /ETHER_SPEED=(.*)/i )
				{
				#print "Matched etherSpeed\n";
    			$length = length( $etherSpeed );
				substr( $sLine, (index( $sLine, "=" )+1), $length ) = $etherSpeed;
				$needToAppend = FALSE;
	   			}
			push( @outLines, $sLine );
			push( @outLines, "\n" );
	   		}
	   	if ( $needToAppend eq FALSE )
	   	   	{
	   	   	print "Writing new settings into epoc.ini\n";
	   	   	# we have read the entire file and replaced what we need to
	   	   	# now lets write it back to the file
		   	seek( EPOCINI, 0, 0 );
	   	   	print EPOCINI @outLines;
	   	   	$finished = TRUE;
           	}
        close ( EPOCINI );
		}

	if ( $finished eq FALSE )
	    {
       	if ( ($needToAppend eq TRUE) && ($epocIniAlreadyExists eq TRUE) )
       	    {
       	    print "Appending settings to current epoc.ini\n";
       	  	# we must append all the settings onto the end of the current epoc.ini
	   	  	open( EPOCINI, ">>$drive\\$epocroot\\epoc32\\data\\epoc.ini" ) or die "Can't open epoc.ini!\n";
       	  	}
    	elsif ( $epocIniAlreadyExists eq FALSE )
       	 	{
       	 	print "Creating new epoc.ini\n";
       	  	# create new file
	   	  	open( EPOCINI, ">$drive\\$epocroot\\epoc32\\data\\epoc.ini" );
       	  	}

		print EPOCINI "\nETHER_NIF=";
	   	print EPOCINI "$etherNif\n";
	   	print EPOCINI "ETHER_MAC=";
	   	print EPOCINI "$etherMac\n";
	   	print EPOCINI "ETHER_SPEED=";
	   	print EPOCINI "$etherSpeed\n";
	   	close ( EPOCINI );
     	}
}
