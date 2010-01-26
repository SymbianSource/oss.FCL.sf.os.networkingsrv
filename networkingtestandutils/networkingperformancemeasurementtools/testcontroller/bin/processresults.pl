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
# Given a folder full of results from netperfte, processes it into a summary
# .csv file
# 
#

#!perl

use strict;
use Cwd;
use File::Find;
use File::Spec;
use Statistics::Descriptive;

my $scriptlocation;
BEGIN{$scriptlocation=$0; $scriptlocation=~s{[\\/][^\\/]+$}{}; push(@INC, $scriptlocation); }
use netperf;


my $binpath=$0;
$binpath=~s{[/\\][^/\\]+$}{};
#die $binpath;

my $workingDir=shift;
unless (defined $workingDir)
	{
	$workingDir = cwd();
	}


die "can't open results folder $workingDir" unless (-d $workingDir);

SetLogDir($workingDir);

my $configFilePath=shift;
my $resultsPathInConfigDir=$configFilePath;
unless (-f $resultsPathInConfigDir)
	{
	$resultsPathInConfigDir='results.csv';
	}
	else
	{
	$resultsPathInConfigDir=~s/ini$/results.csv/;
	}

my $resultsPathInConfigDir = File::Spec->rel2abs($resultsPathInConfigDir);

my $resultsPathInResultsDir = $resultsPathInConfigDir;
$resultsPathInResultsDir =~s{^.*?([^\\/]+)$}{$1};
NetperfLog("Planned destination results file: $resultsPathInResultsDir ..\n");

chdir $workingDir or die "can't open results folder $workingDir";
$workingDir = cwd();

my $resultsPathInResultsDir = File::Spec->rel2abs( $resultsPathInResultsDir );


NetperfLog("Looking for results under $workingDir ..\n");

