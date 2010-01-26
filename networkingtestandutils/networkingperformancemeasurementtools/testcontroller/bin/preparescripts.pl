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
# Used by netperf.pl
# Prepares setup and TEF scripts for netperf, driven from a .ini file and
# command-line arguments.
# 
#

#!perl

use strict;

use File::Spec;
use Cwd;
use Getopt::Long;
use Config::IniHash;
use Net::IPv4Addr;

my $scriptlocation;
BEGIN{$scriptlocation=$0; $scriptlocation=~s{[\\/][^\\/]+$}{}; push(@INC, $scriptlocation); }
use netperf;

my $getProfile = 0;
my $useCpuMeter = 1;
my $runIters = 1;
my $ratesStr = '';
my $writeCommsDb = 0;
my $forTestDriver = 0;
my $platform = 'omap';
my $buildtype = 'urel';
my $packetCapture = 0;
my $iniFile='netperf.ini';

my $test_bearer_iap=0;
my $test_bearer_snap=0;
my $test_bearer_pref="";
my $control_bearer_iap=0;
my $control_bearer_snap=0;
my $control_bearer_pref="";
 
my $testBearerPortBase; # port numbers count upwards from here

GetOptions( 'profile!' => \$getProfile, # whether to run sampling profiler and retrieve results
			'test_bearer_iap=i' => \$test_bearer_iap ,  # IAP for bearer
			'test_bearer_snap=i' => \$test_bearer_snap , # SNAP for bearer
			'control_bearer_iap=i' => \$control_bearer_iap ,  # IAP for control channel
			'control_bearer_snap=i' => \$control_bearer_snap ,  # SNAP control channel
			'rates=s' => \$ratesStr,  # override rates to use with e.g. --rates 50,100,150
			'writecommsdb!' => \$writeCommsDb,  # whether to generate comms db from template..
			'platform=s' => \$platform, # omap/emulator
			'testdriver!', => \$forTestDriver, # generate xml files for test driver instead of batch files for a card
			'packetcapture!' => \$packetCapture, # whether to run a packet monitoring UCC service on another PC
			'inifile=s' => \$iniFile, # name of ini file
			);

unless(-f $iniFile)
	{
	$iniFile="../data/$iniFile";
	die("Can't find ini file") unless(-f $iniFile);
	}

my $configFileLocation = File::Spec->rel2abs($iniFile);
$configFileLocation=~s{[\\/][^\\/]+$}{};
AddPath($configFileLocation);

my %configFileOpts;
$configFileOpts{'case'}='preserve';
$configFileOpts{'comment'}='#;';
my $configFile = ReadINI("$iniFile",%configFileOpts);
die("Can't read ini file [$iniFile]") unless $configFile;


my $currentDirectoryWhenScriptRun=cwd;

my ($setupDir_local,$setupDir_target,$runDir_local,$runDir_target,$uccini,$DUT_IP,$remote_IP)=@ARGV;

$testBearerPortBase = $configFile->{'Test'}->{'BasePortNumber'};
$testBearerPortBase = 5001 unless defined($testBearerPortBase);

my @rates = qw{};
if($ratesStr ne '')
	{
	@rates = split(/,/,$ratesStr);
	}


my $DUT_etherPddFileName = 'ethernet';
my $DUT_etherPddName = 'Ethernet.MOMAP16xx';
my $targetname = 'armv5';
if($platform =~/^emulator$/i)
	{
	$DUT_etherPddName = 'Ethernet.Wins';
	$targetname = 'winscw';
	}
if($DUT_IP =~ /^\d+\.\d+\.\d+\.\d+$/)
	{
	$DUT_IP .= '/24';
	}

my $DUT_gateway = $configFile->{'DeviceUnderTest'}->{'TestBearerGateway'};
unless ($DUT_gateway) # if not in config file, guess at value 1.
	{
	$DUT_gateway = Net::IPv4Addr::ipv4_network($DUT_IP); $DUT_gateway=~s{\d+/\d+$}{1};
	}

my ($DUT_IP,$DUT_cidr) = Net::IPv4Addr::ipv4_parse( $DUT_IP );
my $DUT_netmask = Net::IPv4Addr::ipv4_cidr2msk($DUT_cidr);

if($test_bearer_iap)
	{
	$test_bearer_pref="IAP=$test_bearer_iap\n";
	}
