
This folder is just here as reference to show how the scripts in te_netperf were generated.

generatescripts.bat uses netperf to generate the TEF scripts for te_netperf.

The generation step can't currently be done as part of overnight builds because the build machines don't all have everything netperf needs (e.g. SITK, perl modules - see HowTo for full TestController PC s/w requirements)

Note the loopback tests don't require UCC - so as not to result in failures during the running of the TEF scripts, we grep out the UCC lines (see generatescripts.bat). This requires GnuWin32 (or equivalent) grep.exe

