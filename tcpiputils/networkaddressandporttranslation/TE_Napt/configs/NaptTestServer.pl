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
#

#!C:\Perl\bin\perl
# file: NaptTestServer.pl

use IO::Socket;
use IO::Handle;
use IO::Select;
use Net::Ping;
#script will automatically close test cases after tests are finished.
#for some unknown reasons that i don't know of...the program seems to freeze if the data sent is not ended with a "\n"
#NaptTestServer listen port
$listenport = 4500;
#tcp send params
$ipaddr2sendtcp = "192.168.20.11";
#tcp port no
$port2sendtcp = 7;
#tcp message
$tcpmessage = "tHis is ecHo calling for narcIssus the tcp way \n";
$tcpbulkmessage = "tHis is ecHo calling for narcIssus the tcp way ";

#udp send params
$ipaddr2sendudp = "192.168.20.11";
#udp port no
$port2sendudp = 7;
#udp message
$udpmessage = "tHis is ecHo calling for narcIssus the udp way \n";
#udp invalid port
$invalidport2sendudp = 600000;
#test case 4
$ipaddress2resolv = "64.233.187.99";
#test case 6
$pingaddr = "192.168.20.11";
$add_buffer="";
$route_delete_command="";
$route_add_command="";
$storeaddr = 0;
$total_tests = 11;
$testcount = 0;
$exitstatus = 0;



sub REAPER
{
	print "in reaper as the child died...........................\n";
	$waitedpid = wait;
	# loathe sysV: it makes us not only reinstate
	# the handler, but place it after the wait
	$SIG{CHLD} = \&REAPER;
}

$SIG{CHLD} = \&REAPER;

sub initfunction
{
	$listensock = new IO::Socket::INET (LocalPort => $listenport,
                              Proto     => 'tcp',
                              Listen    => 100,
                              Reuse     => 1,
							);
	die "Could not connect: $!" unless $listensock;
	# Count number of lines and will close napt test server	
	#test case one	
	while(1)
		{
		
		if($testcount == $total_tests)
			{
			last;
			}
		print "Listening for connections on port $listenport...........................\n";
			    					
			$newlistensock = $listensock->accept();
						
			print "Connection established..................................................\n";
			$pid = fork();
				die "fork() failed: $!" unless defined($pid);
			
			if($pid == 0)
				{
			$listensock->autoflush(1);
			
			while(defined ($recvbuf = <$newlistensock>))
				{
					chop($recvbuf);
					$newlistensock->autoflush(1);
					parsemessage($recvbuf);
					close(newlistensock);
					if($buffer eq "test10")
					{

					$exitstatus = 1;
					}
													

				}#while(defined ($buf = <$new_sock>) )
				exit(0);
			   }#if ends here	
		
		$testcount++;	
		}#while
	
}

sub sendresulttoepoc
{
}
sub RouteDelete
{
	#Route delete function
	$del_buffer=$_[0];
	chomp($del_buffer);
	@strings=split( /:delete_route/,$del_buffer);
	$route_delete_command ="route delete ".$ipaddr2sendtcp;
	print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
	system($route_delete_command);
}

sub RouteAdd
{
	#Route add function
	$add_buffer = $_[0];
	chomp($add_buffer);
	@strings=split(/:ethaddress/,$add_buffer);
	$route_delete_command ="route delete ".$ipaddr2sendtcp;
	print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
	system($route_delete_command);

	$route_add_command = "route add ". $ipaddr2sendtcp." MASK 255.255.255.255 ".$strings[0]." METRIC 3";
	print "\nAdding Route\n";
	print "",$route_add_command,"\n";
	system ($route_add_command);
}

sub PingGlobal
{
	print " \n Test Case :- Ping Global Interface. \n";
	$ping_buffer = $_[0];
	chomp($ping_buffer);
	@strings=split(/:pingglobal/,$ping_buffer);
	$pingaddr = $strings[0];
	$route_add_command = "route add ". $pingaddr." MASK 255.255.255.255 ". $storeaddr." METRIC 3";
	print "",$route_add_command,"\n";
	system ($route_add_command);
    &testcase6;
	$route_delete_command ="route delete ".$pingaddr;
	system($route_delete_command);
	print "\n.....................................End of test case............................ \n";

}
sub StoreAddr
{
	$ping_add = $_[0];
	chomp($ping_add);
	@strings=split(/:storeaddr/,$ping_add);
	$storeaddr = $strings[0];
}