{
	my @printableResults=();
	my @files = GetFiles();
	foreach my $file (@files)
	{
	my $friendlyFileName=GetAbsoluteFilePath(Cwd::cwd() ."/".$file);
	NetperfLog("found file $friendlyFileName ..\n");
	my $testCaseName = $file;
		my $cpuUsed;
		my $cpuUsedSd;
		my $cpuCnt;
		# get test case name from file name
		$testCaseName =~ m/(.*)[\\\/](.*)\.htm/i;
		$testCaseName = $2;

		NetperfLog("Digesting results from $testCaseName.htm...\n");

		# there is only one cpu section for each log file
		($cpuUsed, $cpuUsedSd, $cpuCnt) = ExtractCpu($file);

		# get list of receivers/senders from log
		my @agents = ExtractAgents($file);
		foreach my $agent (@agents)
		{
			my $directionWrtDevice;
			my $protocol;
			my $packetSize;
			my $requestedRateInKbps;
			my $stackTopRate;
			my $stackTopRateSd;
			my $stackTopRateCnt;
			my $stackTopLostPackets;
			my $stackTopPackets;
			my $stackBottomRate;
			my $stackBottomRateSd;
			my $stackBottomRateCnt;
			my $stackBottomLostPackets;
			my $tcPcRate;
			my $tcPcJitter;
			my $tcPcLostPackets;
			my $tcPcTotalPacketCount;
			my $stackTopToBottomMinDelay;
			my $stackTopToBottomMaxDelay;
			my $stackTopToBottomMeanDelay;
			my $stackTopToBottomMeanDelaySd;
			my $stackTopJitter;

			$agent =~ /($testCaseName)_(\w+)/;
			my $agentLabel = $2;
			NetperfLog("  Digesting results for agent $agentLabel:\n");

			($directionWrtDevice, $protocol, $packetSize, $requestedRateInKbps) = ExtractAgentInfo($file, $agent);
			($stackTopRate, $stackTopRateSd, $stackTopRateCnt, $stackTopLostPackets,
			 $stackBottomRate, $stackBottomRateSd, $stackBottomRateCnt, $stackBottomLostPackets,
			 $stackTopToBottomMinDelay, $stackTopToBottomMaxDelay, $stackTopToBottomMeanDelay, $stackTopToBottomMeanDelaySd
			 ) = ExtractDeviceRate($file, $agent, $directionWrtDevice);
			$requestedRateInKbps = ExtractReqRate($agent);
			($stackTopJitter) = ExtractJitter($file,$agent);
			($stackTopPackets) = ExtractSummaryStatistics($file,$agent);
			($tcPcRate,$tcPcJitter,$tcPcLostPackets,$tcPcTotalPacketCount) = ExtractPcRate($agent, $directionWrtDevice, $protocol, $requestedRateInKbps);

			my $csv = sprintf ("%s,%s,%s,%s,%d,%d, %.2f,%.2f,%d, %d,%s,%d,%d,%d,%d, %d,%s,%s,%s, %d,%d,%d,%d\n",
					$testCaseName,$agentLabel,$directionWrtDevice,$protocol,$packetSize,$requestedRateInKbps,
					$cpuUsed,$cpuUsedSd,$cpuCnt,
					$stackTopRate,$stackTopJitter,$stackTopRateSd,$stackTopRateCnt,$stackTopPackets,$stackTopLostPackets,
					$tcPcRate,$tcPcJitter,$tcPcTotalPacketCount,$tcPcLostPackets,
					$stackTopToBottomMinDelay,$stackTopToBottomMaxDelay,$stackTopToBottomMeanDelay,$stackTopToBottomMeanDelaySd);
			
			push @printableResults, $csv;
		}
	}
	
	if(@printableResults)
		{
		open(OUT,">$resultsPathInResultsDir") or die ("can't open file $resultsPathInResultsDir for writing");
		print OUT "test_case_name,agent_label,direction_w.r.t._device,protocol,client_transfer_unit_size_(bytes),requested_client_rate_(kbps),".
			"cpu_used_(percent),cpu_used_s.d.,cpu_readings_taken,".
			"stack_top_rate_(kbps),stack_top_jitter_(usec),stack_top_rate_s.d.,stack_top_rate_readings_taken,stack_top_total_packet_count,stack_top_lost_packets,".
			"test_controller_PC_rate_(kbps),test_controller_PC_jitter_(usec),test_controller_total_packet_count,test_controller_PC_lost_packets,".
			"stack_top_to_bottom_min_delay_(usec),stack_top_to_bottom_max_delay_(usec),stack_top_to_bottom_mean_delay_(usec),stack_top_to_bottom_mean_delay_s.d.\n";
		foreach my $line (@printableResults)
			{
			print OUT $line;
			}
		close OUT;
		NetperfLog("Results were stored to file $resultsPathInResultsDir ..\n");
		}
	else
		{
		NetperfLog "No printable results in test run!\n";
		}
}

if(open(IN,$resultsPathInResultsDir))
	{
	if(open(OUT,">$resultsPathInConfigDir"))
		{
		while(<IN>){print OUT}
		close IN;
		close OUT;
		NetperfLog("Results were also stored to file $resultsPathInConfigDir ..\n");
		}
	}

exit;

########################

my @files = ();




sub FileFind()
{
	my $file = $File::Find::name;
	$file =~ /[\/\\]([^\/\\]+)$/;
	my $fname = $1;
	return unless -f $fname;

	if ($file =~ m/\.htm$/i)
	{
		if (FileContains($fname,"Total client packets transferred"))
		{
			push(@files, $file);
		}
	}	
	
	if($fname =~ m/^run\d+.*html?$/)
	{
		my $fileAbs = File::Spec->rel2abs($fname);

		NetperfLog("TestExecute summary at $fileAbs \n");
	}
	
}

sub FileContains($)
{
	my($file,$str)=(@_);

	my $found=0;
	if (open(FILECONTAINSI, $file))
	{
		while (<FILECONTAINSI>)
		{
			if (/$str/)
			{
				$found=1;
				last;
			}
		}
		close FILECONTAINSI;
	}
	return $found;
}
	
