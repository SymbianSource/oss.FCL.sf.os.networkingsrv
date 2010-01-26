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
# Main netperf controller script
# 
#

#!perl

use strict;
use Getopt::Long;
use File::Copy;
use Net::FTP;
use Cwd;
use Win32::Process;
use File::Temp qw/ tempfile tempdir /;
use Carp qw(cluck);
use Win32::TieRegistry;

my $scriptlocation;
BEGIN{$scriptlocation=$0; $scriptlocation=~s{[\\/][^\\/]+$}{}; push(@INC, $scriptlocation); }
use netperf;

my $epocroot = GetEpocRootAbsolute();


$ENV{PATH} = "${epocroot}epoc32\\tools\\netperf\\bin;".$ENV{PATH};

my @targets = ('emulator', 'devboard', 'device');

my $runmode = 0;
my $generatemode = 0;
my $processmode = 0;

my $interactive = 0;

my $expecting_results = 0;

my $target;
my $setupDir_local;
my $setupDir_target;
my $runDir_local;
my $runDir_target;
my $remote_test_bearer_ip;
my $remote_control_bearer_ip;
my $remote_control_bearer_port;
my $method;
my $device_test_bearer_ip;
my $device_test_bearer_iap;
my $device_test_bearer_snap;
my $device_control_bearer_ip;
my $device_control_bearer_iap;
my $device_control_bearer_snap;
my $device_control_bearer_port;
my $device_profiler_arg;
my $testdriver_mode=0;
my $build;
my $platform;
my $rates_arg='';
my $packetcapture_arg='';
my $packetcapture_ftp_server;
my $packetcapture_ftp_user;
my $packetcapture_ftp_password;
my $run_services=1;
my $commsdat_template='';

# certificate
my $cert;
my $key;
my $passopts;

# UCC services
my $uccsvc;
my $exesvc;
my $alwaysOnlineResultsDir = tempdir( CLEANUP => 1 );

sub INT_handler    { die("Interrupted. Aborting.\n"); }
sub BREAK_handler  { die("Interrupted. Aborting.\n"); }
sub INT2_handler   { die("Interrupted. Aborting.\n"); }
sub BREAK2_handler { die("Interrupted. Aborting.\n"); }
sub DIE_handler    { StopUccServices(); ProcessResults(); }
$SIG{'INT'} = 'INT_handler';
$SIG{'BREAK'} = 'BREAK_handler';
$SIG{'INT2'} = 'INT2_handler';
$SIG{'BREAK2'} = 'BREAK2_handler';
$SIG{'__DIE__'} = 'DIE_handler';

my $inifile='netperf.ini';

if($ARGV[0]=~/\.ini$/)
	{
	$inifile=shift;
	}

$inifile=GetAbsoluteFilePath($inifile);


# main entry
main();




########

sub ShowHelp
{
	my $target;

	print "usage: netperf.pl [<inifile>] <target> [generate|run|process] [interactive]\n\n";
	print "inifile: netperf.ini by default\n";
	print "targets:\n";

	foreach $target (@targets)
	{
		print "\t$target\n";
	}
	print "generate:     if specified, only build test files\n";
	print "run:          if specified, run only the test step without building test files\n";
	print "process:      if specified, only process the results\n";
	print "interactive:  if specified, prompt the user to prepare the target\n";
	print "               with the generated files, before running the tests\n";

	exit;
}

sub FixPath($)
{
	my ($in)=@_;
	if($in=~/^\\$/)
	{
		$in.=' ';
	}
	else
	{
		$in=~s/\\+$//;
		$in=~s/\\+/\\/g;
	}

	return $in;
}

sub Execute($)
{
	my ($cmdline) = @_;

	print "EXEC: $cmdline\n====\n";

	system($cmdline);
	if ($?)
	{
		die "execution failed\n";
	}
}



sub LoadIniFile($)
{
	my ($fname) = @_;
	my @ini = ();

	open (INIFILE, $fname) or die "Cannot open file \'$fname\' for reading\n";
	@ini = <INIFILE>;
	close INIFILE;

	return @ini;
}

