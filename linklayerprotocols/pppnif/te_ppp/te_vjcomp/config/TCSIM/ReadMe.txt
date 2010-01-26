This directory contains all the TCSIM files that are generated to provide desired packets for Van Jacobson testing. They are meant to be generated once and are included in this directory for future reference.

The TCSIM files are not used directly in Van Jacobson test code but instead they are converted into libpcap format packet files using the following tools:

cvtppplog.bat file is used in conjunction with trimdump.pl to process pppdump logs into libpcap format.  
tcsim2pcap script converts the output of tcsim into a libpcap format packet file.

