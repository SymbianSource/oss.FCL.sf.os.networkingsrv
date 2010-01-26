# Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# DHCP configs generator
# 
#

#!perl

use strict;
use File::Copy;
use File::Path;
use File::stat;


# print("$0 called with: [@ARGV]\n");

my $command = shift @ARGV;
my $platform = shift @ARGV;
my $variant = shift @ARGV;

my @addrmodes = (qw{ip4 ip6});
my $addrmode = '';  # switches between the above

my @boardtypes = (qw{lubbock omapxxx marvell8385});
my $boardtype = '';  # switches between the above

my %boarddrivers = ();

$boarddrivers{'emulator'} = 
    {
    'Agent' => 'nullagt.agt',
    'IfName' => 'ethint',
    'LDDFilename' => 'enet',
    'LDDName' => 'Ethernet',
    'PDDFilename' => 'ethernet',
    'PDDName' => 'Ethernet.Wins',
    'PacketDriverName' => 'EtherPkt.drv'
    };

$boarddrivers{'lubbock'} = 
    {
    'Agent' => 'nullagt.agt',
    'IfName' => 'ethint',
    'LDDFilename' => 'enet',
    'LDDName' => 'Ethernet',
    'PDDFilename' => 'ethernet',
    'PDDName' => 'Ethernet.Assabet',
    'PacketDriverName' => 'EtherPkt.drv'
    };

$boarddrivers{'omapxxx'} = 
    {
    'Agent' => 'nullagt.agt',
    'IfName' => 'ethint',
    'LDDFilename' => 'enet',
    'LDDName' => 'Ethernet',
    'PDDFilename' => 'ethernet',
    'PDDName' => 'Ethernet.MOMAP16xx',
    'PacketDriverName' => 'EtherPkt.drv'
    };

$boarddrivers{'marvell8385'} = 
    {
    'Agent' => 'wlanagt.agt',
    'IfName' => 'ethint',
    'LDDFilename' => 'WifiCard',
    'LDDName' => 'WifiCard',
    'PDDFilename' => 'WifiMvl83xxSymSdio',
    'PDDName' => 'WifiCard.PddMvl',
    'PacketDriverName' => 'WifiPkt.drv'
    };


# Constants.
#
my $epocRoot = $ENV{EPOCROOT};




# if neither udeb or urel is specifed, do both
#
my @variants = qw{UDEB UREL};

if(defined($variant))
{
	@variants = ($variant);
}



# Main.
#
unless ($command and $platform) {
 die "$0 Error: Invalid arguments [$command][$platform][$variant][@ARGV]\n";
}

my $platType;
if ($platform =~ /^(wins|wincw)/i) {
  $platType = 'emulator';
}
else {
  $platType = 'target';
}



foreach my $addr (@addrmodes)
{
	$addrmode = $addr;
	
	foreach my $var (@variants)
	{
		$variant = $var;
	
		if ($command eq 'releasables')
		{
			Releasables();
		}
		else
		{	
			print("\n** $command commsdat files for DHCP $platType $platform $variant ($addrmode) **\n");
	
			if ($command eq 'generate')
			{
			  Generate();
			}
			elsif ($command eq 'clean')
			{
			  Clean();
			}
			else
			{
			  die("$0: unknown command [$command]");
			}
		}
	}
}

print("\n");





# Subs.
#

sub SrcFile
{
  return ("DHCPced_$addrmode"."_xml_template.txt");
}

sub EmuDest
{
  return ("$epocRoot"."epoc32\\release\\$platform\\$variant\\z\\TestData\\configs\\DHCP\\DHCPced_$addrmode.xml");
}

sub TargDest
{
  return ("$epocRoot"."epoc32\\data\\z\\TestData\\configs\\DHCP\\DHCPced_$addrmode.xml.$::boardtype");
}