sub GetKey
{
	my ($section, $key, $ini, $mandatory, $default) = @_;
	my $lastsec; # last section name

	foreach (@$ini)
	{
		my $line = $_;

		# ignore comments
		if ($line =~ m/^(#+)(.*)$/)
		{
			next;
		}

		# check for section
		if ($line =~ m/^\[(.+)\]/)
		{
			$lastsec = $1;
			next;
		}

		# extract key=value
		if ($line =~ m/^([a-zA-Z0-9]+)(\s*)=(\s*)(.*)$/)
		{
			my ($lastkey, $value) = ($1, $4);
			if ($section eq "")
			{
				die "ini file structure corrupted\n";
			}
			if ($lastsec !~ m/^$section$/)
			{
				next;
			}

			if ($key =~ /$lastkey/i)
			{
				if ($mandatory and $value eq "")
				{
					last;
				}

				print "'$section'/'$key' = $value\n";
				return $value;
			}
		}
	}

	if ($mandatory)
	{
		die "parameter '$key' not found in section '$section'\n";
	}
	return $default;
}


###########

sub LoadConfiguration()
{
	ShowHeading("load configuration");

	# default values
	$setupDir_local = "netperf_${target}_setup";
	$setupDir_target = "C:\\";
	$runDir_local = "netperf_$target";
	$runDir_target = "C:\\";

	# load values from a file
	my @ini      = LoadIniFile($inifile);

	$remote_test_bearer_ip    = GetKey("TestControllerPC",    "TestBearerIP",  \@ini, 1);
	$remote_control_bearer_ip   = GetKey("TestControllerPC",    "ControlIP",     \@ini, 1);
	$remote_control_bearer_port = GetKey("TestControllerPC",    "ControlPort",   \@ini, 0, 1683);

	$method = GetKey("TestControllerPC",  "Method",        \@ini, 1);
	if($method=~/^Testdriver$/i)
		{
		$testdriver_mode=1;
		}
	elsif($method=~/^CopyTestScripts$/)
		{
		$testdriver_mode=0;
		}
	else
		{
		die(qq{Field [TestControllerPC].Method should be "Testdriver" or "CopyTestScripts"\n});
		}

	unless($runmode) # ie if there's some setup necessary...
		{
		die("Setup directory on target [$setupDir_target] needs to be at least a drive letter") unless ($setupDir_target =~ /^[a-zA-Z\\]/);
		$setupDir_target .= ':' if (length($setupDir_target) == 1);
		$setupDir_target .= '\\' unless($setupDir_target =~ /\\$/);
		}
		
	die("Run directory on target [$runDir_target] needs to be at least a drive letter") unless ($runDir_target =~ /^[a-zA-Z]/);
	$runDir_target .= ':' if (length($runDir_target) == 1);
	$runDir_target .= '\\' unless($runDir_target =~ /\\$/);
	
	CreateDir("$runDir_local") or die("Bad local run directory specified [$runDir_local]\n");

	my $tmp = GetKey("TestControllerPC", "DestinationForSetupData", \@ini, 0);
	$setupDir_local = $tmp if($tmp);
	die("Setup directory on local machine [$setupDir_local] needs to be at least a drive letter") unless ($setupDir_local =~ /^[a-zA-Z\\]/);
	$setupDir_local .= ':' if (length($setupDir_local) == 1);
	$setupDir_local .= '\\' unless($setupDir_local =~ /\\$/);
	die("Setup directory on local machine : Bad folder specified $setupDir_local") if (-e $setupDir_local && !-d $setupDir_local);

	$tmp = GetKey("DeviceUnderTest", "LocationForSetupData", \@ini, 0);
	$setupDir_target = $tmp if($tmp);
	
	if($method=~/^CopyTestScripts$/)
		{
		($runDir_target,$runDir_local) = ($setupDir_target,$setupDir_local);
		}


	
	$tmp = GetKey("TestControllerPC", "RunServices", \@ini, 0, "yes");
	if($tmp =~ m/^(0|no?|false|off)$/i)
		{
		$run_services=0;
		}

	$cert     = GetKey("TestControllerPC", "Cert",     \@ini, 0);
	$key      = GetKey("TestControllerPC", "Key",      \@ini, 0);
	$passopts = GetKey("TestControllerPC", "PassOpts", \@ini, 0);
	
	$device_test_bearer_ip    = GetKey("DeviceUnderTest",    "TestBearerIP",        \@ini, 1);
	$device_test_bearer_iap   = GetKey("DeviceUnderTest",    "TestBearerIAP",       \@ini, 0, 0);
	$device_test_bearer_snap  = GetKey("DeviceUnderTest",    "TestBearerSNAP",      \@ini, 0, 0);
	$device_control_bearer_ip   = GetKey("DeviceUnderTest",    "ControlIP",           \@ini, $testdriver_mode?1:0);
	$device_control_bearer_iap  = GetKey("DeviceUnderTest",    "ControlIAP",          \@ini, 0, 0);
	$device_control_bearer_snap = GetKey("DeviceUnderTest",    "ControlSNAP",         \@ini, 0, 0);
	$device_control_bearer_port = GetKey("DeviceUnderTest",    "ControlPort",         \@ini, 0, 3000);
	
	$device_profiler_arg = GetKey("DeviceUnderTest",    "RunSamplingProfiler", \@ini, 0, 'no');
	if($device_profiler_arg =~ m/^(1|y(es)?|true|on)$/i)
		{
		$device_profiler_arg='--profile';
		}
	else
		{
		$device_profiler_arg='';
		}
	$build               = GetKey("DeviceUnderTest", "BuildVariant",  \@ini, 0, "urel");
	$platform            = GetKey("DeviceUnderTest", "Platform",      \@ini, 0, "armv5");
	my $rates            = GetKey("Test", "Rates", \@ini, 0);
	$rates_arg="--rates=$rates" if defined $rates;


	$packetcapture_arg   = GetKey("(PacketCapturePC|NetworkMonitorPC)",    "PacketCapture", \@ini, 0, 'no');
	if($packetcapture_arg=~ m/^(1|y(es)?|true|on)$/i)
		{
		$packetcapture_arg='--packetcapture';
		$packetcapture_ftp_server          = GetKey("(PacketCapturePC|NetworkMonitorPC)",    "IPAddress",     \@ini, 0);
		$packetcapture_ftp_user            = GetKey("(PacketCapturePC|NetworkMonitorPC)",    "FTPUser",       \@ini, 0);
		$packetcapture_ftp_password        = GetKey("(PacketCapturePC|NetworkMonitorPC)",    "FTPPassword",   \@ini, 0);
		}
	
	$commsdat_template = GetKey("DeviceUnderTest","CommsdatTemplate",\@ini,0);
	
}

sub GenerateTestFiles()
{
	ShowHeading("generating test files..");

	# prepare IAP/SNAP arguments
	my $test_bearer_pref;
	if ($device_test_bearer_iap)
	{
		$test_bearer_pref = "--test_bearer_iap=$device_test_bearer_iap";
	}
	elsif ($device_test_bearer_snap)
	{
		$test_bearer_pref = "--test_bearer_snap=$device_test_bearer_snap";
	}

	my $control_bearer_pref;
	if ($device_control_bearer_iap)
	{
		$control_bearer_pref = "--control_bearer_iap=$device_control_bearer_iap";
	}
	elsif ($device_control_bearer_snap)
	{
		$control_bearer_pref = "--control_bearer_snap=$device_control_bearer_snap";
	}

	
	my $testdriver_option = $testdriver_mode?'--testdriver':'';
	my $cmdline = qq{perl -s -S preparescripts.pl $setupDir_local $setupDir_target $runDir_local $runDir_target RemoteHost=$remote_control_bearer_ip:$remote_control_bearer_port $device_test_bearer_ip $remote_test_bearer_ip $rates_arg $device_profiler_arg $testdriver_option --platform=$target --writecommsdb $test_bearer_pref $control_bearer_pref $packetcapture_arg "--inifile=$inifile"};

	Execute($cmdline);
}

sub TestdriverConfig()
{
	ShowHeading("configuring testdriver..");

	my $epocroot = $ENV{'EPOCROOT'};
	$epocroot = FixPath($epocroot);

	my $cmdline = "testdriver config -b $build -e \"$epocroot\" -k EKA2 -p $platform";
	$cmdline .= " --repos \"$runDir_local/repository\" -c \"$runDir_local/results\"";
	$cmdline .= " -s root -x \"$runDir_local/xmlroot\" --source \"$epocroot\"";
	$cmdline .= " --platsec ON --bldmake OFF --bldclean OFF";

	if (defined $cert)
	{
		$cmdline .= " --cert \"$cert\"";
	}
	if (defined $key)
	{
		$cmdline .= " --key \"$key\"";
	}
	if (defined $passopts)
	{
		$cmdline .= " $passopts";
	}

	Execute($cmdline);
}

sub TestdriverBuild()
{
	ShowHeading("building testdriver tests..");

	Execute("testdriver build");
}

sub RunUccServices()
{
	print "Starting UCC services\n";

	my $configdir = $epocroot.'epoc32\tools\netperf\data';
	
	my $uccbinary = 'c:\symbian\epoc32\engineeringtools\ucc\bin\ucc.exe';

	my $tmp = $Registry->{"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SITK\\UninstallString"};
	if(defined $tmp && $tmp =~ s/SITK_Uninstall.exe$/ucc\\bin\\ucc.exe/i)
		{
		print "UCC path found in registry: $tmp\n";
		$uccbinary=$tmp;
		}
		
	# drop to some default if SITK not installed (it really should be!)
	unless (-e $uccbinary)
		{
		print "UCC binary not found at $uccbinary.. ";
		$uccbinary = $epocroot.'epoc32\engineeringtools\ucc\bin\ucc.exe';
		print "Hopefully it's at $uccbinary.\n";
		}

	my $ucccmd    = "ucc.exe --tcpcontrol $remote_control_bearer_port";
	print "Running UCC with command: $ucccmd\n";
	unless (Win32::Process::Create($uccsvc, $uccbinary, $ucccmd, 0, NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, $configdir))
	{
		print("Can't run UCC at $uccbinary !\n");
		die Win32::FormatMessage(Win32::GetLastError());
	}

	
	my $exeservicebinary = $epocroot.'epoc32\tools\netperf\bin\exeservice.exe';
	
	my $exeservicecmd    = qq{exeservice.exe --outputdir "$alwaysOnlineResultsDir\" --proclist iperf,h2_resettousbms,h4hrp_resettousbms,34xx_sdp_resettousbms};
	print "Running exeservice with command: $exeservicecmd\n";
	unless (Win32::Process::Create($exesvc, $exeservicebinary, $exeservicecmd, 0, NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, "."))
	{
		print("Can't run exe service at $exeservicebinary !\n");
		my $lasterr = Win32::GetLastError();
		$uccsvc->Kill(-1);
		die Win32::FormatMessage($lasterr);
	}
	print "UCC services started\n";
}

sub StopUccServices()
{
	$exesvc->Kill(-1) if defined $exesvc;
	$uccsvc->Kill(-1) if defined $uccsvc;

	print "UCC services stopped\n" if defined $exesvc or defined $uccsvc;
}

sub TestdriverRun()
{
	ShowHeading("running testdriver..");

	my $cmdline = "testdriver run -t tcp:$device_control_bearer_ip:$device_control_bearer_port";
	if (defined $passopts)
	{
		$cmdline .= " $passopts";
	}

	Execute($cmdline);
}

sub DownloadMonitorDataFile($)
{
	my $ftp;
	my ($targpath) = @_;
	my (@filelist);
	my $transferred = 0;

	print "Downloading results from monitoring PC..\n";

	if ($packetcapture_ftp_server eq "")
	{
		print "no ftp server defined\n";
		return;
	}

	$ftp = Net::FTP->new($packetcapture_ftp_server, 0);
	if (not defined $ftp)
	{
		print "cannot connect ftp server '$packetcapture_ftp_server'\n";
		return;
	}

	if (not $ftp->login($packetcapture_ftp_user, $packetcapture_ftp_password))
	{
		print "login failed: ", $ftp->code(), " - ", $ftp->message(), "\n";
		return;
	}
	print "'$packetcapture_ftp_server' ftp server connected\n";

	$ftp->binary();
 
	@filelist = $ftp->ls("*.pcap");

	print "transferring files...\n";

	foreach (@filelist)
	{
		print "-> $_\n";

		if ($ftp->get($_, "$targpath/$_"))
		{
			$transferred++;
			if (not $ftp->delete($_))
			{
				print $ftp->code(), " - ", $ftp->message();
			}
		}
		else
		{
			print $ftp->code(), " - ", $ftp->message();
		}
	}

	print "done!\ntransferred $transferred file(s)\n";

	$ftp->quit();
	print "disconnected\n";
}

sub MoveTestDriverLogs()
	{
	print "Moving TestDriver logs..\n";

	# does the results directory exist?
	unless (-d "$runDir_local/results")
		{
		print "Can't open results folder $runDir_local/results\n";
		return;
		}

	# move testdriver log files
	if(opendir(DIR, "."))
		{
		my @list_of_files = readdir(DIR);

		foreach (@list_of_files)
			{
			if ($_ =~ /^TestDriver.*\.log$/)
				{
				rename "$_", "$runDir_local/results/$_" or print " moving $_ FAILED.\n";
				}
			}	

		closedir(DIR);
		return;
		}
	print "Moving TestDriver logs failed.\n";
	}

sub MoveExeServiceOutputs()
	{
	print "Moving ExeService outputs..\n";

	# does the results directory exist?
	unless (-d "$runDir_local/results")
		{
		print "Can't open results folder $runDir_local/results\n";
		return;
		}

	my $initialDir=Cwd::cwd();
	
	chdir "$runDir_local/results" or die;
	my $resultsDir=Cwd::cwd();

	if(chdir $alwaysOnlineResultsDir)
		{
		my $sourceDir=Cwd::cwd();

		system("dir");

		# move iperf log files
		if(opendir(DIR, "."))
			{
			my @list_of_files = readdir(DIR);

			foreach (@list_of_files)
				{
				if(-f $_)
					{
					print "renaming $_ to $resultsDir/$_..\n";
					rename "$_", "$resultsDir/$_" or print " renaming $_ FAILED.\n";
					}
				}
			system("dir");

			closedir(DIR);
			chdir $initialDir;
			}
		return;
		}
	print "Moving ExeService outputs failed.\n";
	}

sub RenameResultsFolder()
{
	# does the results directory exist?
	return unless (-d "$runDir_local/results");

	# don't rename empty directory
	if (not scalar <$runDir_local/*>)
	{
		return $runDir_local;
	}

	# rename results folder
	my ($sec,$min,$hour,$mday,$mon,$year) = localtime();
	my $datetime = sprintf("%.2d%.2d%.2d%.2d%.2d%.2d", 1900+$year, $mon+1, $mday, $hour, $min, $sec );

	my $origname = "$runDir_local/results";
	my $newname = "$runDir_local/results_$datetime";
	my $resultname=$origname;

	if (rename($origname,$newname))
	{
		$resultname = $newname;
	}
	else
	{ 
		print "cannot rename results directory\n";
	}
	return $resultname;
}

sub ProcessResults()
	{
	if ($expecting_results)
		{
		ShowHeading("processing results..");
	
		MoveTestDriverLogs();
	
		MoveExeServiceOutputs();
	
		# download log from monitoring PC
		DownloadMonitorDataFile("$runDir_local/results") if $packetcapture_arg;
	
		system("dir $runDir_local");
		
		# rename results folder by current date/time
		my $renamedResultsFolder = RenameResultsFolder();
	
		Execute(qq{perl -S processresults.pl "$renamedResultsFolder" "$inifile"});
	
		print "result files processed\n";
		}
	else
		{
		print "No results available. Test run failed.\n";
		}
	}


#########

sub main()
{
	die "'EPOCROOT' not defined\n" unless defined $ENV{'EPOCROOT'};

	# Ensure that testdriver is in $ENV{'PATH'}
	if (not $ENV{'PATH'} =~ m/testdriver/i)
	{
		$ENV{'PATH'} = "c:\\symbian\\sitk\\testdriver;" . $ENV{'PATH'};
	}

	# process command line
	if ($#ARGV < 0)
	{
		ShowHelp();
	}
	$target = grep(/^$ARGV[0]$/i, @targets);
	if ($target != 1)
	{
		print "unknown target specified.\n";
		ShowHelp();
	}
	$target = lc(shift @ARGV);
	
	my $arg;
	while(defined($arg = shift @ARGV))
	{
		if ($arg eq 'run' && ($generatemode+$runmode+$processmode==0))
		{
			$runmode = 1;
		}
		elsif ($arg eq 'generate' && ($generatemode+$runmode+$processmode==0))
		{
			$generatemode = 1;
		}
		elsif ($arg eq 'process' && ($generatemode+$runmode+$processmode==0))
		{
			$processmode = 1;
		}
		elsif ($arg eq "interactive")
		{
			$interactive = 1;
		}
		else
		{
			ShowHelp();
		}
	}

	# read configuration from ini file
	LoadConfiguration();

	if ($runmode || $processmode)
	{
		print "skipping build step\n";
	}
	else
		{
		if($method =~ /^Testdriver$/i)
			{
			# configure testdriver
			TestdriverConfig();
			}

		# generate test files
		GenerateTestFiles();

		# the output directory has to exist before service start!
		CreateDir("$runDir_local\\results");

		if($method =~ /^Testdriver$/i)
			{
			ShowHeading("Attention",'#');
			if($interactive)
				{
				if ($target eq "emulator")
					{
					print "\nN.B. Use of testdriver with the emulator is discouraged.\n";
					print "It's better to operate in CopyScriptsTo mode.\n";
					}
				else
					{
					print "\nIf appropriate, please build the appropriate ROM and boot it.\n";
					}

				my $setupDir_local_abs = File::Spec->rel2abs($setupDir_local);
			#	my $runDir_local_abs = File::Spec->rel2abs($runDir_local);
				my $statinipath_local_abs="${setupDir_local_abs}\\system\\data\\stat.ini";
				$statinipath_local_abs=~s{[\\/]+}{\\}g;
				my $commsdatpath_local_abs="$setupDir_local_abs\\testdata\\configs\\netperf\\netperf.(xml|cfg)";
				$commsdatpath_local_abs=~s{[\\/]+}{\\}g;
				my $runscriptcmd_target="$setupDir_target\\netperf_init";
				$runscriptcmd_target=~s{[\\/]+}{\\}g;
		
				if(DirectoryMounted($setupDir_local))
					{
		
					my $bearerStr = "the default connection";
					$bearerStr = "SNAP $device_control_bearer_snap" if $device_control_bearer_snap;
					$bearerStr = "IAP $device_control_bearer_iap" if $device_control_bearer_iap; # IAP takes priority
					print "
Setup files have been copied to $setupDir_local.
These files attempt to:";
				  	if($commsdat_template)
				  		{
					  	print "\n  => Use ced to provision the commsdat generated at $commsdatpath_local_abs onto the device.";
						}
					print "
  => Ensure Stat starts with the correct connection ($bearerStr)
      by making the file $statinipath_local_abs available to the device.
  => Launch Stat so that Testdriver can accept test cases

To use the setup scripts:
	- ensure the contents of folder $setupDir_local_abs are present
	   at location $setupDir_target on the Symbian OS device.
	- then on the Symbian OS device eshell, type:
			$runscriptcmd_target

Depending on your configuration, running the scripts might not be possible.
If this is the case, perform the above steps marked => manually.

";
					}
				else
					{
					print "To start the tests:
	- ensure the previously written setup files are present
	   at location $setupDir_target on the Symbian OS device.
	- then on the Symbian OS device eshell, type:
			$runscriptcmd_target\n\n";
					}
				
				print "Ensure Stat is up and running on the device, press ENTER and I'll run the tests.\n\n";
				my $dummy = <STDIN>;
				}
			else
				{
				my $bearerStr = "the default connection";
				$bearerStr = "SNAP $device_control_bearer_snap" if $device_control_bearer_snap;
				$bearerStr = "IAP $device_control_bearer_iap" if $device_control_bearer_iap; # IAP takes priority				
				print "
Proceeding to run the tests. For this to succeed you need:

  1. An appropriate commsdat provisioned on the device (with access points defined
      for the test bearer, and separate control bearer if appropriate),
  2. A config file for Stat on the device which ensures it's set to listen for
      commands on the correct bearer ($bearerStr),
  3. Stat to be up and running on the device, and listening on $device_control_bearer_ip:$device_control_bearer_port.

If you don't have these 3 things, the tests can not run.
  ";
				}
			# testdriver build
			TestdriverBuild();
			}
		}

	exit if $generatemode;
	
	if($processmode)
	{
		print "Skipping run step..\n";
		$expecting_results = 1;
	}
	else
	{
		RunUccServices() if $run_services;

		$expecting_results = 1;

		if($method =~ /^Testdriver$/i)
		{
			# testdriver run
			TestdriverRun();
		}
		elsif(-e 'netperf.copyscripts.runstep.cmd')
		{
			print "runstep script detected. Launching that...\n";
			system("netperf.copyscripts.runstep.cmd");
		}
		else
		{ # method was 'copy to emulator / card'
			my $runscriptcmd_target="$setupDir_target\\netperf";
			$runscriptcmd_target=~s{[\\/]+}{\\}g;
			ShowHeading("Attention",'#');
			print "Setup and test files have been copied to $setupDir_local.\n";
			print "Ensure the contents of that folder are present at location $setupDir_target on the Symbian OS device.\n";
			print "\n Then on the Symbian OS device eshell, type:\n\t$runscriptcmd_target\n\n";
			print "Once the tests have finished, ensure the files are available again at $setupDir_local.\n";
			print "Then press ENTER and I'll collect and analyse results..\n\n";
			my $dummy = <STDIN>;
		}

		StopUccServices();
	}
	
	# process results
	undef $SIG{'__DIE__'};
	ProcessResults();
}

