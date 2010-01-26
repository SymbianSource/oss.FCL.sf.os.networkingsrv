This is the automatic regression test for netperf, to be used in the nightly build

Currently it just runs a couple of generated TEF scripts in loopback mode.

Testing the whole functionality (ie driven from netperf script) requires an environment to be set up on the build machine that may not be there e.g. SITK, extra perl modules

This however provides a basic regression test of the TEF plugin.