sub GetFiles
{
	@files = ();

	find(\&FileFind, '.');

	return @files;
}

sub ExtractAgents($)
{
	my ($file) = @_;
	my @agents;

	if (open(LOGFILE, $file))
	{
		while (<LOGFILE>)
		{
			# search only for REPORT test step
			if (/^\d\d:\d\d:\d\d:\d+\s+\S+\s+run_test_step\s\d+\snetperfte\sreport\s\S+\s(\w+)/i)
			{
				my $agentLabel = $1;
				push @agents, $agentLabel if ($1 =~ /\w+(sender|receiver)\w*/i);
			}
		}
		close LOGFILE;
	}

	return @agents;
}

sub ExtractAgentInfo($$)
{
	my ($file, $agent) = @_;
	my ($direction, $protocol, $packetsize, $rate)=("unknown","unknown","unknown","unknown");
	my $found = 0;

	if($agent =~ /\w+(sender|receiver)\w*/i)
	{
		if ($1 =~ m/sender/i)
		{
			$direction = "send";
		}
		else # receive
		{
			$direction = "receive";
		}
	}

	if (open(LOGFILE, $file))
	{
		while (<LOGFILE>)
		{
			if (/^\d\d:\d\d:\d\d:\d+\s+\S+\s+run_test_step\s\d+\snetperfte\ssetup(sender|receiver)\s\S+\s(\w+)/i)
			{
				last if $found;
				next unless ($agent =~ m/$2/i);
				$found = 1;
			}

			next unless $found;

			if (/^\d\d:\d\d:\d\d:\d+\s+INFO\s-\s+\d+\s\S+\s\d+ will use (\d+) as packetSizeInBytes/i)
			{
				$packetsize = $1;
			}
			elsif (/^\d\d:\d\d:\d\d:\d+\s+INFO\s-\s+\d+\s\S+\s\d+ ini read.+$agent\s($direction)protocol\s(\w+)/i)
			{
				$protocol = lc($2);
			}
		}
		close LOGFILE;
	}

	return ($direction, $protocol, $packetsize, $rate);
}

sub ExtractData($$$)
{
	my ($file, $agent, $dataSet) = @_;
	my $foundCommand = 0;
	my $foundData = 0;
	my @lines;

	if (open(LOGFILE, $file))
	{
		while (<LOGFILE>)
		{
			if (/\srun_test_step\s\d+\snetperfte report \S+\s$agent/i)
			{
				last if $foundCommand;
				$foundCommand = 1;
			}
			next unless $foundCommand;

			if (/$dataSet begin/)
			{
				$foundData = 1;
			}
			elsif (/$dataSet end/)
			{
				$foundData = 0;
			}
			elsif ($foundData && /^\d\d:\d\d:\d\d:\d+\s+INFO\s+-\s+\d+\s+\S+\s+\d+\s+(.+)\s*$/)
			{
				my $line = $1;
				##NetperfLog("LINE: [$line]\n");
				$line =~ s/(\d),(\d)/$1$2/g;
				push @lines, $line;
			}
		}
		close LOGFILE;
	}

	return @lines;
}

sub ExtractCpu($)
{
	my ($file)=@_;
	my @lines = ExtractData($file, "CpuMeter", "CPU usage data");
	my @cpuUsed;

	foreach my $line (@lines)
	{
		next unless ($line =~ /\d/ && $line =~ /^[\d,\.\s]+$/);
		my @data = split(/[,\s]+/, $line);
		my $cpuIdle = $data[3];
		$cpuIdle = 100 if ($cpuIdle>100);

		my $cpuUsed = 100 - $cpuIdle;
		push @cpuUsed, $cpuUsed;
	}
	close I;

	my $stat = Statistics::Descriptive::Full->new();
	$stat->add_data(\@cpuUsed);
	my $mean  = $stat->mean();
	my $sd    = sqrt($stat->variance());
	my $count = $stat->count();

	NetperfLog("    cpu load: mean ${mean}\%, sd $sd, count $count\n");
	return ($mean, $sd, $count);
}