sub parsemessage
{
	
	$buffer = $_[0];
	if ($buffer =~ /:ethaddress/)
	{
		RouteAdd($buffer);
	}
	elsif($buffer =~/:route_delete/)
	{
		RouteDelete($buffer);
	}
	elsif ($buffer =~ /:pingglobal/)
	{
		PingGlobal($buffer);
	}
	elsif ($buffer =~ /:storeaddr/)
	{
		StoreAddr($buffer);
	}
	elsif("test1" eq $buffer)
	{
		print "\n Test Case :- Send data over TCP socket to Echo Server running in Test Network \n";
		&testcase1;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test2" eq $buffer)
	{
		print "\n Test Case :- Send data over UDP Socket to Echo Server running in Test Network \n";
		&testcase2;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test3" eq $buffer)
	{
		print "\n Test Case :- Send data over UDP Socket to invalid port. \n";
		&testcase3;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test4" eq $buffer)
	{
		print "Test Case :- Resolve host name from IP address.\n";
		&testcase4;
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test5" eq $buffer)
	{
		print " \n Test Case :- Resolve IP addr from host name.\n";
		&testcase5;
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test6" eq $buffer)
	{
		print "Test Case :- Ping Echo Server in Test Network.\n";
		&testcase6;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test7" eq $buffer)
	{
		print " \n Test Case :- Open many TCP Sockets and Send data to Echo Server over each socket.\n";
		&testcase7;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test8" eq $buffer)
	{
		print " \n Test Case :- Send bulk data over TCP socket to Echo Server in Test Network.\n";
		&testcase8;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test9" eq $buffer)
	{
		print " \n Test Case :- Open,Send and Close TCP Sockets in different order.\n";
		&testcase9;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test10" eq $buffer)
	{
		print " \n Test Case :- NAPT Timer Expires. \n";
		&testcase10;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	elsif("test11" eq $buffer)
	{
		print " \n Test Case :- Send data over global socket and Napt socket \n";
		&testcase1;
		$route_delete_command ="route delete ".$ipaddr2sendtcp;
		print "\n Deleting Route \n"; print "",$route_delete_command,"\n";
		system($route_delete_command);
		print "\n.....................................End of test case............................ \n";
	}
	else
	{	
		print "invalid entry \n";
	}
}

#DESCRIPTION:Sending TCP packets to echo server in Test Network.
sub testcase1
{
	
	$result = "0";
	
	if($tcpsendsock = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
			                             Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:TCP socket could not be created. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}	
	
	if(print $tcpsendsock $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:TCP socket unable to send. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}
	
	$answertcpserver = <$tcpsendsock>;
	print $answertcpserver;
	close ($tcpsendsock);
	sendresult($result);
}

#DESCRIPTION:testcase2
#Sending UDP packets to echo server in test n/w
sub testcase2
{
	#print "executing test case2 UDP\n";
	$result = 0;

	if($udpsendsock = new IO::Socket::INET (Proto => 'udp',
											))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:UDP socket could not be created. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}	
		
	$udpipaddr = inet_aton($ipaddr2sendudp);
	$udpaddr = sockaddr_in($port2sendudp, $udpipaddr);
	
$z = 0;
while($z < 11 )
{
	if(send($udpsendsock, $udpmessage, 0, $udpaddr) == length($udpmessage) )
	{
		$result = 1;
	}
	else
	{
		print "ERROR:Cannot send to $ipaddr2sendudp($port2sendudp): $!";
		$result = 0;
		sendresult($result);
		return;
	}
$z++;
}

	$MAXLEN = 100;
	if($udpserveraddr = recv($udpsendsock, $ansudpserver, $MAXLEN, 0))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:Cannot recv: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	($udpserverport, $udpserveripaddr) = sockaddr_in($udpserveraddr);
	print $ansudpserver;
	close ($udpsendsock);
	sendresult($result);
}

#DESCRIPTION:testcase3
#Sending UDP packets to invalid port
sub testcase3
{
	#print "executing test case3 invalid UDP\n";
	$result = 0;

	if($udpsendsock = new IO::Socket::INET (Proto => 'udp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:UDP socket could not be created. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}	
		
	$udpipaddr   = inet_aton($ipaddr2sendudp);
	$udpaddr = sockaddr_in($invalidport2sendudp, $udpipaddr);
	
	if(send($udpsendsock, $udpmessage, 0, $udpaddr) == length($udpmessage) )
	{
		$result = 1;
	}
	else
	{
		print "ERROR:Cannot send to $ipaddr2sendudp($port2sendudp): $!";
		$result = 0;
		sendresult($result);
		return;
	}
	$MAXLEN = 100;
	if($udpserveraddr = recv($udpsendsock, $ansudpserver, $MAXLEN, 0))
	{
		$result = 0;
	}
	else
	{
		print "ans is:",$ansudpserver,"\n";
		print "ERROR:Cannot recv: $!\n";
		close ($udpsendsock);
		$result = 1;
		sendresult($result);
		return;
	}

	($udpserverport, $udpserveripaddr) = sockaddr_in($udpserveraddr);
	print $ansudpserver;
	close ($udpsendsock);
	sendresult($result);

}

#DESCRIPTION:testcase4
#Resolve IP address to host name
sub testcase4
{
	print "Test case 4\n";
	$result = 1;
		sendresult($result);

}

#DESCRIPTION:testcase5
#Resolve host name  to IP address
sub testcase5
{
	$ip_command="ipconfig /all  > dns.txt";
	system($ip_command);
	open (DNSFILE,"< dns.txt") or  die("Could not open log file.");
	undef $/;
	@ips;
	my($lines) = <DNSFILE>; # read file into single variable
	my @values= split(':',$lines);
	for ($i =0; $i<=$#values ; $i++)
	{
		if ($values[$i] =~/DNS Servers/)
		{
			@ips = split(' ',$values[$i+1]);
			foreach $ip (@ips)
			{
				if ($ip =~ /[0-9]+/)
				{
														
					$dns_route_add_command = "route add ".$ip." MASK 255.255.255.255 ".$storeaddr." METRIC 3";
					#RouteAdd($epoc_ip.$ip);
					print "\n$dns_route_add_command";
					system($dns_route_add_command);
					
				}	
			}
		}
	}
	
    $host2resolv = "www.google.com";
	$result = 0;
	if($hostip = gethostbyname($host2resolv) )
	{
		$result = 1;	
	}
	else
	{
		print "failed :(\n";
		sendresult($result);
		return;
	}
	
	for $ip (@ips)
	{
		if ($ip =~ /[0-9]+/)
		{
			$dns_route_delete_command = "route delete ".$ip;
			print $dns_route_delete_command;
			system($dns_route_delete_command);
		}	

	}	
	close(DNSFILE);

	print "",inet_ntoa($hostip),"\n";
	sendresult($result);
	
}

#DESCRIPTION:testcase6
#Ping someone
sub testcase6
{
	#print "executing test case6 ping \n";
	$result = 0;
	$pinghandle = Net::Ping->new("icmp",10,24);
	for ($i=0;$i<10;$i++)
	{
		if($pinghandle->ping($pingaddr) )
		{
			$result = 1;
			print "ping successful\n";
			last;
		}
	}
	sendresult($result);
}

#DESCRIPTION:testcase7
#Opening 5 tcp sockets and send data(not the most eligent way of doing it..need to improve upon it)
sub testcase7
{
	#print "executing test case7 open 5 sockets and send data \n";
	$result = 0;

	if($tcpsendsock1 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock1:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}
	sleep 1;
	if($tcpsendsock2 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock2:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	if($tcpsendsock3 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock3:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	if($tcpsendsock4 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock4:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	if($tcpsendsock5 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock5:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;
	
	#socket opening is over need to send data through all of them...printing the response as it comes
		
	if(print $tcpsendsock1 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock1:TCP socket unable to send. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}
	
	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock1>;
	print $answertcpserver;

	sleep 1;

	if(print $tcpsendsock2 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock2:TCP socket unable to send. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}

	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock2>;
	print $answertcpserver;

	sleep 1;

	if(print $tcpsendsock3 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock3:TCP socket unable to send. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}
	
	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock3>;
	print $answertcpserver;

	sleep 1;

	if(print $tcpsendsock4 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock4:TCP socket unable to send. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}

	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock4>;
	print $answertcpserver;

	sleep 1;

	if(print $tcpsendsock5 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock5:TCP socket unable to send. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}

	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock5>;
	print $answertcpserver;

	sleep 1;
	
	sendresult($result);
}

#DESCRIPTION:testcase8
#Sending bulk data
sub testcase8
{
	#print "executing test case8 sending bulk \n";
	$result = 0;
	$count = 0;
	$longtcpmessage = "";
	#Generating data long enough for TCP socket
	while($count<400)
	{
		$longtcpmessage = join($longtcpmessage,'',$tcpbulkmessage);
		$count++;
	}
	$longtcpmessage = join($longtcpmessage,'',$tcpmessage);
	#print "message is=  $longtcpmessage \n";

	if($tcpsendsock = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
			                             Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:TCP socket could not be created. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}	
	
	if(print $tcpsendsock $longtcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:TCP socket unable to send. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}
	
	$answertcpserver = <$tcpsendsock>;
	#print "Echo message is=  $answertcpserver \n";

	close ($tcpsendsock);

	sendresult($result);
}

#DESCRIPTION:testcase9
#Random opening sending and closing data
sub testcase9
{
	#print "executing test case9 random sending data \n";
	#Opening 5 sockets 
	$result = 0;
	$answertcpserver = "";
	
	print "opening socket no 1, 2, 3, 4 and 5\n";

	if($tcpsendsock1 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock1:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}
	
	sleep 1;
	
	if($tcpsendsock2 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	
	}
	else
	{
		print "ERROR:tcpsendsock2:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	if($tcpsendsock3 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock3:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	if($tcpsendsock4 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	
	}
	else
	{
		print "ERROR:tcpsendsock4::TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	if($tcpsendsock5 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	
	}
	else
	{
		print "ERROR:tcpsendsock5::TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	#here we have socket fds tcpsendsock1,tcpsendsock2,tcpsendsock3,tcpsendsock4,tcpsendsock5
	
	#sending message through sockets 5, 2 and 4
	if(print $tcpsendsock5 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock5:TCP socket unable to send from $tcpsendsock5. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}
	
	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock5>;
	print $answertcpserver;

	if(print $tcpsendsock2 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock2:TCP socket unable to send from $tcpsendsock2. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}

	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock2>;
	print $answertcpserver;

	if(print $tcpsendsock4 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:tcpsendsock4:TCP socket unable to send from $tcpsendsock4. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}
	
	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock4>;
	print $answertcpserver;

	#closing 5 2 1 3
	print "closing socket no 5, 2, 1 and 3\n";
	close ($tcpsendsock5);
	close ($tcpsendsock2);
	close ($tcpsendsock1);
	close ($tcpsendsock3);
	
	#opening 5 and 2
	if($tcpsendsock5 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:reopen tcpsendsock5:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	if($tcpsendsock2 = new IO::Socket::INET (PeerAddr => "$ipaddr2sendtcp",
										 PeerPort => "$port2sendtcp",
										 Proto    => 'tcp',
										))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:reopen tcpsendsock2:TCP socket could not be created by. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	sleep 1;

	#sending message through sockets 5, 2 and 4
	print "sending message 5 2 and 4 \n";
	if(print $tcpsendsock5 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:resend tcpsendsock5:TCP socket unable to send from $tcpsendsock5. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}

	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock5>;
	print $answertcpserver;

	if(print $tcpsendsock2 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:resend tcpsendsock2:TCP socket unable to send from $tcpsendsock2. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}

	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock2>;
	print $answertcpserver;

	if(print $tcpsendsock4 $tcpmessage)
	{
		$result = 1;
	}
	else
	{
		print "ERROR:resend tcpsendsock4:TCP socket unable to send from $tcpsendsock5. Reason: $!\n";	
		$result = 0;
		sendresult($result);
		return;
	}

	$answertcpserver = "";
	$answertcpserver = <$tcpsendsock4>;
	print $answertcpserver;

	#closing 5 4 2 1 
	print "closing socket no 5, 4, 2 and 1...in tht order\n";
	close ($tcpsendsock5);
	close ($tcpsendsock4);
	close ($tcpsendsock2);
	close ($tcpsendsock1);

	sendresult($result);
}

#DESCRIPTION:testcase10
#Sending UDP packets to echo server in test and sleep for 10 secs n/w
sub testcase10
{
	#print "executing test case10 UDP send and sleep\n";
	$result = 0;

	if($udpsendsock = new IO::Socket::INET (Proto => 'udp',
											))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:UDP socket could not be created. Reason: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}	
		
	
	$udpipaddr = inet_aton($ipaddr2sendudp);
	$udpaddr = sockaddr_in($port2sendudp, $udpipaddr);
	
	if(send($udpsendsock, $udpmessage, 0, $udpaddr) == length($udpmessage) )
	{
		$result = 1;
	}
	else
	{
		print "ERROR:Cannot send to $ipaddr2sendudp($port2sendudp): $!";
		$result = 0;
		sendresult($result);
		return;
	}

	$MAXLEN = 100;
	if($udpserveraddr = recv($udpsendsock, $ansudpserver, $MAXLEN, 0))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:Cannot recv: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	($udpserverport, $udpserveripaddr) = sockaddr_in($udpserveraddr);
	print $ansudpserver;
	
	print "Sleeping for 10 secs\n";	
	sleep 10;
	
	if(send($udpsendsock, $udpmessage, 0, $udpaddr) == length($udpmessage) )
	{
		$result = 1;
	}
	else
	{
		print "ERROR:Cannot send to $ipaddr2sendudp($port2sendudp): $!";
		$result = 0;
		sendresult($result);
		return;
	}

	$MAXLEN = 100;
	if($udpserveraddr = recv($udpsendsock, $ansudpserver, $MAXLEN, 0))
	{
		$result = 1;
	}
	else
	{
		print "ERROR:Cannot recv: $!\n";
		$result = 0;
		sendresult($result);
		return;
	}

	($udpserverport, $udpserveripaddr) = sockaddr_in($udpserveraddr);
	print $ansudpserver;
	
	close ($udpsendsock);
	sendresult($result);
}

sub sendresult
{
	$testresult = $_[0];
	print $newlistensock $testresult;
	print "Test case result:$testresult\n";
	
}

initfunction;