elsif($test_bearer_snap)
	{
	$test_bearer_pref="SNAP=$test_bearer_snap\n";
	}

if($control_bearer_iap)
	{
	$control_bearer_pref="IAP=$control_bearer_iap\n";
	}
elsif($control_bearer_snap)
	{
	$control_bearer_pref="SNAP=$control_bearer_snap\n";
	}


my $setupconfigpath_local="$setupDir_local/testdata/configs/netperf";
mkdir "$setupDir_local";
mkdir "$setupDir_local/testdata";
mkdir "$setupDir_local/testdata/configs";
mkdir "$setupDir_local/testdata/configs/netperf";

my $configpath_local="$runDir_local/testdata/configs/netperf";
mkdir "$runDir_local";
mkdir "$runDir_local/testdata";
mkdir "$runDir_local/testdata/configs";
mkdir "$runDir_local/testdata/configs/netperf";

my $scriptpath_local="$runDir_local/testdata/scripts/netperf";
mkdir "$runDir_local/testdata/scripts";
mkdir "$runDir_local/testdata/scripts/netperf";

my $resultspath_target=$runDir_target.'\results';
$resultspath_target=~s{[/\\]+}{\\}g;
mkdir "$runDir_local/results" unless $forTestDriver;

my $setupconfigpath_target=$setupconfigpath_local;
$setupconfigpath_target=~s{^\Q$setupDir_local}{${setupDir_target}};
$setupconfigpath_target=~s{[/\\]+}{\\}g;

my $configpath_target=$configpath_local;
$configpath_target=~s{^\Q$runDir_local}{${runDir_target}};
$configpath_target=~s{[/\\]+}{\\}g;

my $scriptpath_target=$scriptpath_local;
$scriptpath_target=~s{^\Q$runDir_local}{${runDir_target}};
$scriptpath_target=~s{[/\\]+}{\\}g;

my $batchpath="$runDir_local/netperf";
mkdir "$batchpath" unless $forTestDriver;
my $batchpathonboard=$batchpath;
$batchpathonboard=~s{^\Q$runDir_local}{${runDir_target}};
$batchpathonboard=~s{[/\\]+}{\\}g;




open(INI,">$configpath_local/netperfte.ini") or die "can't open $configpath_local/netperfte.ini for writing";

my $writeSetupFiles=DirectoryMounted($setupDir_local);

if($writeSetupFiles)
	{
	print "Setup media at $setupDir_local detected. Will write test setup files and scripts for the specified tests.\n";
	}
else
	{
	if($forTestDriver)
		{
		ShowHeading("Attention");
		print "I can't write to setup folder $setupDir_local.\nPresumably you've already ejected your setup media.\nProceeding to write scripts for the specified tests..\n\n";
		}
	else
		{
		die "I can't access setup folder $setupDir_local. So can't continue with the test run. Make this path available and re-run.\n";
		}
	}

if($writeSetupFiles)
	{
	open(NPINIT,">$setupDir_local/netperf_init.bat");
	print NPINIT "md $resultspath_target\n";
	WriteCommsdat() if $writeCommsDb;
	print NPINIT "statapi\n" if($forTestDriver);
	close NPINIT;
	
	WriteStatConfig() if($forTestDriver);
	}

WriteIniPreamble();

my ($testDriverXmlRoot,$netperfSourceRootForTestDriver,$rootpathForTestDriver,$configpath_localForTestDriver,$scriptpath_localForTestDriver,$bindepsPathForTestDriver);
if($forTestDriver)
	{
	($testDriverXmlRoot,$netperfSourceRootForTestDriver,$rootpathForTestDriver,$configpath_localForTestDriver,$scriptpath_localForTestDriver,$bindepsPathForTestDriver) = FindTestDriverVars();

	WriteTestDriverPreambles();
	}
else # batch mode
	{
	open(NETPERFBAT,">$runDir_local/netperf.bat") or die;

	my $netperfinibat="$setupDir_target\\netperf_init.bat";
	$netperfinibat=~s{[/\\]+}{\\}g;
	print NETPERFBAT "$netperfinibat\n";
	}

my @tests = (); # array of hashes to be built up by the following..:

sub notyetpastend($$$)
	{
	my ($operation,$i,$end)=@_;
	return ($i>=$end) if($operation eq '-');
	return ($i<=$end);
	}