sub Generate
{

	if($platType eq 'emulator')
	{
		$::boardtype = 'emulator';
		TranslateFile(SrcFile(),EmuDest(),
				'<<AGENT_NAME>>', $boarddrivers{$::boardtype}{'Agent'},
				'<<NIF_NAME>>', $boarddrivers{$::boardtype}->{'IfName'},
				'<<LDD_FILENAME>>', $boarddrivers{$::boardtype}->{'LDDFilename'},
				'<<LDD_NAME>>', $boarddrivers{$::boardtype}->{'LDDName'},
				'<<PDD_FILENAME>>', $boarddrivers{$::boardtype}->{'PDDFilename'},
				'<<PDD_NAME>>', $boarddrivers{$::boardtype}->{'PDDName'},
				'<<PACKET_DRIVER_NAME>>', $boarddrivers{$::boardtype}->{'PacketDriverName'});
	}
	else
	{
		foreach $::boardtype (@boardtypes)
		{
			TranslateFile(SrcFile(),TargDest(),
				'<<AGENT_NAME>>', $boarddrivers{$::boardtype}{'Agent'},
				'<<NIF_NAME>>', $boarddrivers{$::boardtype}->{'IfName'},
				'<<LDD_FILENAME>>', $boarddrivers{$::boardtype}->{'LDDFilename'},
				'<<LDD_NAME>>', $boarddrivers{$::boardtype}->{'LDDName'},
				'<<PDD_FILENAME>>', $boarddrivers{$::boardtype}->{'PDDFilename'},
				'<<PDD_NAME>>', $boarddrivers{$::boardtype}->{'PDDName'},
				'<<PACKET_DRIVER_NAME>>', $boarddrivers{$::boardtype}->{'PacketDriverName'});
		}
	}
}


sub TranslateFile
{
	my $src = shift;
	my $dst = shift;

	my %srmap;
	my $srch;
	my $repl;
	while(defined($srch=shift) && defined($repl=shift))
	{
		$srmap{$srch} = $repl;
#		print("   replacing string '$srch' with '$repl'\n");
	}

# Ensure replacement file is more up-to-date than existing file
	if (FileIsYounger($src,$dst))
	{
		print("Translating '$src'\n  .. to '$dst'..\n");

		open(I,$src) or die("Error: Can't open $src for read!\n");
		open(O,">$dst") or die("Error: Can't open $dst for write!\n");
		
		while(my $line = <I>)
		{
			foreach my $searchfor (keys %srmap)
			{
				if($line =~ m/$searchfor/)
				{
					print("Replacing [$line]");
					my $replacewith = $srmap{$searchfor};
					$line =~ s/$searchfor/$replacewith/;
					print(" with [$line]\n");				
				}
			}
			print O $line;
		}

		close I;
		close O;

		print("\n");
	}
	else {print("$src not newer than $dst, so leaving it alone.\n");}
}


sub Clean {
	my @a = GetReleasables();

	foreach my $r (@a)
	{
		print("Deleting $r..\n");
		DeleteFile($r);
	}
}

sub Releasables
{
	my @a = (GetReleasables(),"\n");

    $, = "\n";

    print @a;
}

sub GetReleasables {
	my @releasables;
	if($platType eq 'emulator')
	{
		push @releasables, EmuDest();
	}
	else
	{
		foreach $::boardtype (@boardtypes)
		{
			push @releasables, TargDest();
		}
	}
	
	return @releasables;
}





sub SearchAndReplace
{
  my $string = shift;
  my $src = shift;
  my $dest = shift;

  $string =~ s/$src/$dest/g;
  return $string;
}


sub DeleteFile {
  my $file = shift;
  if (-e $file) {
    unlink ($file) or die "CreateCommdbs Error: Couldn't delete \"$file\": $!\n";
  }
}


sub FileIsYounger {
  my $file1 = shift;
  my $file2 = shift;
  return (FileModifiedTime($file1) > FileModifiedTime($file2));
}

sub FileModifiedTime {
  my $file = shift;
  if (-e $file) {
    my $st = stat($file) or return 0;
    return $st->mtime;
  }
  return 0;
}




__END__

PRJ_TESTEXPORTS

//-- ip4 specific
..\te_dhcp\config\DHCPced_emulator_ip4.xml				$epocRoot.\epoc32\release\winscw\udeb\z\TestData\configs\DHCP\DHCPced_ip4.xml
..\te_dhcp\config\DHCPced_emulator_ip4.xml				$epocRoot.\epoc32\release\winscw\urel\z\TestData\configs\DHCP\DHCPced_ip4.xml
// Target
..\te_dhcp\config\DHCPced_ip4.xml.lubbock				$epocRoot.\epoc32\data\z\TestData\configs\DHCP\DHCPced_ip4.xml.lubbock
..\te_dhcp\config\DHCPced_ip4.xml.omapxxx				$epocRoot.\epoc32\data\z\TestData\configs\DHCP\DHCPced_ip4.xml.omapxxx

  + ipv6