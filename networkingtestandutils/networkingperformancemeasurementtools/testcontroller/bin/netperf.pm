# Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# Common perl functions used by other scripts in Netperf.
# 
#

#!perl

use strict;
use File::Spec;

my @customPathsToSearch = ();
my @defaultPathsToSearch = ();

my $netperfLogDir = ".";

sub SetLogDir($)
	{
	($netperfLogDir)=@_;
	$netperfLogDir=File::Spec->rel2abs($netperfLogDir);
	}

sub AddPath
	{
	my($dirToAdd)=@_;
	push @customPathsToSearch, File::Spec->rel2abs($dirToAdd);
	}


# set up default search path (always at end) using the functions for custom paths..
my $scriptlocation=$0;
$scriptlocation=~s{[\\/][^\\/]+$}{};
AddPath($scriptlocation);
AddPath("$scriptlocation/../data");
@defaultPathsToSearch=@customPathsToSearch;

# now set up custom paths (. should always be at beginning to avoid surprises..)
@customPathsToSearch=();
AddPath('.');

sub GetAbsoluteFilePath
{
	my($file)=@_;
	my $originalWd = Cwd::cwd;

	my @pathsToSearch = (@customPathsToSearch, @defaultPathsToSearch);
	
	while(@pathsToSearch)
		{
		my $possibleDir=shift(@pathsToSearch);
		#print(" Looking in [$possibleDir] for $file..\n");
		chdir $possibleDir;
		my $found=0;
		if(-f $file)
			{
			$found=1;
			print("Using file '$file' ");
			$file = File::Spec->rel2abs($file);
			print("at location '$file'.\n");
			}
		chdir $originalWd;
		return $file if $found;
		}

	# not found!
	die("File $file not found!\n");
}


sub CreateDir($)
{
	my ($dir) = @_;

	unless (-d $dir)
	{
		mkdir $dir or die "Cannot create directory \'$dir\'\n";
	}
}

sub GetEpocRootAbsolute()
{
	my $cwd = Cwd::cwd();
	my $epocroot = $ENV{'EPOCROOT'};
	chdir $epocroot or die ("Bad %EPOCROOT% specified [$epocroot] - I can't chdir to it.");
	my $epocrootAbsolute = Cwd::cwd();
	$epocrootAbsolute=~s{/}{\\}g;
	chdir $cwd or die("can't chdir back to $cwd");
	return $epocrootAbsolute;
}

sub ShowHeading
{
	my ($text,$char) = @_;
	$text = uc($text);
	$char='-' unless $char;
	
	my $bgchars = 79;
	if(length($text)>71)
		{
		$bgchars = length($text) + 8;
		}
	
	my $midbgchars = ($bgchars - length($text) - 1) / 2;
	$midbgchars = int($midbgchars);
	$bgchars = $midbgchars * 2 + 2 + length($text);

	$bgchars = $char x $bgchars;
	$midbgchars = $char x $midbgchars;

	print "\n$bgchars\n";
	print "$midbgchars $text $midbgchars\n";
	print "$bgchars\n\n";
}

sub DirectoryMounted($)
	{
	# this is the only non Win32-API way to check if the card is available without popping up a dialog.
	my ($pathToTest)=@_;
	my $testFileName="$pathToTest/netperf_file_test.zzz";
	my $result = open(TESTFILE,">$testFileName");
	if($result)
		{
		close TESTFILE;
		unlink $testFileName;
		}
	return $result;
	}

	
sub DateString()
	{
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
                                                localtime(time);
	return sprintf("%04d-%02d-%02d %02d:%02d:%02d",$year+1900,$mon+1,$mday,$hour,$min,$sec);
	}

my $scriptName = $0;
$scriptName =~ m{([^\\/]+)$};
$scriptName = $1;
sub ScriptName()
	{
	return $scriptName;
	}
	
sub NetperfLog($)
	{
	my ($line)=@_;
	$line = DateString()." ".ScriptName().": ".$line;
	open(NETPERFLOG,">>$netperfLogDir/netperf.log");
	print NETPERFLOG $line;
	print $line;
	close NETPERFLOG;
	}


1;