if(@rates)
	{
	for(my $i=0 ; $i<@rates; $i++)
		{
		my $rate=@rates[$i];
		
		# values:
		#  100    -> 100kbps, udp+tcp, send+receive
		#  100@500 -> as above, with 500 byte packets
		#  100(+100)800us -> udp send at 100,200,300,400,500,600,700,800
		
		unless($rate=~/^\d+(\([\+\*\-][0-9\.]+\)\d+)?[utsr]*(\@\d+)?$/)
			{
			die("Invalid rates string specified [$rate]\n");
			}

		my $packetSizeOverride = 0;
		($rate=~s/\@(\d+)$//) && ($packetSizeOverride=$1);

		my ($udp,$tcp,$send,$recv)=(0,0,0,0);
		($rate=~s/u//) && ($udp=1);
		($rate=~s/t//) && ($tcp=1);
		($rate=~s/s//) && ($send=1);
		($rate=~s/r//) && ($recv=1);
		if($udp+$tcp == 0) {$udp=1;$tcp=1}
		if($send+$recv == 0) {$send=1;$recv=1}

		my @subrates = ();
		if ($rate =~ /^(\d+)\((.)([0-9\.]+)\)(\d+)$/)
			{
			my ($start,$operation,$modifier,$end)=($1,$2,$3,$4);
			print("$start,$operation,$modifier,$end\n");
			for(my $i=$start; notyetpastend($operation,$i,$end) ; )
				{
				push @subrates, $i; print("II $i\n");
				eval("\$i $operation= \$modifier");
				}
			}
		else
			{
			push @subrates, $rate;
			}
		
		foreach $rate (@subrates)
			{
			$rate=int($rate);
			if($send)
				{
				if($udp)
					{
					my %agents1=();
					$agents1{'label'} = "Send${rate}kbpsUDP";
					$agents1{'Sender_01'}->{Protocol} = 'UDP';
					$agents1{'Sender_01'}->{Rate} = $rate;
					$agents1{'Sender_01'}->{PacketSize} = $packetSizeOverride?$packetSizeOverride:1470;
					print "Reading from rates string: UDP send, ${rate}kbps\n";
					push @tests, \%agents1;
					}
			
				if($tcp)
					{
					my %agents2=();
					$agents2{'label'} = "Send${rate}kbpsTCP";
					$agents2{'Sender_01'}->{Protocol} = 'TCP';
					$agents2{'Sender_01'}->{Rate} = $rate;
					$agents2{'Sender_01'}->{PacketSize} = $packetSizeOverride?$packetSizeOverride:16384;
					print "Reading from rates string: TCP send, ${rate}kbps\n";
					push @tests, \%agents2;
					}
				}
	
			if($recv)
				{
				if($udp)
					{
					my %agents3=();
					$agents3{'label'} = "Recv${rate}kbpsUDP";
					$agents3{'Receiver_01'}->{Protocol} = 'UDP';
					$agents3{'Receiver_01'}->{Rate} = $rate;
					$agents3{'Receiver_01'}->{PacketSize} = $packetSizeOverride?$packetSizeOverride:1470;
					push @tests, \%agents3;
					print "Reading from rates string: UDP receive, ${rate}kbps\n";
					}
				
				if($tcp)
					{			
					my %agents4=();
					$agents4{'label'} = "Recv${rate}kbpsTCP";
					$agents4{'Receiver_01'}->{Protocol} = 'TCP';
					$agents4{'Receiver_01'}->{Rate} = $rate;
					$agents4{'Receiver_01'}->{PacketSize} = $packetSizeOverride?$packetSizeOverride:16384;
					push @tests, \%agents4;
					print "Reading from rates string: TCP receive, ${rate}kbps\n";
					}
				}
			}
		}
	}

# also try to read specific use cases from ini file...
	{
	if($configFile)
		{
#	print(keys %$configFile);
		foreach my $section (grep(/^Test_/,sort keys %$configFile))
			{
			$section=~m/^Test_(.+)/;
			my $testlabel = $1;
			print("Reading test $testlabel from ini file..\n");

			my %agents=(); # the senders/receivers used by the test
			$agents{'label'}=$testlabel;
			
			foreach my $name (keys %{$configFile->{$section}})
				{
				my $value = $configFile->{$section}->{$name};
				if($name eq 'DurationInSeconds')
					{
					unless($value =~ /^\d+$/)
						{
						die("DurationInSeconds must be an integer. Aborted");
						}
					print(" $name is $value\n");
					$agents{$name}=$value;
					}
				elsif($name eq 'UseLowerLayerPacketGenerator')
					{
					if($value =~ m/^(1|y|yes|true|on)$/i)
						{
						$value = 1;
						}
					else
						{
						$value = 0;
						}
					print(" $name is $value\n");
					$agents{$name}=$value;
					}
				elsif($name=~/^((Receiver|Sender)(_\w+)?)\.(\w+)$/)
					{
					my ($agentlabel,$variable)=($1,$4);
					print(" $agentlabel -- $variable is $value\n");
					$agents{$agentlabel}->{$variable}=$value;
					}
				else
					{
					die("Bad syntax [$name]".' for value in ini file! Use ^(Receiver|Sender(_\w+)?)\.(\w+)$ e.g. receiver_01.rate'."\n");
					}
				}
			my $numagents = (keys %agents);
			if($numagents<2)
				{
				die("No test agents specified in ini file for case $testlabel!");
				}
			push @tests,\%agents;
			}
		}
	}

if(@tests == 0)
	{
	die("No tests specified in ini file!\n");
	}


my $testCount=0;
my $totalDuration_sec=60; # generally observed setup time
foreach my $test (@tests)
	{
	$testCount++;
	WriteTestCase($test);
	}

close INI;
if($forTestDriver)
	{
	WriteTestDriverPostambles();
	}
else
	{
	close NETPERFBAT;
	}

print "Created $testCount tests.\n";
printf ("Total duration estimated to be $totalDuration_sec seconds (%.1f hours).\n",$totalDuration_sec/3600);

exit;


sub WriteIniPreamble
	{
	
	my $cpuMeterThreadPriority = $configFile->{'DeviceUnderTest'}->{'CpuMeterThreadPriority'};
	$cpuMeterThreadPriority = 100 unless defined $cpuMeterThreadPriority;

	print INI "
[UCCControl]
$uccini
$control_bearer_pref

[CpuMeter]
EaterThreadPriority=$cpuMeterThreadPriority

[StartExeService]
svcname=ExeService
methodid=1
$control_bearer_pref

[StopExeService]
svcname=ExeService
methodid=2
$control_bearer_pref

[IPerfStop]
svcname=ExeService
methodid=10
call=Stop
num_params=0
$control_bearer_pref

";

	if($packetCapture)
		{
	print INI "
[StartPcapService]
svcname=PcapService
methodid=1
$control_bearer_pref

[StopPcapService]
svcname=PcapService
methodid=2
$control_bearer_pref

[StopCapture]
svcname=PcapService
methodid=10
call=StopCapture
num_params=0
expect_positive_return_code=true
$control_bearer_pref

";
		}
	}
	

sub WriteTestCase
	{
	my ($testCase)=(@_);

	my $testLabel = $testCase->{'label'};
	print("Writing test $testLabel..\n");

	my $testDuration = $testCase->{'DurationInSeconds'};
	$testDuration = $configFile->{'Test'}->{'DurationInSeconds'} unless defined($testDuration);
	$testDuration = 60 unless defined($testDuration); # default test length 60 sec
	$totalDuration_sec += $testDuration + 35; # 35 sec of communicating per test observed. agent setup cost added per agent a bit further down..
	
	my $useLowerLayerPacketGenerator = $testCase->{'UseLowerLayerPacketGenerator'};
	$useLowerLayerPacketGenerator = $configFile->{'Test'}->{'UseLowerLayerPacketGenerator'} unless defined($useLowerLayerPacketGenerator);
	if (defined($useLowerLayerPacketGenerator) && $useLowerLayerPacketGenerator)
		{
		$useLowerLayerPacketGenerator = "UseLowerLayerPacketGenerator=true";
		}
	else
		{
		$useLowerLayerPacketGenerator = "";
		}
	
	# add common stuff first...
	if($packetCapture)
		{
		print INI "
[StartCapture_$testLabel]
svcname=PcapService
methodid=10
call=StartCapture
num_params=1
param_name_1=outputFileName
param_value_1=$testLabel.pcap
expect_positive_return_code=true
$control_bearer_pref

";
		}
		
	my ($setupStage,$runStage,$runStage2,$stopStage,$processStage) = ('','','','','');
	my $inifile = "$configpath_target\\netperfte.ini";

	# separate base indices for sender/receiver. this means it's possible to
	#  create a loopback test using parallel sender/receiver
	my $receiverPortBase=$testBearerPortBase;
	my $senderPortBase=$testBearerPortBase;

	# now work through the agents in the test
	foreach my $agentStr (grep(/^(Sender|Receiver)/, sort { $a cmp $b } keys %$testCase))
		{
		$totalDuration_sec+=10;
		my($proto,$rate,$packetSizeInBytes)=(
			$testCase->{$agentStr}->{'Protocol'},
			$testCase->{$agentStr}->{'Rate'},
			$testCase->{$agentStr}->{'PacketSize'}
			);

		my $proto=uc($proto);
		if($proto ne 'UDP' && $proto ne 'TCP')
			{
			die ("Protocol [$proto] must be UDP or TCP\n");
			}

		if($rate ne '')
			{
			die ("Rate [$rate] must be a number in kbps\n") unless $rate=~/^\d+$/;
			}
		else
			{
			$rate=512;
			}
			
		if($packetSizeInBytes ne '')
			{
			die ("PacketSize [$packetSizeInBytes] must be a number of bytes\n") unless $packetSizeInBytes=~/^\d+$/;
			}
		else
			{
			$packetSizeInBytes=1470;
			}
		
		if($agentStr =~ /^Sender/)
			{
			my $senderTestPcPort=$senderPortBase;
			$senderPortBase++;
			
			my $iperfOpts='';
			if($proto eq 'UDP')
				{
				$iperfOpts = "-u";
				}

			print INI "
[${testLabel}_$agentStr]
TestDurationInSeconds=$testDuration
SamplingPeriodInMilliseconds=1000
SendProtocol=$proto
SendThroughputInKilobitsPerSecond=$rate
PacketSizeInBytes=$packetSizeInBytes
SendPeriodInMilliseconds=10
SendToAddress=$remote_IP
SendToPort=$senderTestPcPort
$useLowerLayerPacketGenerator
$test_bearer_pref

[IPerfStartRecv_${testLabel}_$agentStr]
svcname=ExeService
methodid=10
call=Start
num_params=3
param_name_1=cmd
param_value_1=iperf
param_name_2=args
param_value_2=-s -fk -p$senderTestPcPort -l$packetSizeInBytes $iperfOpts
param_name_3=output
param_value_3=IPerf_${testLabel}_${agentStr}_${rate}.txt
expect_positive_return_code=true
$control_bearer_pref

";
			$setupStage .= "RUN_TEST_STEP 50 netperfte SetupSender $inifile ${testLabel}_$agentStr
RUN_TEST_STEP 15 UCCControlTE RunCommand $inifile IPerfStartRecv_${testLabel}_$agentStr
DELAY 2000\n";
			$runStage .= "RUN_TEST_STEP 50 netperfte Start $inifile ${testLabel}_$agentStr\n";
			$stopStage = "RUN_TEST_STEP 20 netperfte Stop $inifile ${testLabel}_$agentStr\n".$stopStage;
			$processStage .= "RUN_TEST_STEP 10 netperfte Report $inifile ${testLabel}_$agentStr
RUN_TEST_STEP 10 netperfte Destroy $inifile ${testLabel}_$agentStr\n";
			}
		elsif($agentStr =~ /^Receiver/)
			{
			my $receiverDevicePort=$receiverPortBase;
			$receiverPortBase++;
			
			my $iperfOpts='';
			if($proto eq 'UDP')
				{
				$iperfOpts = "-u -b${rate}k";
				}
			print INI "
[${testLabel}_$agentStr]
TestDurationInSeconds=$testDuration
SamplingPeriodInMilliseconds=1000
ReceiveProtocol=$proto
ReceivePort=$receiverDevicePort
PacketSizeInBytes=$packetSizeInBytes
EchoReceivedData=false
$test_bearer_pref

[IPerfStartSend_${testLabel}_$agentStr]
svcname=ExeService
methodid=10
call=Start
num_params=3
param_name_1=cmd
param_value_1=iperf
param_name_2=args
param_value_2=-c $DUT_IP -fk -p$receiverDevicePort -l$packetSizeInBytes -t$testDuration $iperfOpts
param_name_3=output
param_value_3=IPerf_${testLabel}_${agentStr}_${rate}.txt
expect_positive_return_code=true
$control_bearer_pref

";		
			$setupStage .= "RUN_TEST_STEP 50 netperfte SetupReceiver $inifile ${testLabel}_$agentStr\n";
			$runStage .= "RUN_TEST_STEP 50 netperfte Start $inifile ${testLabel}_$agentStr\n";
			$runStage2 .= "RUN_TEST_STEP 15 UCCControlTE RunCommand $inifile IPerfStartSend_${testLabel}_$agentStr\nDELAY 2000\n";
			$stopStage .= "RUN_TEST_STEP 20 netperfte Stop $inifile ${testLabel}_$agentStr\n";
			$processStage .= "RUN_TEST_STEP 10 netperfte Report $inifile ${testLabel}_$agentStr
RUN_TEST_STEP 10 netperfte Destroy $inifile ${testLabel}_$agentStr\n";
			}
		}

	# make sure port numbers are unique and incremental through the test run.
	$testBearerPortBase=$senderPortBase>$receiverPortBase?$senderPortBase:$receiverPortBase;
	$testBearerPortBase++;

	# now write the script file
	my $inifile = "$configpath_target\\netperfte.ini";
	my($startProfiler,$stopProfiler,$copyProfilerData)=('','','');
	if($getProfile)
		{
		$startProfiler = 'RUN_TEST_STEP 50 netperfte StartProfile';
		$stopProfiler = 'RUN_TEST_STEP 50 netperfte StopProfile';
		$copyProfilerData = "RUN_UTILS copyfile C:\\profiler.dat C:\\profiler_${testLabel}.dat";
		}

	my($startCapture,$stopCapture,$startPcapService,$stopPcapService)=('','','','');
	if($packetCapture)
		{
		$startPcapService = "RUN_TEST_STEP 15 UCCControlTE RunCommand $inifile StartPcapService";
		$stopPcapService = "RUN_TEST_STEP 15 UCCControlTE RunCommand $inifile StopPcapService";
		$startCapture = "RUN_TEST_STEP 20 UCCControlTE RunCommand $inifile StartCapture_$testLabel";
		$stopCapture = "RUN_TEST_STEP 20 UCCControlTE RunCommand $inifile StopCapture";
		}

	my($setupCpuMeter,$startCpuMeter,$stopCpuMeter,$reportCpuMeter)=('','','','');
	if($useCpuMeter)
		{
		$setupCpuMeter = "RUN_TEST_STEP 200 netperfte SetupCpuMeter $inifile CpuMeter";
		$startCpuMeter = "RUN_TEST_STEP 50 netperfte Start $inifile CpuMeter";
		$stopCpuMeter = "RUN_TEST_STEP 10 netperfte Stop $inifile CpuMeter";
		$reportCpuMeter = "RUN_TEST_STEP 10 netperfte Report $inifile CpuMeter\nRUN_TEST_STEP 10 netperfte Destroy $inifile CpuMeter";
		}


	open(SCRIPT,">$scriptpath_local/$testLabel.script") or die;
	print SCRIPT "

LOAD_SUITE netperfte -SharedData
LOAD_SUITE UCCControlTE

START_TESTCASE $testLabel

$startPcapService
RUN_TEST_STEP 15 UCCControlTE RunCommand $inifile StartExeService
$setupCpuMeter
$setupStage

$runStage
$startCapture
$runStage2

$startCpuMeter
$startProfiler
DELAY ${testDuration}000
$stopProfiler
$stopCpuMeter

$stopStage
$stopCapture

$processStage
$reportCpuMeter
RUN_TEST_STEP 15 UCCControlTE RunCommand $inifile StopExeService
$stopPcapService

END_TESTCASE $testLabel
$copyProfilerData

";
	close SCRIPT;


	if($forTestDriver)
		{
		AddTestDriverTest("$testLabel",$testDuration);
		}
	else
		{
		print NETPERFBAT "$batchpathonboard\\${testLabel}.bat\n";
		AddBatch("$testLabel");
		}
	}	


sub AddBatch
	{
	my ($filename)=@_;

	open(BATCH,">$batchpath/$filename.bat");
	for(my $iter = 1; $iter <= $runIters; ++$iter)
		{
		my $iterSuffix = ($runIters > 1)? "_$iter": "";
		my $copyprofiler = $getProfile ? "move C:\\profiler_${filename}.dat $resultspath_target\\profiler_$filename${iterSuffix}.dat\n" : "";
		print BATCH "
testexecute $scriptpath_target\\$filename.script -d
move c:\\logs\\testexecute\\$filename.htm $resultspath_target\\$filename$iterSuffix.htm
$copyprofiler";
		}
	close BATCH;
	}

sub FindTestDriverVars
	{
	my @out = `TestDriver config`;

	my @xr= grep(/(XML root\s*=|Preference xmlRoot:)\s*(\S.+)$/, @out);
	$xr[0] =~ m/(XML root\s*=|Preference xmlRoot:)\s*(\S.+)$/ or die;
	my $testDriverXmlRoot = $2;
#	my $testDriverXmlRoot = 'M:\testdriver\XML';
#	if(! -d $testDriverXmlRoot) {$testDriverXmlRoot = 'L:\testdriver\XML';}
	$testDriverXmlRoot=~s{\\}{/}g;
#	$testDriverXmlRoot.='/xmlroot';
	$testDriverXmlRoot=~s{\\$}{};
	
	my @sr=grep(/(Source root\s*=|Preference sourceRoot:)\s*(\S.+)$/, @out);
	$sr[0] =~ m/(Source root\s*=|Preference sourceRoot:)\s*(\S.+)$/ or die;
	my $testDriverSourceRoot = $2;
	
	print("testDriverSourceRoot: $testDriverSourceRoot\n");

	my $thisDirForTestDriver=$currentDirectoryWhenScriptRun;
	$thisDirForTestDriver=~s{/}{\\}g;
	unless($thisDirForTestDriver =~ s/\Q$testDriverSourceRoot\E\\?//i)
		{
		die("You must run this from below TestDriver's source folder!!");
		}

	my $netperfSourceRootForTestDriver=$thisDirForTestDriver;
	$netperfSourceRootForTestDriver='.' if $netperfSourceRootForTestDriver eq '';
	
	if($runDir_local =~ /^\\/ || $runDir_local =~ /:/)
		{
		die("You mustn't specify such an interesting root path as $runDir_local for the output folder");
		}
	my $rootpathForTestDriver="$thisDirForTestDriver\\$runDir_local";
	
	my $configpath_localForTestDriver=$configpath_local;
	$configpath_localForTestDriver=~s{^\Q$runDir_local}{$rootpathForTestDriver};
	$configpath_localForTestDriver=~s{/}{\\}g;
	
	my $scriptpath_localForTestDriver=$scriptpath_local;
	$scriptpath_localForTestDriver=~s{^\Q$runDir_local}{$rootpathForTestDriver};
	$scriptpath_localForTestDriver=~s{/}{\\}g;

	my $epocrootAbsolute = GetEpocRootAbsolute();

	my $bindepsPathForTestDriver = "$epocrootAbsolute\\epoc32\\release\\$targetname\\$buildtype";
	$bindepsPathForTestDriver =~ s/\\+/\\/g;
	unless($bindepsPathForTestDriver =~ s/\Q$testDriverSourceRoot\E\\?//i)
		{
		die("Epoc binaries ($bindepsPathForTestDriver) must be somewhere below TestDriver's source folder ($testDriverSourceRoot)!!");
		}
	
	print("Xml root: $testDriverXmlRoot\n");
	print("netperf src root: $netperfSourceRootForTestDriver\n");
	print("rootpath for testdriver: $rootpathForTestDriver\n");
	print("configpath_local for testdriver: $configpath_localForTestDriver\n");
	print("scriptpath_local for testdriver: $scriptpath_localForTestDriver\n");
	print("bin deps for testdriver: $bindepsPathForTestDriver\n");

	return($testDriverXmlRoot,$netperfSourceRootForTestDriver,$rootpathForTestDriver,$configpath_localForTestDriver,$scriptpath_localForTestDriver,$bindepsPathForTestDriver);
	}

sub WriteTestDriverPreambles
	{
	mkdir("$runDir_local/xmlroot");

	# testdriver version >=2 xml follows
	open(TDTESTSET,">$runDir_local/xmlroot/xmlroot.driver") or die;
	print TDTESTSET qq{<?xml version="1.0" encoding="UTF-8"?>
<driver:driver xmlns:driver="http://www.symbian.com/TestDriver">
  <task name="xmlroot" timeout="100000">
    <task name="root">
      <task name="TestSet">
        <transferToSymbian>
          <transfer move="false" PCPath="\${sourceroot}\\$configpath_localForTestDriver\\netperfte.ini" SymbianPath="$configpath_target\\netperfte.ini"/>
        </transferToSymbian>
        <transferToSymbian>
          <transfer move="false" PCPath="\${sourceroot}\\$bindepsPathForTestDriver\\netperfte.exe" SymbianPath="c:\\sys\\bin\\netperfte.exe"/>
          <transfer move="false" PCPath="\${sourceroot}\\$bindepsPathForTestDriver\\networkemulatorcontrol.dll" SymbianPath="c:\\sys\\bin\\networkemulatorcontrol.dll"/>
          <transfer move="false" PCPath="\${sourceroot}\\$bindepsPathForTestDriver\\ucccontrolte.exe" SymbianPath="c:\\sys\\bin\\ucccontrolte.exe"/>
         </transferToSymbian>
        <task name="AllTests" timeout="20000">
};
	# RJL - possible future enhancement here: config file option to install profiler with testdriver..
	}
	

sub AddTestDriverTest
	{
	my ($name,$testDuration) = @_;

	# testdriver version >=2 xml follows
	print TDTESTSET qq{          <executeOnSymbian>
            <testExecuteScript PCPath="\${sourceroot}\\$scriptpath_localForTestDriver\\$name.script" SymbianPath="$scriptpath_target\\$name.script"/>
          </executeOnSymbian>
};
	if($getProfile)
		{
		print TDTESTSET qq{			<retrieveFromSymbian>
            <transfer move="true" PCPath="\${sourceroot}\\$rootpathForTestDriver\\results\\profiler_$name.dat" SymbianPath="C:\\profiler_${name}.dat"/>
          </retrieveFromSymbian>
};
		}
	}


sub WriteTestDriverPostambles
	{
	# testdriver version >=2 xml follows
	print TDTESTSET qq{        </task>
      </task>
    </task>
  </task>
</driver:driver>
};
	close TDTESTSET;
	}

	
sub WriteCommsdat()
	{
	my $template = $configFile->{'DeviceUnderTest'}->{'CommsdatTemplate'};
	return unless defined $template;

	$template=~/\.(xml|cfg)$/ or die "CommsdatTemplate must be an .xml or .cfg file";
	my $ext=$1;

	$template = GetAbsoluteFilePath($template);
	open(I, "$template") or die "Can't find specified commsdat template $template";

	unless (open(C,">$setupconfigpath_local/netperf.$ext"))
		{
		print "Can't write to file $setupconfigpath_local/netperf.$ext";
		return 0;
		}

	my $commsdatpathondevice = "$setupconfigpath_target\\netperf.$ext";
	$commsdatpathondevice =~ s{[\\/]+}{\\}g;
	print NPINIT "ced -i $commsdatpathondevice\n";
	
	while(<I>)
		{
		if(/====/)
			{
			s/====ETHERPDDFILENAME====/$DUT_etherPddFileName/;
			s/====ETHERPDDNAME====/$DUT_etherPddName/;
			s/====IPNETMASK====/$DUT_netmask/;
			s/====IPGATEWAY====/$DUT_gateway/;
			s/====IPADDR====/$DUT_IP/;
			}
		print C;
		}
	close C;
	
	return 1;
	}

sub WriteStatConfig()
	{
	die("Path on target for setup data [$setupDir_target] must be the root of some drive (e.g. E:\\)\n")
				unless($setupDir_target=~/^[a-zA-Z]\:\\?$/);
	mkdir("$setupDir_local/system");
	mkdir("$setupDir_local/system/data");
	open(STATINI,">$setupDir_local/system/data/stat.ini") or die;
	print STATINI "
[SectionOne]
comport=2
[SectionTwo]
transport=tcpip
";
	if($control_bearer_iap)
		{
		print STATINI "iap=$control_bearer_iap";
		}
	elsif($control_bearer_snap)
		{
		print STATINI "snap=$control_bearer_snap";
		}
	# otherwise write nothing => default connection

	print STATINI "
[SectionThree]
logging=0
";
	close STATINI;
	}


__END__