sub ExtractDeviceRate($$$)
{
	my ($file, $agent, $direction)=@_;
	my @lines = ExtractData($file, $agent, "packets data");
	my @times;
	my @ratesTop;
	my @ratesBottom;
	my $lostPktsTop = 0;
	my $lostPktsBottom = 0;
	my @delaysMin;
	my @delaysMean;
	my @delaysMax;

	foreach my $line (@lines)
	{
		next unless ($line =~ /\d/ && $line =~ /^[\d,\.\s]+$/);
		my @data = split(/[,\s]+/, $line);
		my $timeDelta = $data[1];
		my $byteDeltaTop = $data[3];
		my $kbitDeltaTop = $byteDeltaTop / 1000 * 8;

		my $byteDeltaBottom;
		my $kbitDeltaBottom;
		my $stackDelayMin;
		my $stackDelayMean;
		my $stackDelayMax;

		if ($direction eq "send")
		{
			if (@data >= 6)
			{
				$byteDeltaBottom = $data[5];
				$kbitDeltaBottom = $byteDeltaBottom / 1000 * 8;
				$lostPktsBottom += $data[6];
			}

			if (@data >= 9)
			{
				$stackDelayMin  = $data[7];
				$stackDelayMean = $data[8];
				$stackDelayMax  = $data[9];
			}
		}
		else
		{
			$lostPktsTop += $data[5];

			if (@data >= 8)
			{
				$byteDeltaBottom = $data[7];
				$kbitDeltaBottom = $byteDeltaBottom / 1000 * 8;
				$lostPktsBottom += $data[8];
			}

			if (@data >= 11)
			{
				$stackDelayMin  = $data[9];
				$stackDelayMean = $data[10];
				$stackDelayMax  = $data[11];
			}
		}

		##NetperfLog "$line\n";
		##NetperfLog("[td $timeDelta][bd $byteDelta][kd $kbitDelta]\n");
		my $rateTop = 0;
		my $rateBottom = 0;
		if ($timeDelta != 0.0)
		{
			push @times, $data[0];
			$rateTop = $kbitDeltaTop / $timeDelta;
			push @ratesTop, $rateTop;
			if (defined $kbitDeltaBottom)
			{
				$rateBottom = $kbitDeltaBottom / $timeDelta;
				push @ratesBottom, $rateBottom;
			}

			push @delaysMin,  $stackDelayMin  if defined $stackDelayMin;
			push @delaysMean, $stackDelayMean if defined $stackDelayMean;
			push @delaysMax,  $stackDelayMax  if defined $stackDelayMax;
		}
	}

	# trim the first and last sampling periods, which are useless due to setup bumpiness
	shift @ratesTop;
	shift @ratesBottom;
	shift @delaysMin;
	shift @delaysMean;
	shift @delaysMax;
	pop @ratesTop;
	pop @ratesBottom;
	pop @delaysMin;
	pop @delaysMean;
	pop @delaysMax;

	my $statTop = Statistics::Descriptive::Full->new();
	$statTop->add_data(\@ratesTop);
	my $meanTop  = $statTop->mean();
	my $sdTop    = sqrt($statTop->variance());
	my $countTop = $statTop->count();

	my $statBottom = Statistics::Descriptive::Full->new();
	$statBottom->add_data(\@ratesBottom);
	my $meanBottom  = $statBottom->mean();
	my $sdBottom    = sqrt($statBottom->variance());
	my $countBottom = $statBottom->count();

	my $delayMin = 0;
	if (@delaysMin > 0)
	{
		my $statDelaysMin = Statistics::Descriptive::Full->new();
		$statDelaysMin->add_data(\@delaysMin);
		$delayMin = $statDelaysMin->min();
	}

	my $delayMax = 0;
	if (@delaysMax > 0)
	{
		my $statDelaysMax = Statistics::Descriptive::Full->new();
		$statDelaysMax->add_data(\@delaysMax);
		$delayMax = $statDelaysMax->max();
	}

	my $statDelaysMean = Statistics::Descriptive::Full->new();
	$statDelaysMean->add_data(\@delaysMean);
	my $delayMean   = $statDelaysMean->mean();
	my $delayMeanSd = sqrt($statDelaysMean->variance());

	NetperfLog("    rate-top:    mean ${meanTop}kbps, sd $sdTop, count $countTop, lost_packets $lostPktsTop\n");
	NetperfLog("    rate-bottom: mean ${meanBottom}kbps, sd $sdBottom, count $countBottom, lost_packets $lostPktsBottom\n");
	NetperfLog("    stack-delay: mean ${delayMean}us, sd $delayMeanSd, min ${delayMin}us, max ${delayMax}us\n");
	return ($meanTop, $sdTop, $countTop, $lostPktsTop, $meanBottom, $sdBottom, $countBottom, $lostPktsBottom,
			$delayMin, $delayMax, $delayMean, $delayMeanSd);
}

sub ExtractJitter($$)
{
	my ($file, $agent)=@_;
	my @lines = ExtractData($file, $agent, "Jitter timings in microseconds");
	my @periods=();

	foreach my $line (@lines)
		{
		next unless ($line =~ /\d/ && $line =~ /^[\d,\.\s]+$/);
		while($line=~s/^\s*([\d,\.]+)//)
			{
			push @periods, $1;
			}
		}

	return "n/c" unless(@periods);

	my $jitterCalc = Statistics::Descriptive::Full->new();
	$jitterCalc->add_data(\@periods);

	my $meanPeriod  = $jitterCalc->mean();
	my $packetFreq = 1000000/($meanPeriod?$meanPeriod:1);
	my $periodSd    = sqrt($jitterCalc->variance());
	my $periodCount = $jitterCalc->count();

	NetperfLog("    mid-run packet period sample:    mean ${meanPeriod}us, sd(jitter) ${periodSd}us, count $periodCount\n");
	return (int($periodSd+0.5));
}

sub ExtractSummaryStatistics($$)
{
	my ($file, $agent)=@_;
	my @lines = ExtractData($file, $agent, "Summary statistics");
	my $totalPacketsTransferred="?";

	foreach my $line (@lines)
		{
		if($line=~/Total client packets transferred: (\d+)/)
			{
			$totalPacketsTransferred=$1;
			}
		}

	NetperfLog("    summary statistics:  total client packets counted $totalPacketsTransferred\n");
	return ($totalPacketsTransferred);
}

sub ExtractReqRate($)
{
	my ($agent) = @_;
	my $rate = 0;

	foreach my $file (<Iperf_*.txt>)
	{
		if ($file =~ /^iperf_($agent)_(\d+).txt$/i)
		{
			$rate = $2;
			last;
		}
	}

	return $rate;
}

sub ExtractPcRate($$$$)
{
	my ($agent, $direction, $protocol, $reqRate) = @_;
	my $rate = 0;
	my $jitter = 'n/a';
	my $lostPackets = 'n/a';
	my $totalPackets = 'n/a';

	my $fname = "iperf_$agent"."_$reqRate.txt";
	if (open(FILE, $fname))
	{
		while (<FILE>)
		{
			# e.g. [124]  0.0-10.0 sec  1281 KBytes  1047 Kbits/sec  0.000 ms    0/  892 (0%)
			if (s{^.+sec.+KBytes\s+([\d\.]+)\sKbits/sec}{})
			{
				$rate = $1;
				if (m{\s+([\d\.]+)\s+ms\s+(\d+)\s*/\s*(\d+)\s+})
				{
					($jitter,$lostPackets,$totalPackets) = ($1,$2,$3);
					$jitter *= 1000; # we want this in microsec
				}
				last;
			}
		}
		close FILE;
	}

	NetperfLog("    tc: rate ${rate}kbps");
	NetperfLog(", jitter ${jitter}us, lost packets $lostPackets, total packets $totalPackets") if $totalPackets;
	NetperfLog("\n");
	return ($rate, $jitter, $lostPackets, $totalPackets);
}

