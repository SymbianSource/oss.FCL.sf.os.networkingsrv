/*
  anvlppp.c: PPP tests for ANVL
  (C) Copyright 1994, 1995, 1996 Midnight Networks Inc.
  (C) Copyright 1996,1997,1998,1999 Teradyne Midnight Networks Inc.
  
	Portions Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).

  $Header$

  $Log$
*/

#include "midnight.h"
#include "anvlppp.h"
#include "anvppput.h"
#include "ahdlcpkt.h"
#include "anvlutil.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


/**** Structs, etc. Decls ****/

enum LCPBogusConfigureOptions_e {
  NONE=0,
  REVERSE_OPTIONS,
  OPTION_MISSING,
  BOGUS_VALUES,
  EXTRA_OPTION,
  LCP_BOGUS_END
};

struct LCPBogusConfigureState_s {
  ubyte responseCode;
  boolean bogusIdent;
  boolean bogusLengthOK;
  ubyte2 bogusLength;
  enum LCPBogusConfigureOptions_e bogusData;
};

struct PPPCheckCompressionState_s {
  boolean receivedPkt;
  boolean fcsCorrect;
  boolean addressControlCompressed;
  boolean protocolCompressed;
};

/* 0 may not be rejected as bogus, and a whole bunch of tests will
   fail if so, so at least this is a localization of BOGUS code */
#define LCP_BOGUS_CODE 0
/* this value was changed from 500 because many implementations will not
   allow you to configure the value below 576 due to IPX */
#define LCP_DIFFERENT_MRU 576
#define MAX_CONFIG_REQUESTS (LCP_DEFAULT_MAX_CONFIGURE * 20)
#define MAX_ECHO_DATA \
	(LCP_DEFAULT_MRU - (LCP_HDR_LEN + 4)) /* 4 for magicnum length */
#define LCP_DIFFERENT_ACCM 0xA5A5A5A5
#define NUM_OF_CHARS 0x20
#define LCP_DIFFERENT_MAGIC_NUM 0x12345678
#define RESTART_TIMER_PERCENT .1
#define HUGE_LENGTH 9999
/* several tests use this in the hopes that the value will be NAK'ed */
#define BOGUS_AUTH_PROTOCOL 9
#define LCP_CONFIG_REQ_TIME 10 /* seconds to wait */


struct TimingStats_s{
  ubyte lcpType;
  ubyte4 numSeen;
  Time_t times[MAX_CONFIG_REQUESTS];
};

struct ProtocolReject_s {
  boolean sawOne;
  ubyte2 protocol;
};

/**** Function and Handler Decls ****/

static void PPPValidateConfig(ANVLConfig_t *config, NetConn_t **pppCon,
							  boolean *echoSend);

ANVLPacketHandler_t LCPBogusConfigureHandler;
void LCPSendBogusResponse(NetConn_t *n, LCPForm_t *lcp,
						  struct LCPBogusConfigureState_s *cfgState);

static ANVLPacketHandler_t LCPGrabRawEchoHandler;
static ANVLPacketHandler_t LCPTimingHandler;
static boolean ValidateTimingStats(struct TimingStats_s *s,
								   Time_t *initialTime, Time_t *allowance);
static boolean LCPEchoAYT(NetConn_t *n, ubyte4 magic, byte *testNum);
static ANVLPacketHandler_t LCPRejHandler;

static ANVLPacketHandler_t PPPCheckCompressionHandler;
static ANVLPacketHandler_t NCPIgnoreNakHandler;
static boolean LCPWatchForTerminateOrConfigure(NetConn_t *n);
static ANVLPacketHandler_t LCPWatchForTerminateOrConfigureHandler;

static void
PPPTestsRunPart2(ANVLConfig_t *config, byte *protocol, NetConn_t *pppConn,
				 Time_t requestTimeOut, Time_t echoRequestTimeOut,
				 boolean echoSend);


/* start DUT */

void DUTstart(unsigned nConfigID)
{
 int status;
 ubyte ack[30];
 char   configCmd[20];

  /*the config command is of form startN$
 where N - is the number of the configuration (see EPOC client spec)*/
  sprintf(configCmd,"start%d$",nConfigID);

  if(!DUTSendConfigCommand((ubyte *)configCmd)) 
  {
    status = FALSE;
  }

  if(!DUTConfigAck(ack, 30))  
  {
    status = FALSE;
  }
  else 
  {
  /*	if (strcmp(ack, "start0$", 7) == 0) */
	{
		  /* ack received correctly */
	}
  }
}

 /*>>

  (void) PPPValidateConfig(ANVLConfig_t *config, NetConn_t **pppConn,
                           boolean *echoSend);

  REQUIRES:
 
  DESCRIPTION:
  Gets PPP interfaces.  

  Required interfaces:
	one PPP interface

  Optional interfaces:
    NONE

  Reasons that certain tests can be disqualified:
    PPP interface is bit synchronous
	PPP interface does not support IP
	the device under test does not support sending Echo-Request packets

  Unless only documentation is requested, requires a PPP interface. 
  Certain tests will be disqualified if the DUT does not support 
  certain PPP options or if the interface is synchronous.

  ARGS:
  config            ANVL config file specified by user in the command line (in)
  pppConn           the PPP interface (out)
  echoSend          does the device under test support sending Echo-Request
                    packets (out)

  RETURNS:

<<*/

static void
PPPValidateConfig(ANVLConfig_t *config, NetConn_t **pppConn, 
				  boolean *echoSend)
{
  byte *which;
  /* disqualify tests unless there is an interface with PPP and IP */
  boolean disqualifyTests = TRUE;
  IPIF_t *ipif = 0;

  *pppConn = 0;
  *echoSend = FALSE;

  if (config->docOnly){
	return;
  }

  /* Get a PPP interface which supports IP */
  ipif = ANVLConfigInterfaceNext(config, 0, ANVL_INTERFACE_PPP, 
								 ANVL_PROTOCOL_IP);
  if (ipif) {
	*pppConn = ipif->netConn;
  }
  
  /* If no IP or no IPIF, find own pppConn */
  if (!*pppConn) {
	*pppConn = ANVLConfigInterfaceNext(config, 0, ANVL_INTERFACE_PPP,
									   ANVL_PROTOCOL_NONE);
	if (!*pppConn) {
	  Error(FATAL, 
			"A PPP Interface line must appear in the configuration file\n");
	}
  }

  *echoSend = config->param.pppEchoSend;
  /* Disqualify tests if the DUT doesn't support sending Echo-Request 
	 packets */
  CheckPPPOptionTests(config, config->param.pppEchoSend,
					  "10.1 10.3 10.4 10.7 10.11 10.12 10.13 10.14 20.3",
					  "sending Echo-Request packets");

  /* Disqualify tests if the DUT doesn't support LQM */
  CheckPPPOptionTests(config, config->param.pppLQM, "15.1", "LQM");

  /* Disqualify tests for async PPP */
  CheckPPPOptionTests(config, ((*pppConn)->pktSrc->type == PKTSRC_AHDLC),
					  "13.2 18.2 18.3 18.4 20.1 20.2", "Asynchronous PPP");

  if (ipif) {
	disqualifyTests = FALSE;
  }

  if (disqualifyTests) {
	which = TestVectorClear(config,
							"9.2 9.10 17.2 17.3 17.4 17.5 18.2 18.4 20.1");
	if (StrLen(which)) {
	  Log(LOGMEDIUM, 
		  "\nThere is no PPP interface that supports IP.  Therefore\n"
		  "the following tests will not be run: %s\n", which);
	}
  }
}



/*>>
REQUIRES:
	all arguments must be properly initialized. 
DESCRIPTION
 	Procedure for testing DUT's handling of incoming LCP TerminateRequest.
 	TerminateRequest are sent and incoming TerminateAck are processed
 	to determine if DUT behaves correctly.
<<*/
static boolean PPPTestTerminateAck
        (
        ANVLConfig_t *config,	/* standard ANVL configuration */
        NetConn_t *pppConn,	/* PPP connection to use */
        int epocTestConfigNum,	/* Config Number on Epoc side */
        int testSteps,	/* Steps in this test */
        int lastTermAckStep,	/* Last step where Terminate Ack expected */
        int baseTimeout,	/* Base timeout: < Terminate Ack timeout */
        int extendedTimeout		/* timeout to make DUT leave the STOPPING state */
        )
{
	boolean status = FALSE; /* status of the test, TRUE -> Pass */ 
    int correctTermAckNum = 1; /* expected for this step */
    int step = 0;
    LCPForm_t *lcp;
    ubyte identifier;
	
    NCPEstConnState_t *state;
	
	HandlerID_t hand;
	
	struct TimingStats_s stats;
     
    Time_t timeOut = { LCP_DEFAULT_RESTART_TIMER, 0 };
	
    timeOut.sec = baseTimeout;

	identifier = NCPGetNextIdentifier();
    lcp = LCPFormCreate();

	FORM_SET_FIELD(lcp, code, lcpTypeTerminateRequest);
	FORM_SET_FIELD(lcp, identifier, identifier);
        
	
	DUTstart(epocTestConfigNum);/*see config IDs spec on epoc client side*/
	
    state = NCPEstConnStateCreate(LCP);
	
    status = LCPEstConn(pppConn, state);
	if(!status){
		Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
		stats.lcpType = lcpTypeTerminateAck;
	  	stats.numSeen = 0;
	  	state->userData = &stats;

	  	hand = ANVLHandlerInstall(pppConn, PPP, 
								pppTypeLCPPacked, PPP_TYPE_LEN,
								LCPTimingHandler, state);

	  	DUTLCPClose(config);
 
        for(step = 0; step < testSteps; ++step){
           	Log(LOGMEDIUM, "Sending TerminateRequest Id(%u)\n", identifier);
            LCPSend(pppConn,lcp, 0, 0);
                
                
            if(step == lastTermAckStep){ 
            	/* Give DUT a plenty of time to send the last Terminate Ack,
                time out, and terminate. I.e. DUT is down for the next step.*/
                Log(LOGMEDIUM, "Lengthening Timeout.\n");
                timeOut.sec = extendedTimeout;
            }

	  		LogWaiting("for Terminate-Ack packets", &timeOut);
	  		RunHandlers(&timeOut);

        	if(step > lastTermAckStep){ /* DUT should be down, and must not respond */
              		correctTermAckNum = 0; 
           	}
        	
        	status = (stats.numSeen == correctTermAckNum);
			if(!status){
				Log(LOGMEDIUM,
		       	"! Number of Terminate-Ack received (%lu) "
		       	"did not equal correct number (%u)\n",
		       	stats.numSeen, correctTermAckNum);
	  	    	break; /* Test failure */
        	}
	  		else{
				Log(LOGMEDIUM, "Received %lu Terminate-Ack\n", stats.numSeen);
	  		}
                
       		stats.numSeen = 0; /* reset for the next iteration */ 
		} 
		HandlerRemove(pppConn, hand, PPP);
    }
      
    NCPEstConnStateDestroy(state);
    DUTSendConfigCommand((ubyte *) "stop$");
    Free(lcp);
    
    return status;
}
 








void
PPPTestsRun(ANVLConfig_t *config)
{
  boolean status, echoSend;
  byte *protocol, *testNum;
  NetConn_t *pppConn;
  Time_t requestTimeOut = { LCP_CONFIG_REQ_TIME, 0 };
  Time_t echoRequestTimeOut;

	  Log(LOGMEDIUM, "! 2 Unable to open PPP connection\n");
  TEST_SETUP(pppSetupCommand,
    "ANVL Test Suite PPP Family (ppp of ppp, pppauth, rfc1471, rfc1473)\n"
    "RFC 1661:  'The Point-to-Point Protocol (PPP)'\n"
    "(William Allen Simpson, Daydreamer, July 1994)\n"
    "RFC 1662:  'PPP in HDLC-like Framing'\n"
    "(William Allen Simpson, Daydreamer, July 1994)\n",
	"This test suite requires that the workstation running ANVL be\n"
	"connected to the DUT with a serial cable. The DUT should be\n"
	"configured to have all default values for its options. Some\n"
	"tests require LQM, IP (if available in this version of ANVL) or\n"
	"a LAN interface to also be configured.\n");

  /* Setup useful vars */
  protocol = "PPP";
  PPPValidateConfig(config, &pppConn, &echoSend);
  /* How long to wait for an echo request, some DUTs send them periodically */
  echoRequestTimeOut.sec = config->param.pppEchoWait;

 Log(LOGMEDIUM, "! 6 Unable to open PPP connection\n");
  BEGIN_TEST(
     TEST_NUM
     "1.1",
     TEST_DESCRIPTION
     "Restart Timer default is 3 seconds (with Configure-Requests)\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Send Configure-Requests every 3 secs (or doubling up to that)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	struct TimingStats_s stats;
	boolean tmpStatus;
	Time_t value = { LCP_DEFAULT_RESTART_TIMER, 0 };
	Time_t allowance = { 0, 500000 };


 	state = NCPEstConnStateCreate(LCP); 
	/* Collect as many as we can */
	state->sendReqAfterNReqs = state->sendAckAfterNReqs =
		LCP_DEFAULT_MAX_CONFIGURE - 1;

	state->userData = &stats;
	state->UserHandler = (PacketHandler_t *)LCPTimingHandler;
	stats.numSeen = 0;
	stats.lcpType = lcpTypeConfigureRequest;

 	DUTLCPOpen(config, pppConn); 
        DUTstart(0);/*see config IDs spec on epoc client side*/

 	tmpStatus = LCPEstConn(pppConn, state);

	if (!tmpStatus) {
	  Log(LOGMEDIUM, "Could not establish LCP Connection\n");
	}

	status = ValidateTimingStats(&stats, &value, &allowance);

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(state);

	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "1.2",
     TEST_DESCRIPTION
     "Restart Timer default is 3 seconds (with Terminate-Requests)\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Cause DUT to close connection\n"
     "-  DUT: Send Terminate-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Send Terminate-Requests every 3 secs (or doubling up to that)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	struct TimingStats_s stats;
	HandlerID_t hand;
	Time_t value = { LCP_DEFAULT_RESTART_TIMER, 0 },
           allowance = { 0, 500000 },
           timeOut = { LCP_DEFAULT_RESTART_TIMER*10, 0 };

	 /* Log(LOGMEDIUM, "! 1 Unable to open PPP connection\n");*/
	DUTstart(0);/*see config IDs spec on epoc client side*/
	/*  Log(LOGMEDIUM, "! 2 Unable to open PPP connection\n");*/

	state = NCPEstConnStateCreate(LCP);

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  stats.lcpType = lcpTypeTerminateRequest;
	  stats.numSeen = 0;
	  state->userData = &stats;
	  hand = ANVLHandlerInstall(pppConn, PPP, 
								pppTypeLCPPacked, PPP_TYPE_LEN,
								LCPTimingHandler, state);

	  DUTLCPClose(config);
   
	  DUTSendConfigCommand((ubyte *) "terminate$");

	  LogWaiting("for Terminate-Request packets", &timeOut);
	  RunHandlers(&timeOut);

	  HandlerRemove(pppConn, hand, PPP);

	  status = ValidateTimingStats(&stats, &value, &allowance);
	}
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(state);

	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "1.3",
     TEST_DESCRIPTION
     "Restart Timer MUST be configurable\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Configure Restart Timer to be different than 3 seconds\n"
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Send Configure-Requests every Restart-Timer secs\n"
	 "        (or doubling up to that)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	struct TimingStats_s stats;
	boolean tmpStatus;
	/* Something that isn't the default */
	Time_t value = { LCP_DEFAULT_RESTART_TIMER * 2, 0 },
           allowance = { 0, 500000 },
           longTime = {LCP_MAX_OPEN_TIME * 2, 0};

	DUTLCPSetRestartTimer(config, value.sec);

	DUTstart(1);/*see config IDs spec on epoc client side*/

	state = NCPEstConnStateCreate(LCP);
	/* Collect as many as we can */
	state->sendReqAfterNReqs = state->sendAckAfterNReqs =
		LCP_DEFAULT_MAX_CONFIGURE - 1;
	state->connTimeOut = longTime;
	state->userData = &stats;
	state->UserHandler = (PacketHandler_t *)LCPTimingHandler;
	stats.numSeen = 0;
	stats.lcpType = lcpTypeConfigureRequest;

	DUTLCPOpen(config, pppConn);

	tmpStatus = LCPEstConn(pppConn, state);

	if (!tmpStatus) {
	  Log(LOGMEDIUM, "! Could not establish LCP Connection\n");
	  status = FALSE;
	}
	else {
	  status = ValidateTimingStats(&stats, &value, &allowance);
	}
	  status = ValidateTimingStats(&stats, &value, &allowance);
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(state);
	/* Undo what we did in this test */
	DUTLCPSetRestartTimer(config, LCP_DEFAULT_RESTART_TIMER);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "1.4",
     TEST_DESCRIPTION
     "Max Terminate default is 2\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Cause DUT to close connection\n"
     "-  DUT: Send Terminate-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Send Terminate-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Drop connection\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	struct TimingStats_s stats;
	HandlerID_t hand;
	Time_t timeOut = { LCP_DEFAULT_RESTART_TIMER*10, 0 };

	DUTstart(0);/*see config IDs spec on epoc client side*/

	state = NCPEstConnStateCreate(LCP);

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  stats.lcpType = lcpTypeTerminateRequest;
	  stats.numSeen = 0;
	  state->userData = &stats;

	  hand = ANVLHandlerInstall(pppConn, PPP, 
								pppTypeLCPPacked, PPP_TYPE_LEN,
								LCPTimingHandler, state);

	  DUTLCPClose(config);
 
	  DUTSendConfigCommand((ubyte *) "terminate$");

	  LogWaiting("for Terminate-Request packets", &timeOut);
	  RunHandlers(&timeOut);

	  HandlerRemove(pppConn, hand, PPP);
	  
	  status = (stats.numSeen == LCP_DEFAULT_MAX_TERMINATE);
	  if(!status){
		Log(LOGMEDIUM,
			"! Number of Terminate-Requests sent (%lu) "
			"did not equal correct default (%u)\n",
			stats.numSeen, LCP_DEFAULT_MAX_TERMINATE);
	  }
	  else{
		Log(LOGMEDIUM, "Received %lu Terminate-Requests\n", stats.numSeen);
	  }
	}

	NCPEstConnStateDestroy(state);
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "1.5",
     TEST_DESCRIPTION
     "Max Terminate MUST be configurable\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Configure Max Terminate to be different than 2\n"
     "- ANVL: Open connection\n"
     "- ANVL: Cause DUT to close connection\n"
     "-  DUT: Send Terminate-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Send total of 'Max Terminate' Terminate-Requests\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Drop connection\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	struct TimingStats_s stats;
	HandlerID_t hand;
	Time_t timeOut = { LCP_DEFAULT_RESTART_TIMER*10, 0 };
	ubyte4 value = LCP_DEFAULT_MAX_TERMINATE*2;

	/** Must use config with Max-Terminate other than default. */
	DUTstart(11);/*see config IDs spec on epoc client side*/

	DUTLCPSetMaxTerminate(config, value);
	state = NCPEstConnStateCreate(LCP);

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  stats.lcpType = lcpTypeTerminateRequest;
	  stats.numSeen = 0;
	  state->userData = &stats;

	  hand = ANVLHandlerInstall(pppConn, PPP, 
								pppTypeLCPPacked, PPP_TYPE_LEN,
								LCPTimingHandler, state);

	  DUTLCPClose(config);
	
	  /** Make DUT negotiate connection termination.*/	
	  DUTSendConfigCommand((ubyte*) "terminate$");
		
	  LogWaiting("for Terminate-Request packets", &timeOut);
	  RunHandlers(&timeOut);

	  HandlerRemove(pppConn, hand, PPP);
	  
	  status = (stats.numSeen == value);
	  if(!status){
		Log(LOGMEDIUM,
			"! Number of Terminate-Requests sent (%lu) "
			"did not equal the configured value (%lu)\n",
			stats.numSeen, value);
	  }
	  else{
		Log(LOGMEDIUM, "Received %lu Terminate-Requests\n", stats.numSeen);
	  }
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(state);
	DUTLCPSetMaxTerminate(config, LCP_DEFAULT_MAX_TERMINATE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "1.6",
     TEST_DESCRIPTION
     "Max Configure default is 10\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Send total of 10 Configure-Requests\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Stop sending Configure-Requests\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	struct TimingStats_s stats;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	state = NCPEstConnStateCreate(LCP);
	/* Never respond */
	state->sendReqAfterNReqs = state->sendAckAfterNReqs =
		LCP_DEFAULT_MAX_CONFIGURE + 1;
	state->userData = &stats;
	state->UserHandler = (PacketHandler_t *)LCPTimingHandler;
	stats.numSeen = 0;
	stats.lcpType = lcpTypeConfigureRequest;

	DUTLCPOpen(config, pppConn);

	LCPEstConn(pppConn, state);

	status = (stats.numSeen == LCP_DEFAULT_MAX_CONFIGURE);
	if(!status){
	  Log(LOGMEDIUM,
		  "! Number of Configure-Requests sent (%lu) "
		  "did not equal correct default (%u)\n",
		  stats.numSeen, LCP_DEFAULT_MAX_CONFIGURE);
	}
	else{
	  Log(LOGMEDIUM, "Received %lu Configure-Requests\n", stats.numSeen);
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(state);

	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "1.7",
     TEST_DESCRIPTION
     "Max Configure MUST be configurable\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Configure Max Configure to be different than 10\n"
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Send total of 'Max Configure' Configure-Requests\n"
     "- ANVL: Do not respond\n"
     "-  DUT: Stop sending Configure-Requests\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	struct TimingStats_s stats;
	/* Pick some non-default value */
	ubyte4 value = 3;/*LCP_DEFAULT_MAX_CONFIGURE/2;*/

	DUTstart(2);/*see config IDs spec on epoc client side*/

	DUTLCPSetMaxConfigure(config, value);

	state = NCPEstConnStateCreate(LCP);
	/* Never respond */
	state->sendReqAfterNReqs = state->sendAckAfterNReqs = value + 1;
	state->userData = &stats;
	state->UserHandler = (PacketHandler_t *)LCPTimingHandler;

	stats.numSeen = 0;
	stats.lcpType = lcpTypeConfigureRequest;

	DUTLCPOpen(config, pppConn);

	LCPEstConn(pppConn, state);

	status = (stats.numSeen == value);
	if(!status){
	  Log(LOGMEDIUM,
		  "! Number of Configure-Requests sent (%lu) "
		  "did not equal the configured value (%lu)\n",
		  stats.numSeen, value);
	}
	else{
	  Log(LOGMEDIUM, "Received %lu Configure-Requests\n", stats.numSeen);
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(state);
	/* Undo the reconfig */
	DUTLCPSetMaxConfigure(config, LCP_DEFAULT_MAX_CONFIGURE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "1.8",
     TEST_DESCRIPTION
     "Max Failure default is 5\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p25 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request with option DUT will Nak\n"
     "-  DUT: Send Configure-Nak\n"
	 "- ANVL: Send total of 5 Configure-Requests that are the same\n"
     "-  DUT: Send total of 5 Configure-Naks\n"
	 "- ANVL: Send 1 additional Configure-Request that is the same\n"
     "-  DUT: Send Configure-Reject\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	/* Turn on Auth so there is something to Nak */
	DUTSetPPPPAP(config, TRUE);

	lcpState = NCPEstConnStateCreate(LCP);
	/* We should keep trying beyond the max */
	lcpState->maxConfigure = LCP_DEFAULT_MAX_FAILURE + 1;
	lcpState->UserHandler = (PacketHandler_t *)NCPIgnoreNakHandler;
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), authProtocol, 
				   BOGUS_AUTH_PROTOCOL);

	lcpStatus = LCPEstConn(pppConn, lcpState);

	if(lcpState->nakReceived != LCP_DEFAULT_MAX_FAILURE){
	  Log(LOGMEDIUM,
		  "! Received %lu Configure-Naks but expected %u\n",
		  lcpState->nakReceived, LCP_DEFAULT_MAX_FAILURE);
	  status = FALSE;
	}
	if(lcpState->rejReceived != 1){
	  Log(LOGMEDIUM,
		  "! Received %lu Configure-Rejects but expected 1\n",
		  lcpState->rejReceived);
	  status = FALSE;
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPPAP(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "1.9",
     TEST_DESCRIPTION
     "Max Failure MUST be configurable\n",
     TEST_REFERENCE
     "RFC 1661 s4.6 p25 Counters and Timers\n",
     TEST_METHOD
	 "- ANVL: Configure DUT with Max Failure to N (different than 5)\n"
     "- ANVL: Send Configure-Request with option DUT will Nak\n"
     "-  DUT: Send Configure-Nak\n"
	 "- ANVL: Send total of N Configure-Requests that are the same\n"
     "-  DUT: Send total of N Configure-Naks\n"
	 "- ANVL: Send 1 additional Configure-Request that is the same\n"
     "-  DUT: Send Code-Reject\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	ubyte4 value = LCP_DEFAULT_MAX_FAILURE/2;

	DUTstart(3);/*see config IDs spec on epoc client side*/

	/* Give it something to Nak */
	DUTSetPPPPAP(config, TRUE);
	DUTLCPSetMaxFailure(config, value);

	lcpState = NCPEstConnStateCreate(LCP);
	/* We should keep trying beyond the max */
	lcpState->maxConfigure = value + 1;
	lcpState->UserHandler = (PacketHandler_t *)NCPIgnoreNakHandler;
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), authProtocol, 
				   BOGUS_AUTH_PROTOCOL);

	lcpStatus = LCPEstConn(pppConn, lcpState);

	if(lcpState->nakReceived != value){
	  Log(LOGMEDIUM,
		  "! Received %lu Configure-Naks but expected %lu\n",
		  lcpState->nakReceived, value);
	  status = FALSE;
	}
	if(lcpState->rejReceived != 1){
	  Log(LOGMEDIUM,
		  "! Received %lu Configure-Rejects but expected 1\n",
		  lcpState->rejReceived);
	  status = FALSE;
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTLCPSetMaxFailure(config, LCP_DEFAULT_MAX_FAILURE);
	DUTSetPPPPAP(config, FALSE);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;


  /* SECTION 5.1 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "3.1",
     TEST_DESCRIPTION
     "An implementation wishing to open connection MUST transmit a\n"
     "Configure-Request\n",
     TEST_REFERENCE
     "RFC 1661 s5.1 p28 Configure-Request\n",
     TEST_METHOD
	 "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate fields\n"
     "        Code             1\n"
     "        Identifier       any\n"
     "        Length           packet length (<= 1500)\n"
     "        Options          type is 1-5, 7, 8, 17-19 (MP options)\n"
     "                         no default values\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPConfigOptionForm_t opt;
	LCPAndExpectState_t *state;
	ubyte *dataPtr;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();

	DUTLCPOpen(config, pppConn);

	state->ncpType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, 0, state);
	if (!pkt) {
	  status = FALSE;
	}
	else {
	  Log(LOGMEDIUM, "Validating Configure-Request packet\n");

	  /* check length */
	  if (state->ncpForm.length != (LCP_HDR_LEN + pkt->len)) {
		Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
			"length (%lu)\n",
			state->ncpForm.length, (LCP_HDR_LEN + pkt->len));
		status = FALSE;
	  }
	  
	  for(dataPtr = state->ncpForm.data; 
		  dataPtr < (state->ncpForm.data + state->ncpForm.dataLen); 
		  dataPtr += opt.length) {
		LCPConfigOptionToForm(dataPtr, &opt);
		/* if it is either reserved or zero of in that range between 8
           and 17 (the MP options) or greater than the highest known
           MP option, then fail */
		if ((opt.type == lcpConfigTypeReserved) ||
			(opt.type == 0) || 
			((opt.type > lcpConfigTypeACFC) && 
			 (opt.type < lcpConfigTypeMultilinkMRRU)) ||
			(opt.type > lcpConfigTypeMultilinkEndpointDiscriminator)){
		  Log(LOGMEDIUM, "! Incorrect option type %u in Options\n",
			  opt.type);
		  status = FALSE;
		}
		/* +++alan: should probably check no default values */
	  }
	}
	DUTPPPResetConn(config, pppConn);
	Free(state);
	Free(lcp);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;
  

  BEGIN_TEST(
     TEST_NUM
     "3.3",
     TEST_DESCRIPTION
     "The Identifier field MUST be changed whenever the content of the\n"
     "Options field changes.\n",
     TEST_REFERENCE
     "RFC 1661 s5.1 p28 Configure-Request\n",
     TEST_METHOD
	 "- SETUP: Give DUT non-default MRU\n"
     "- ANVL: Cause DUT to open connection with non-default MRU\n"
     "-  DUT: Send Configure-Request\n"
	 "- ANVL: Send Configure-Reject for MRU specifying default value\n"
     "-  DUT: Send Configure-Request\n"
	 "- ANVL: Check that id is different and Options changed\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	ubyte ident = 0;
	LCPAndExpectState_t *state;
	LCPConfigDataForm_t *opts, *optsOut;
	ubyte cfgBuf[MAX_PACKET_LEN];
	ubyte4 cfgLen;
	ubyte2 mru;

	DUTstart(4);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	optsOut = LCPConfigDataFormCreate();

	DUTSetPPPMRU(config, LCP_DIFFERENT_MRU);
	DUTLCPOpen(config, pppConn);

	state->ncpType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, 0, state);
	if (!pkt) {
	  status = FALSE;
	}
	else {
	  ident = state->ncpForm.identifier;
	  Log(LOGMEDIUM, "Configure-Request identifier is 0x%02X\n", ident);
	  LCPConfigDataToForm(pkt->data, pkt->len, opts);
	  if (!opts->mruOK) {
		Log(LOGMEDIUM, "! Configure-Request did not contain MRU option\n");
		status = FALSE;
	  }
	  else {
		mru = opts->mru;

		Log(LOGMEDIUM, "Sending Configure-Reject for MRU\n");
		FORM_SET_FIELD(optsOut, mru, opts->mru);
		cfgLen = NCPConfigDataBuild(pppConn,LCP,
									(NCPConfigDataForm_t *)optsOut,cfgBuf);
		CLEAR_DATA(lcp);
		FORM_SET_FIELD(lcp, code, lcpTypeConfigureReject);
		FORM_SET_FIELD(lcp, identifier, ident);
		FORM_SET_DATA(lcp, cfgBuf, cfgLen);
		
		/* Clear state out for another */
		Free(state);
		state = LCPAndExpectStateCreate();
		state->ncpType = lcpTypeConfigureRequest;
		pkt = LCPAndExpect(pppConn, lcp, state);
		if (!pkt) {
		  status = FALSE;
		}
		else {
		  CLEAR_DATA(opts); /* so we can reuse it */
		  LCPConfigDataToForm(pkt->data, pkt->len, opts);
		  if (opts->mruOK) {
			Log(LOGMEDIUM,
				"! Configure-Request contained MRU option\n");
			status = FALSE;
		  }
		  if (state->ncpForm.identifier == ident) {
			Log(LOGMEDIUM,
				"! Configure-Request identifier did not change (0x%02X)\n",
				state->ncpForm.identifier);
			status = FALSE;
		  }
		  else {
			Log(LOGMEDIUM, 
				"Configure-Request contains a different identifier (0x%02X)\n",
				state->ncpForm.identifier);
		  }
		}
	  }
	}
	DUTSetPPPMRU(config, LCP_DEFAULT_MRU);
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");	
	Free(opts);
	Free(optsOut);
	Free(state);
	Free(lcp);

  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "3.4",
     TEST_DESCRIPTION
     "Upon reception of Configure-Request, an appropriate reply MUST\n"
     "be transmitted.\n",
     TEST_REFERENCE
     "RFC 1661 s5.1 p28 Configure-Request\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request\n"
     "-  DUT: Send one of Configure-{Ack,Nak,Reject}\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();

	DUTLCPOpen(config, pppConn);

	Log(LOGMEDIUM, "Sending Configure-Request\n");
	FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);
	
	state->ncpType = 0;
	state->timeOut = requestTimeOut;
	state->ignoreType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, lcp, state);
	if (!pkt) {
	  status = FALSE;
	}
	else {
	  if (!(state->ncpForm.code == lcpTypeConfigureAck ||
			state->ncpForm.code == lcpTypeConfigureNak ||
			state->ncpForm.code == lcpTypeConfigureReject)) {
		Log(LOGMEDIUM,
			"! Did not receive an LCP Configure-{Ack,Nak,Reject}\n");
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
	Free(lcp);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "3.5",
     TEST_DESCRIPTION
     "Configure-Requests with bad lengths are ignored\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.1 p28 Configure-Request\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request with incorrect length\n"
     "-  DUT: Should not send Configure-{Ack,Nak,Reject}\n"
     "- CASE: length = 0-3\n"
     "- CASE: stated length > actual length\n"
     "- CASE: Options length = MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	NCPEstConnState_t *lcpState;
	ubyte4 i;
	ubyte2 bogusLength[] = { 0, 1, 2, 3, 45, 1500 };
	LCPAndExpectState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	state = LCPAndExpectStateCreate();
	
	DUTLCPOpen(config, pppConn);

	for (i=0; i<NUM_ELEMENTS(bogusLength); i++) {
	  Log(LOGMEDIUM,
		  "\nSending LCP Configure-Request with incorrect length %u\n",
		  bogusLength[i]);
	  
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);
	  FORM_SET_FIELD(lcp, length, bogusLength[i]);

	  state->ncpType = 0;
	  state->ignoreType = lcpTypeConfigureRequest;
	  state->expectedPkts = 0;
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  
	  if (pkt) {
		status = FALSE;
		Log(LOGMEDIUM, "! Received unexpected packet.\n");
	  }

	  /* Clear the bad length */
	  CLEAR_DATA(lcp);
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

	  /* Expect one Configure-Ack */
	  state->ncpType = lcpTypeConfigureAck;
	  state->ignoreType = 0;
	  state->expectedPkts = 1;
	  
	  Log(LOGMEDIUM, "Sending good Configure Request and expecting a response\n");
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  
	  if (!pkt){
		status = FALSE;
		Log(LOGMEDIUM, "! Did not receive expected Configure-Ack\n");
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
	NCPEstConnStateDestroy(lcpState);
	Free(lcp);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "3.6",
     TEST_DESCRIPTION
     "Configuration Options SHOULD NOT be included with default values\n",
     TEST_REFERENCE
     "RFC 1661 s5.1 p28 Configure-Request\n",
     TEST_METHOD
	 "- SETUP: Give DUT non-default MRU\n"
     "- ANVL: Cause DUT to open connection with non-boolean Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Nak for Option specifying default value\n"
     "-  DUT: Send Configure-Request without Option\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	NCPGrabOptionState_t *state;
	LCPConfigOptionForm_t *lcpConfig;
	ubyte2 mru;

	DUTstart(4);/*see config IDs spec on epoc client side*/
	/* 
	 * +++badger: Nak-ing MRUs may violate the spirit of the RFC
	 * (what does it mean when you do this?)  But everyone seems
	 * to allow it so...
	 */
	state = NCPGrabOptionStateCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	lcpConfig = LCPConfigOptionFormCreate();
	lcpState->UserHandler = NCPGrabOptionHandler;
	lcpState->maxFailure = 3; /* The connection won't open, make it be fast */
	state->optType = lcpConfigTypeMaximumReceiveUnit;
	state->cfgCode = lcpTypeConfigureRequest;
	state->grabLastPkt = TRUE;
	lcpState->userData = (void *)state;

	/* cause DUT to open a connection with a config option we can Nak */
	DUTSetPPPMRU(config, LCP_DIFFERENT_MRU);

	FORM_SET_FIELD(LCP_CFG(lcpState->nakOpts), mru,1000/* LCP_DEFAULT_MRU*/);

	/* This shouldn't come up, but we don't care either way */
	lcpStatus = ANVLPPPEstConn(config, pppConn, lcpState);

	if (state->cfgPkt == 0){
	  Log(LOGMEDIUM, "No MRU Option contained in Configure-Request\n");
	}
	else{
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, lcpConfig);

	  /* Type doesn't need checking, because that's how we found it */
	  if(lcpConfig->length != 4){
		Log(LOGMEDIUM, "! MRU Option has len = %u (should be 4)\n",
			lcpConfig->length);
	  }
	  Unpack(lcpConfig->data, "S", &mru);
	  if(mru != LCP_DEFAULT_MRU){
		Log(LOGMEDIUM, "! MRU Option has value = %u (configured as %u in "
			"Configure-Nak)\n",
			mru, LCP_DEFAULT_MRU);
	  }
	  else{
		Log(LOGMEDIUM, "! Configure-Request contained MRU Option with "
			"default value\n");
	  }
	  status = FALSE;
	  PacketDestroy(state->cfgPkt);
	}
	DUTSetPPPMRU(config, LCP_DEFAULT_MRU);

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	NCPEstConnStateDestroy(lcpState);
	Free(lcpConfig);
	Free(state);
  }
  END_TEST;

  /* SECTION 5.2 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "4.1",
     TEST_DESCRIPTION
     "If every Option and value received in Configure-Request is\n"
     "recognizable then the implementation MUST transmit Configure-Ack\n",
     TEST_REFERENCE
     "RFC 1661 s5.2 p29 Configure-Ack\n",
     TEST_METHOD
     "- ANVL: Send DUT Configuration-Request with some Options\n"
     "-  DUT: Send Configure-Ack\n"
     "- ANVL: Validate fields\n"
     "        Code             2\n"
     "        Identifier       same as in Request\n"
     "        Length           packet length (<= 1500)\n"
     "        Options          same as in Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t lcpForm;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	NCPGrabOptionState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureAck;
	lcpState->userData = (void *)state;

	Log(LOGMEDIUM,
		"Establishing PPP Connection requesting no options initially\n");
	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! Did not receive any Configure-Ack packets\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Configure-Ack packet\n");
	  LCPPacketToForm(state->cfgPkt, &lcpForm);

	  /* identifier is already checked by LCPEstConn() */

	  /* check length */
	  if (lcpForm.length != (LCP_HDR_LEN + lcpForm.dataLen)) {
		Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
			"length (%lu)\n",
			lcpForm.length, (LCP_HDR_LEN + lcpForm.dataLen));
		status = FALSE;
	  }

	  /* check options */
	  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B,
									 LCP_CFG(lcpState->localOptsLatest),
									 LCP_CFG(lcpState->localOptsAckd),
									 CHECK_OPTION_VALUES)) {
		Log(LOGMEDIUM,
			"! Configure-Ack Options do not match Configure-Request\n");
		status = FALSE;
	  }

	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);

	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(state);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "4.2",
     TEST_DESCRIPTION
     "Discard Configure-Acks whose Identifier and/or Option fields do not\n"
     "match those of the Configure-Request.\n",
     TEST_REFERENCE
     "RFC 1661 s5.2 p29 Configure-Ack\n",
     TEST_METHOD
	 "- ANVL: Cause DUT to open connection\n"
     "- ANVL: Negotiate LCP using Configure-Ack with bad Identifier field\n"
     "-  DUT: Send Configure-Request (should not reach LCP Open state)\n"
     "- ANVL: Negotiate LCP using Configure-Ack with extra option appended\n"
     "-  DUT: Send Configure-Request (should not reach LCP Open state)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;
	struct LCPBogusConfigureState_s ackState;
	boolean lcpStatus;
	ubyte4 i;


	state = LCPAndExpectStateCreate();

	for (i=0; i<2; i++) {
	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  lcpState->UserHandler = (PacketHandler_t *)LCPBogusConfigureHandler;
	  lcpState->userData = (void *)&ackState;
	  
	  CLEAR_DATA(&ackState);
	  ackState.responseCode = lcpTypeConfigureAck;
	  switch(i) {
	  case 0:
		ackState.bogusIdent = TRUE;
		break;
	  case 1:
		ackState.bogusData = EXTRA_OPTION;
		break;
	  default:
		Error(FATAL, "Bad test case number - %lu\n", i);
	  }
	  
	  Log(LOGMEDIUM, "\nEstablishing PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  /* At this point we should think that the connection is up, the
		 the DUT should think that it isn't because our Ack was bad */
	  if (!lcpStatus) {
		Log(LOGMEDIUM,
			"! Timed out trying to establish connection with DUT\n");
		status = FALSE;
	  }
	  else{
/*		Log(LOGMEDIUM,
			"Waiting for 1 more Configure-Request to indicate that DUT\n"
			" did not accept invalid Configure-Ack and that LCP Connection\n"
			" wasn't really established\n"); */
		state->ncpType = lcpTypeConfigureRequest;
		pkt = LCPAndExpect(pppConn, 0, state);
		if (!pkt) {
		  status = FALSE;
		}
	  }	  
	  DUTSendConfigCommand((ubyte *) "stop$");
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	}

	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "4.3",
     TEST_DESCRIPTION
     "The Options in the Configure-Ack contains the Options field\n"
     "copied from the Configure-Request\n",
     TEST_REFERENCE
     "RFC 1661 s5.2 p29 Configure-Ack\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request with agreeable Options\n"
     "-  DUT: Send Configure-Ack\n"
	 "- ANVL: Validate Options are exactly the same\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t lcpForm;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	NCPGrabOptionState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), mru, LCP_DIFFERENT_MRU);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), accm, LCP_DIFFERENT_ACCM);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum,
				   LCP_DIFFERENT_MAGIC_NUM);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), addrControlComp, TRUE);
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureAck;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! Did not receive any Configure-Ack packets\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Configure-Ack packet\n");
	  LCPPacketToForm(state->cfgPkt, &lcpForm);

	  /* identifier is already checked by LCPEstConn() */

	  /* check length */
	  if (lcpForm.length != (LCP_HDR_LEN + lcpForm.dataLen)) {
		Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
			"length (%lu)\n",
			lcpForm.length, (LCP_HDR_LEN + lcpForm.dataLen));
		status = FALSE;
	  }

	  /* check options */
	  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B,
									 LCP_CFG(lcpState->localOptsLatest),
									 LCP_CFG(lcpState->localOptsAckd),
									 CHECK_OPTION_VALUES)) {
		Log(LOGMEDIUM,
			"! Configure-Ack Options do not match Configure-Request\n");
		status = FALSE;
	  }

	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	NCPEstConnStateDestroy(lcpState);
	Free(state);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "4.4",
     TEST_DESCRIPTION
     "A Configure-Ack with invalid Options should be discarded\n",
     TEST_REFERENCE
     "RFC 1661 s5.2 p29 Configure-Ack\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with >1 Options\n"
     "- ANVL: Negotiate LCP using Configure-Ack with different\n"
	 "        values in Options\n"
     "-  DUT: Send Configure-Request (should not reach LCP Open state)\n"
     "- ANVL: Negotiate LCP using Configure-Ack with Options in\n"
     "        different order\n"
     "-  DUT: Send Configure-Request (should not reach LCP Open state)\n"
     "- ANVL: Negotiate LCP using Configure-Ack with last Option missing\n"
     "-  DUT: Send Configure-Request (should not reach LCP Open state)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;
	struct LCPBogusConfigureState_s ackState;
	boolean lcpStatus;
	ubyte4 i;


	DUTSetPPPACCM(config, LCP_DIFFERENT_ACCM);

	state = LCPAndExpectStateCreate();

	for (i=0; i<3; i++) {
	  DUTstart(5);/*see config IDs spec on epoc client side*/
	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  lcpState->UserHandler = (PacketHandler_t *)LCPBogusConfigureHandler;
	  lcpState->userData = (void *)&ackState;
	  
	  CLEAR_DATA(&ackState);
	  ackState.responseCode = lcpTypeConfigureAck;
	  switch(i){
	  case 0:
		ackState.bogusData = BOGUS_VALUES;
		break;
	  case 1:
		ackState.bogusData = REVERSE_OPTIONS;
		break;
	  case 2:
		ackState.bogusData = OPTION_MISSING;
		break;
	  default:
		Error(FATAL, "Bad test case number - %ld\n", i);
	  }
	  
	  Log(LOGMEDIUM, "\nEstablishing PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  /* At this point we should think that the connection is up, the
		 the DUT should think that it isn't because our Ack was bad */
	  if (!lcpStatus) {
		Log(LOGMEDIUM,
			"! Timed out trying to establish connection with DUT\n");
		status = FALSE;
	  }
	  else {
		Log(LOGMEDIUM,
			"Waiting for 1 more Configure-Request to indicate that DUT\n"
			" did not accept invalid Configure-Ack and that LCP Connection\n"
			" wasn't really established\n");
		state->ncpType = lcpTypeConfigureRequest;
		pkt = LCPAndExpect(pppConn, 0, state);
		if (!pkt) {
		  status = FALSE;
		}
	  }	  
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}

	DUTSetPPPACCM(config, LCP_DEFAULT_ACCM);
	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "4.5",
     TEST_DESCRIPTION
     "Configure-Ack with bad length is discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.2 p29 Configure-Ack\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "- ANVL: Negotiate LCP using Configure-Ack with incorrect length\n"
     "-  DUT: Send Configure-Request (should not reach LCP Open state)\n"
     "- CASE: length = 0-3\n"
     "- CASE: stated length > actual\n"
     "- CASE: Options field length = MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;
	struct LCPBogusConfigureState_s ackState;
	ubyte2 bogusLen[] = { 0, 1, 2, 3, 45, 1500 };
	ubyte4 i;
	boolean lcpStatus;


	state = LCPAndExpectStateCreate();

	for (i = 0; i <NUM_ELEMENTS(bogusLen); i++) {
	  DUTstart(5);/*see config IDs spec on epoc client side*/
	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  lcpState->UserHandler = (PacketHandler_t *)LCPBogusConfigureHandler;
	  lcpState->userData = (void *)&ackState;
	  
	  CLEAR_DATA(&ackState);
	  ackState.responseCode = lcpTypeConfigureAck;
	  ackState.bogusLengthOK = TRUE;
	  ackState.bogusLength = bogusLen[i];
	  
	  Log(LOGMEDIUM, "\nEstablishing PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  /* At this point we should think that the connection is up, the
		 the DUT should think that it isn't because our Ack was bad */
	  if (!lcpStatus) {
		Log(LOGMEDIUM,
			"! Timed out trying to establish connection with DUT\n");
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM,
			"Waiting for 1 more Configure-Request to indicate that DUT\n"
			" did not accept invalid Configure-Ack and that LCP Connection\n"
			" wasn't really established\n");
		state->ncpType = lcpTypeConfigureRequest;
		pkt = LCPAndExpect(pppConn, 0, state);
		if (!pkt) {
		  status = FALSE;
		}
	  }
	  
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}
	
	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "4.6",
     TEST_DESCRIPTION
     "Unsolicited Configure-Acks are discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.2 p29 Configure-Ack\n",
     TEST_METHOD
     "- ANVL: Send Configure-Ack, before connection is established\n"
     "-  DUT: Should not crash\n"
     "- ANVL: Establish connection\n"
	 "- ANVL: Close connection\n"
     "- ANVL: Send Configure-Ack, after connection is closed\n"
     "-  DUT: Should not crash\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	ubyte4 i;
	LCPAndExpectState_t *state;
	boolean tmpStatus;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();

	/* Send 3 unsolicited Configure-Acks in initial state */
	for (i=0; i<3; i++) {
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureAck);
	  
	  state->ncpType = 0;
	  state->ignoreType = lcpTypeConfigureRequest;
	  state->expectedPkts = 0;
	  Log(LOGMEDIUM, "Sending LCP Configure-Ack\n");
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  
	  if (pkt) {
		status = FALSE;
	  }
	}

	tmpStatus = LCPEstConn(pppConn, 0);
	if (!tmpStatus){
	  Log(LOGMEDIUM, "! Could not establish connection\n");
	  status = FALSE;
	}
	DUTPPPResetConn(config, pppConn);

	/* The last part of the TAL: send Configure-Ack after the
       connection is closed */
	state->ncpType = 0;
	state->ignoreType = lcpTypeConfigureRequest;
	state->expectedPkts = 0;
	Log(LOGMEDIUM, "Sending LCP Configure-Ack\n");
	pkt = LCPAndExpect(pppConn, lcp, state);
	
	if (pkt) {
	  status = FALSE;
	}
	DUTSendConfigCommand((ubyte *) "stop$");
	
	Free(state);
	Free(lcp);
  }
  END_TEST;

  /* SECTION 5.3 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "5.1",
     TEST_DESCRIPTION
     "If all Options are known, but some have unacceptable values a\n"
     "Configure-Nak is sent with appropriate values indicated\n",
     TEST_REFERENCE
     "RFC 1661 s5.3 p30 Configure-Nak\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request with unacceptable Options\n"
     "-  DUT: Send Configure-Nak\n"
     "- ANVL: Validate fields\n"
     "        Code              3\n"
     "        Identifier        same as in Request\n"
     "        Length            packet length (<= 1500)\n"
     "        Options           unnacceptable, new values, in order\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t lcpForm;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	NCPGrabOptionState_t *state;
	LCPConfigDataForm_t nakOpts;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), authProtocol, 
				   BOGUS_AUTH_PROTOCOL);
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureNak;
	lcpState->userData = (void *)state;

	Log(LOGMEDIUM,
		"Establish PPP connection Nak'ing with invalid Auth protocol 0x%04X\n",
		LCP_CFG(lcpState->localOpts)->authProtocol);
	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! Did not receive any Configure-Nak packets\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Configure-Nak packet\n");
	  LCPPacketToForm(state->cfgPkt, &lcpForm);
	  CLEAR_DATA(&nakOpts);
	  LCPConfigDataToForm(lcpForm.data, lcpForm.dataLen, &nakOpts);

	  /* identifier is already checked by LCPEstConn() */

	  /* check length */
	  if (lcpForm.length != (LCP_HDR_LEN + lcpForm.dataLen)) {
		Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
			"length (%lu)\n",
			lcpForm.length, (LCP_HDR_LEN + lcpForm.dataLen));
		status = FALSE;
	  }

	  /* check options */
	  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B,
									 LCP_CFG(lcpState->localOpts),
									 &nakOpts,
									 NOCHECK_OPTION_VALUES)) {
		Log(LOGMEDIUM,
			"! Configure-Nak Options do not match Configure-Request\n");
		status = FALSE;
	  }

	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	NCPEstConnStateDestroy(lcpState);
	Free(state);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "5.2",
     TEST_DESCRIPTION
     "Options which have no value fields (boolean Options) MUST use the\n"
     "Configure-Reject reply instead.\n",
     TEST_REFERENCE
     "RFC 1661 s5.3 p30 Configure-Nak\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request with unacceptable boolean Options\n"
     "-  DUT: Send Configure-Reject, not Configure-Nak\n"
     "- ANVL: Send Configure-Request with unacceptable Options of both types\n"
     "-  DUT: Send Configure-Reject for boolean Option only\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);

	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), reserved, TRUE);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else{
	  if (!lcpState->rejReceived) {
		Log(LOGMEDIUM, "! Did not receive any Configure-Reject packets\n");
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");

	DUTstart(0);/*see config IDs spec on epoc client side*/
	lcpState = NCPEstConnStateCreate(LCP);

	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), reserved, TRUE);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), authProtocol, 
				   BOGUS_AUTH_PROTOCOL);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else{
	  if (!lcpState->rejReceived) {
		Log(LOGMEDIUM, "! Did not receive any Configure-Reject packets\n");
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	NCPEstConnStateDestroy(lcpState);

  }
  END_TEST;
  
  
  BEGIN_TEST(
     TEST_NUM
     "5.4",
     TEST_DESCRIPTION
     "If Identifier in Configure-Nak does not match that of the last\n"
     "Configure-Request it MUST be discarded\n",
     TEST_REFERENCE
     "RFC 1661 s5.3 p31 Configure-Nak, p46 Magic-Number\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Magic-Number option\n"
     "-  DUT: Send Configure-Request with Magic-Number option\n"
     "- ANVL: Send Configure-Nak with different Identifier field\n"
     "-  DUT: Send unmodified Configure-Request (same ID field)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPConfigDataForm_t *opts, *saveOpts;
	ubyte4 cfgLen;
	ubyte cfgBuf[MAX_PACKET_LEN];


	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	saveOpts = LCPConfigDataFormCreate();

	DUTstart(10);/*see config spec  IDs on epoc side*/
    DUTLCPOpen(config, pppConn);
	
	state->ncpType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, 0, state);
	if (!pkt) {
	  /* LCPAndExpect should print the error message */
	  status = FALSE;
	}
	else {
	  LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
						  saveOpts);
	  
	  if (!saveOpts->magicNumOK) {
		Log(LOGMEDIUM, "! Did not receive a Magic-Number option\n");
		status = FALSE;
	  }
	  else {
		FORM_SET_FIELD(lcp, code, lcpTypeConfigureNak);
		
		/* Use different identifier for the Nak--it should be ignored */
		FORM_SET_FIELD(lcp, identifier, state->ncpForm.identifier + 1);
		
		/* send Configure-Nak with the _same_ magic number, not a
		   different one.  See RFC 1661 p. 46 for an explanation of
		   why this makes sense: it should cause the DUT to choose a
		   new magic number. */
		FORM_SET_FIELD(opts, magicNum, saveOpts->magicNum);
		cfgLen = NCPConfigDataBuild(pppConn, LCP,
									(NCPConfigDataForm_t *)opts, cfgBuf);
		FORM_UNSET_FIELD(lcp, length);
		FORM_SET_DATA(lcp, cfgBuf, cfgLen);
		
		Log(LOGMEDIUM,
			"Sending Configure-Nak with incorrect identifier 0x%08X\n"
			"The received identifier in the Configure-Request was 0x%08X\n",
			lcp->identifier, state->ncpForm.identifier);
		pkt = LCPAndExpect(pppConn, lcp, state);
		if (!pkt) {
		  status = FALSE;
		}
		else {
		  LCPConfigDataToForm(state->ncpForm.data, 
							  state->ncpForm.dataLen,
							  opts);
		  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B, 
										 saveOpts, opts,
										 CHECK_OPTION_VALUES)) {
			Log(LOGMEDIUM, "! Configure-Nak with bad identifier accepted\n");
			status = FALSE;
		  }
		}
	  }
	}
	DUTPPPResetConn(config, pppConn);

	DUTSendConfigCommand((ubyte *) "stop$");

	Free(saveOpts);
	Free(opts);
	Free(state);
	Free(lcp);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "5.5",
     TEST_DESCRIPTION
     "Reception of Configure-Nak indicates that new Configure-Request\n"
     "MAY be sent with the Options modified as in the Configure-Nak\n",
     TEST_REFERENCE
     "RFC 1661 s5.3 p30 Configure-Nak\n",
     TEST_METHOD
	 "- SETUP: Give DUT non-default MRU\n"
     "- ANVL: Cause DUT to open connection with Options\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Nak with new values\n"
     "-  DUT: Send Configure-Request with modified values\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	NCPGrabOptionState_t *state;
	LCPConfigOptionForm_t *lcpConfig;
	ubyte2 mru;

	/* 
	 * +++badger: Nak-ing MRUs may violate the spirit of the RFC
	 * (what does it mean when you do this?)  But everyone seems
	 * to allow it so...
	 */
	DUTstart(4);/*see config spec  IDs on epoc side*/

	state = NCPGrabOptionStateCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	lcpConfig = LCPConfigOptionFormCreate();
	lcpState->UserHandler = NCPGrabOptionHandler;
	state->optType = lcpConfigTypeMaximumReceiveUnit;
	state->cfgCode = lcpTypeConfigureRequest;
	state->grabLastPkt = TRUE;
	lcpState->userData = (void *)state;

	/* cause DUT to open a connection with a config option we can Nak */
	DUTSetPPPMRU(config, LCP_DIFFERENT_MRU);

	FORM_SET_FIELD(LCP_CFG(lcpState->nakOpts), mru, LCP_DIFFERENT_MRU + 1);

	lcpStatus = ANVLPPPEstConn(config, pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! Did not receive Configure-Request packet with MRU Option\n");
	  status = FALSE;
	}
	else{
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, lcpConfig);

	  /* Type doesn't need checking, because that's how we found it */
	  if(lcpConfig->length != 4){
		Log(LOGMEDIUM, "! MRU Option has len = %u (should be 4)\n",
			lcpConfig->length);
		status = FALSE;
	  }
	  Unpack(lcpConfig->data, "S", &mru);
	  if(mru != LCP_DIFFERENT_MRU + 1){
		Log(LOGMEDIUM, "! MRU Option has value = %u (configured as %u in "
			"Configure-Nak)\n",
			mru, LCP_DIFFERENT_MRU + 1);
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM, "Configure-Request contained correct MRU of %u\n", mru);
	  }
	  PacketDestroy(state->cfgPkt);
	}
	DUTSetPPPMRU(config, LCP_DEFAULT_MRU);

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(lcpConfig);
	Free(state);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "5.6",
     TEST_DESCRIPTION
     "A Configure-Nak may contain Options with default values if different\n"
     "from the Configure-Request\n",
     TEST_REFERENCE
     "RFC 1661 s5.3 p30 Configure-Nak\n",
     TEST_METHOD
	 "- SETUP: Give DUT non-default MRU\n"
     "- ANVL: Cause DUT to open connection with non-boolean Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Nak for Option specifying default value\n"
     "-  DUT: Send Configure-Request either with the default value or\n"
	 "        without Option\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	NCPGrabOptionState_t *state;
	LCPConfigOptionForm_t *lcpConfig;
	ubyte2 mru;

	/* 
	 * +++badger: Nak-ing MRUs may violate the spirit of the RFC
	 * (what does it mean when you do this?)  But everyone seems
	 * to allow it so...
	 */
	DUTstart(4);/*see config spec  IDs on epoc side*/
	state = NCPGrabOptionStateCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	lcpConfig = LCPConfigOptionFormCreate();
	lcpState->UserHandler = NCPGrabOptionHandler;
	state->optType = lcpConfigTypeMaximumReceiveUnit;
	state->cfgCode = lcpTypeConfigureRequest;
	state->grabLastPkt = TRUE;
	lcpState->userData = (void *)state;

	/* cause DUT to open a connection with a config option we can Nak */
	DUTSetPPPMRU(config, LCP_DIFFERENT_MRU);

	FORM_SET_FIELD(LCP_CFG(lcpState->nakOpts), mru, LCP_DEFAULT_MRU);
	lcpState->nakWithDefault = TRUE;     /* we're using defaults */

	lcpStatus = ANVLPPPEstConn(config, pppConn, lcpState);
	if (!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if (state->cfgPkt == 0){
	  Log(LOGMEDIUM, "No MRU Option contained in Configure-Request\n");
	}
	else{
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, lcpConfig);

	  /* Type doesn't need checking, because that's how we found it */
	  if(lcpConfig->length != 4){
		Log(LOGMEDIUM, "! MRU Option has len = %u (should be 4)\n",
			lcpConfig->length);
		status = FALSE;
	  }
	  Unpack(lcpConfig->data, "S", &mru);
	  if(mru != LCP_DEFAULT_MRU){
		Log(LOGMEDIUM, "! MRU Option has value = %u (configured as %u in "
			"Configure-Nak)\n",
			mru, LCP_DEFAULT_MRU);
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM, "Configure-Request contained correct MRU of %u\n", mru);
	  }
	  PacketDestroy(state->cfgPkt);
	}
	DUTSetPPPMRU(config, LCP_DEFAULT_MRU);

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	NCPEstConnStateDestroy(lcpState);
	Free(lcpConfig);
	Free(state);
  }
  END_TEST;


  BEGIN_TEST(
     TEST_NUM
     "5.9",
     TEST_DESCRIPTION
     "Unsolicited Configure-Naks are discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.3 p30 Configure-Nak\n",
     TEST_METHOD
     "- ANVL: (with connection open) Send Configure-Nak\n"
     "- ANVL: Check that connection is still up\n"
	 "- ANVL: Close connection\n"
     "- ANVL: Send Configure-Nak\n"
     "-  DUT: Should not crash and should not send a response\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	ubyte4 i;
	LCPAndExpectState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();

	/* Send 3 unsolicited Configure-Naks in initial state */
	for (i=0; i<3; i++) {
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureNak);
	  state->ncpType = 0;
	  state->ignoreType = lcpTypeConfigureRequest;
	  state->expectedPkts = 0;
	  Log(LOGMEDIUM, "Sending LCP Configure-Nak\n");
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  
	  if (pkt) {
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
	Free(lcp);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "5.10",
     TEST_DESCRIPTION
     "Configure-Nak with bad length is discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.3 p30 Configure-Nak\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Nak with incorrect length\n"
     "-  DUT: Send unmodified Configure-Request\n"
     "- CASE: length = 0-3\n"
     "- CASE: stated length > actual\n"
     "- CASE: Options field length = MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPConfigDataForm_t *opts, *saveOpts;
	ubyte2 bogusLen[] = { 0, 1, 2, 3, 45, 1500 };
	ubyte4 i, cfgLen;
	ubyte cfgBuf[MAX_PACKET_LEN];


	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	saveOpts = LCPConfigDataFormCreate();

	for (i=0; i<NUM_ELEMENTS(bogusLen); i++) {
	DUTstart(10);/*see config IDs spec on epoc client side*/
	  CLEAR_DATA(opts);
	  CLEAR_DATA(saveOpts);

	  /* Separate each case */
	  Log(LOGMEDIUM, "\n");
	  DUTLCPOpen(config, pppConn);
	  
	  state->ncpType = lcpTypeConfigureRequest;
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							saveOpts);

		/* +++alan: check magicnum in saveOpts */

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureNak);
		FORM_SET_FIELD(lcp, identifier, state->ncpForm.identifier);
		FORM_SET_FIELD(lcp, length, bogusLen[i]);
		FORM_SET_FIELD(opts, magicNum, saveOpts->magicNum+1);
		cfgLen = NCPConfigDataBuild(pppConn, LCP,
									(NCPConfigDataForm_t *)opts, cfgBuf);
		FORM_SET_DATA(lcp, cfgBuf, cfgLen);
		
		Log(LOGMEDIUM, "Sending Configure-Nak with incorrect length %u\n",
			bogusLen[i]);
		pkt = LCPAndExpect(pppConn, lcp, state);
		if (!pkt) {
		  status = FALSE;
		}
		else {
		  LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							  opts);
		  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B, saveOpts, opts,
										 CHECK_OPTION_VALUES)) {
			Log(LOGMEDIUM,
				"! Configure-Nak with incorrect length was accepted\n");
			status = FALSE;
		  }
		}
	  }
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}

	Free(saveOpts);
	Free(opts);
	Free(state);
	Free(lcp);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "5.11",
     TEST_DESCRIPTION
     "Configure-Nak with no Options should be ignored\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.3 p30 Configure-Nak\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Nak with no Options specified\n"
     "-  DUT: Send Configure-Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPConfigDataForm_t *opts, *saveOpts;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	saveOpts = LCPConfigDataFormCreate();
	
	DUTLCPOpen(config, pppConn);
	
	state->ncpType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, 0, state);
	if (!pkt) {
	  status = FALSE;
	}
	else {
	  LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
						  saveOpts);
	  
	  Log(LOGMEDIUM, "Sending Configure-Nak with no options\n");
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureNak);
	  LCPSend(pppConn, lcp, 0, 0);
	  
	  state->timeOut.sec=10;/*For debugging only! Remove!!*/
	  
	  LogWaiting("for next Configure-Request", &state->timeOut);
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							opts);
		if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B, saveOpts, opts,
									   NOCHECK_OPTION_VALUES)) {
		  Log(LOGMEDIUM, "! Bogus Configure-Nak was accepted\n");
		  status = FALSE;
		}
	  }
	}
	DUTPPPResetConn(config, pppConn);
	
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(saveOpts);
	Free(opts);
	Free(state);
	Free(lcp);
  }
  END_TEST;

  /* SECTION 5.4 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "6.1",
     TEST_DESCRIPTION
     "If Options received in Configure-Request are not acceptable for\n"
     "negotiation then Configure-Reject MUST be sent\n",
     TEST_REFERENCE
     "RFC 1661 s5.4 p31 Configure-Reject\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request with unknown Option\n"
     "-  DUT: Send Configure-Reject\n"
     "- ANVL: Validate fields\n"
     "- ANVL: Validate fields\n"
     "        Code                       4\n"
     "        Identifier                 same as in Request\n"
     "        Length                     packet length (<= 1500)\n"
     "        Options                    bad Options, in order\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t lcpForm;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	NCPGrabOptionState_t *state;
	LCPConfigDataForm_t rejOpts;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), reserved, TRUE);
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureReject;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! Did not receive any Configure-Reject packets\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Configure-Reject packet\n");
	  LCPPacketToForm(state->cfgPkt, &lcpForm);
	  CLEAR_DATA(&rejOpts);
	  LCPConfigDataToForm(lcpForm.data, lcpForm.dataLen, &rejOpts);

	  /* identifier is already checked by LCPEstConn() */

	  /* check length */
	  if (lcpForm.length != (LCP_HDR_LEN + lcpForm.dataLen)) {
		Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
			"length (%lu)\n",
			lcpForm.length, (LCP_HDR_LEN + lcpForm.dataLen));
		status = FALSE;
	  }

	  /* check options */
	  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B,
									 LCP_CFG(lcpState->localOpts),
									 &rejOpts,
									 CHECK_OPTION_VALUES)) {
		Log(LOGMEDIUM,
			"! Configure-Reject Options do not match Configure-Request\n");
		status = FALSE;
	  }

	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	NCPEstConnStateDestroy(lcpState);
	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "6.2",
     TEST_DESCRIPTION
     "Invalid Configure-Rejects are discarded\n",
     TEST_REFERENCE
     "RFC 1661 s5.4 p31 Configure-Reject\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Options (>1)\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send DUT Configure-Reject with different Identifier\n"
     "-  DUT: Send unmodified Configure-Request\n"
     "- ANVL: Send Configure-Reject with Options in wrong order\n"
     "-  DUT: Send unmodified Configure-Request\n"
     "- ANVL: Send Configure-Reject with different values in Option fields\n"
     "-  DUT: Send unmodified Configure-Request\n"
     "- ANVL: Send Configure-Reject with Options not in Configure-Request\n"
     "-  DUT: Send unmodified Configure-Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	struct LCPBogusConfigureState_s rejState;
	LCPConfigDataForm_t *opts, *saveOpts;
	enum LCPBogusConfigureOptions_e bogusType;

	/* +++alan: how to configure DUT for >1 options, options w/ values */


	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	saveOpts = LCPConfigDataFormCreate();

	for (bogusType=NONE; bogusType<LCP_BOGUS_END; bogusType++) {
	  CLEAR_DATA(opts);
	  CLEAR_DATA(saveOpts);
	  CLEAR_DATA(&rejState);
	  rejState.responseCode = lcpTypeConfigureReject;
	  switch (bogusType) {
	  case NONE:
		rejState.bogusIdent = TRUE;
		break;
	  case OPTION_MISSING: /* not a useful test for Configure-Reject */
		continue;
	  default:
		rejState.bogusData = bogusType;
	  }
	  
	  /* Separate each case */
	  Log(LOGMEDIUM, "\n");
	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  DUTLCPOpen(config, pppConn);
	  state->ncpType = lcpTypeConfigureRequest;
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							saveOpts);

		LCPSendBogusResponse(pppConn, &state->ncpForm, &rejState);
		
		LogWaiting("for next Configure-Request", &state->timeOut);
		pkt = LCPAndExpect(pppConn, 0, state);
		if (!pkt) {
		  status = FALSE;
		}
		else {
		  LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							  opts);
		  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B, saveOpts, opts,
										NOCHECK_OPTION_VALUES)) {
			Log(LOGMEDIUM, "! Bogus Configure-Reject accepted\n");
			status = FALSE;
		  }
		}
	  }
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}

	Free(saveOpts);
	Free(opts);
	Free(state);
	Free(lcp);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "6.3",
     TEST_DESCRIPTION
     "A Configure-Request with none of the rejected Options should be sent\n"
     "when valid Configure-Reject is received\n",
     TEST_REFERENCE
     "RFC 1661 s5.4 p31 Configure-Reject\n",
     TEST_METHOD
	 "- ANVL: Cause DUT to open connection with non-necessary Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Reject with Option\n"
     "-  DUT: Send Configure-Request without Option\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPConfigDataForm_t *opts, *saveOpts;
	ubyte cfgBuf[MAX_PACKET_LEN];
	ubyte4 cfgLen;

	DUTSetPPPACCM(config, LCP_DIFFERENT_ACCM);
	DUTstart(5);/*see config spec  IDs on epoc side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	saveOpts = LCPConfigDataFormCreate();
	
	DUTLCPOpen(config, pppConn);
	
	state->ncpType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, 0, state);
	if (!pkt) {
	  status = FALSE;
	}
	else {
	  LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
						  saveOpts);

	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureReject);
	  FORM_SET_FIELD(lcp, identifier, state->ncpForm.identifier);
	  FORM_SET_FIELD(opts, accm, LCP_DIFFERENT_ACCM);
	  cfgLen = NCPConfigDataBuild(pppConn, LCP,
								  (NCPConfigDataForm_t *)opts, cfgBuf);
	  FORM_SET_DATA(lcp, cfgBuf, cfgLen);
	  
	  Log(LOGMEDIUM, "Sending Configure-Reject for ACCM\n");
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		CLEAR_DATA(opts);
		LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							opts);
		if (opts->accmOK){
		  Log(LOGMEDIUM, "! Configure-Request still contains ACCM option\n");
		  status = FALSE;
		}
	  }
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	
	Free(saveOpts);
	Free(opts);
	Free(state);
	Free(lcp);
	DUTSetPPPACCM(config, LCP_DEFAULT_ACCM);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "6.4",
     TEST_DESCRIPTION
     "Configure-Rejects with bad lengths should be discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.4 p31-32 Configure-Reject\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Reject with incorrect length\n"
     "-  DUT: Send unmodified Configure-Request\n"
     "- CASE: length = 0-3\n"
     "- CASE: stated length > actual\n"
     "- CASE: Options field length = MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPConfigDataForm_t *opts, *saveOpts;
	ubyte2 bogusLen[] = { 0, 1, 2, 3, 45, 1500 };
	ubyte4 i;


	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	saveOpts = LCPConfigDataFormCreate();

	for (i=0; i<NUM_ELEMENTS(bogusLen); i++) {
	  CLEAR_DATA(opts);
	  CLEAR_DATA(saveOpts);
	  
	  /* Separate each case */
	  Log(LOGMEDIUM, "\n");
	  DUTstart(10);/*see config IDs spec on epoc client side*/
	  DUTLCPOpen(config, pppConn);
	  
	  state->ncpType = lcpTypeConfigureRequest;
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							saveOpts);

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureReject);
		FORM_SET_FIELD(lcp, identifier, state->ncpForm.identifier);
		FORM_SET_FIELD(lcp, length, bogusLen[i]);
		FORM_SET_DATA(lcp, state->ncpForm.data, state->ncpForm.dataLen);
		
		Log(LOGMEDIUM, "Sending Configure-Reject with incorrect length %u\n",
			bogusLen[i]);
		pkt = LCPAndExpect(pppConn, lcp, state);
		if (!pkt) {
		  status = FALSE;
		}
		else {
		  LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							  opts);
		  if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B, saveOpts, opts,
										NOCHECK_OPTION_VALUES)) {
			Log(LOGMEDIUM,
				"! Configure-Reject with incorrect length was accepted\n");
			status = FALSE;
		  }
		}
	  }
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}

	Free(saveOpts);
	Free(opts);
	Free(state);
	Free(lcp);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "6.5",
     TEST_DESCRIPTION
     "Configure-Reject with no Options should be ignored\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.4 p31-32 Configure-Reject\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Reject with no Options specified\n"
     "-  DUT: Send Configure-Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPConfigDataForm_t *opts, *saveOpts;

	/* +++alan: how to configure DUT with options */

	DUTstart(10);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	opts = LCPConfigDataFormCreate();
	saveOpts = LCPConfigDataFormCreate();
	
	DUTLCPOpen(config, pppConn);
	
	state->ncpType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, 0, state);
	if (!pkt) {
	  status = FALSE;
	}
	else {
	  LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
						  saveOpts);
	  
	  Log(LOGMEDIUM, "Sending Configure-Reject with no options\n");
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureReject);
	  LCPSend(pppConn, lcp, 0, 0);
	  
	  LogWaiting("for next Configure-Request", &state->timeOut);
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		LCPConfigDataToForm(state->ncpForm.data, state->ncpForm.dataLen,
							opts);
		if (!LCPCompareConfigDataForms(NCPCMP_A_EQUALS_B, saveOpts, opts,
									   NOCHECK_OPTION_VALUES)) {
		  Log(LOGMEDIUM, "! Bogus Configure-Reject was accepted\n");
		  status = FALSE;
		}
	  }
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	
	Free(saveOpts);
	Free(opts);
	Free(state);
	Free(lcp);

  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "6.6",
     TEST_DESCRIPTION
     "Unsolicited Configure-Rejects are discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.4 p31-31 Configure-Reject\n",
     TEST_METHOD
	 "- ANVL: Close connection\n"
     "- ANVL: Send Configure-Reject\n"
     "-  DUT: Should not crash\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	ubyte4 i;
	LCPAndExpectState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();

	/* Send 3 unsolicited Configure-Rejects in initial state */
	for (i=0; i<3; i++) {
	  Log(LOGMEDIUM, "Sending LCP Configure-Reject\n");
	  
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureReject);
	  
	  state->ncpType = 0;
	  state->ignoreType = lcpTypeConfigureRequest;
	  state->expectedPkts = 0;
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  
	  if (pkt) {
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
	Free(lcp);
  }
  END_TEST;

  /* SECTION 5.5 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "7.1",
     TEST_DESCRIPTION
     "A LCP implementation wishing to close connection SHOULD transmit\n"
     "a LCP packet with the Code field set to 5 (Terminate-Request)\n",
     TEST_REFERENCE
     "RFC 1661 s5.5 p33 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
	 "- ANVL: Cause DUT to close connection\n"
     "-  DUT: Send Terminate-Request\n"
     "- ANVL: Validate fields\n"
     "        Code                       5\n"
     "        Identifier                 any\n"
     "        Length                     packet length (<= MRU)\n"
     "        Data                       len - 4 bytes of data\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcpIn;
	LCPAndExpectState_t *state;
	Packet_t *pkt;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  state = LCPAndExpectStateCreate();
	  lcpIn = &state->ncpForm;

	  DUTLCPClose(config);

	  state->ncpType = lcpTypeTerminateRequest;
	  /* wait long enough for DUT to transition */
	  state->timeOut.sec = 15;
	  LogWaiting("for Terminate-Request", &state->timeOut);
 	  DUTSendConfigCommand((ubyte *) "terminate$");
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if(pkt == 0){
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM, "Validating Terminate-Request Fields\n");
		/* Type is how we get pkt, identifier can be anything */

		/* Data + header sent + header received */
		if(lcpIn->length != (lcpIn->dataLen + LCP_HDR_LEN)){
		  Log(LOGMEDIUM, "! Terminate-Request length (%u) should be %lu\n",
			  lcpIn->length, lcpIn->dataLen + LCP_HDR_LEN);
		  status = FALSE;
		}
	  }
	  Free(state);
	}
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "7.2",
     TEST_DESCRIPTION
     "Terminate-Request packets SHOULD continue to be sent until\n"
     "Terminate-Ack is received.\n",
     TEST_REFERENCE
     "RFC 1661 s5.5 p33 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
	 "- ANVL: Cause DUT to close connection\n"
     "-  DUT: Send Terminate-Request packets\n"
     "- ANVL: Ignore 2 Terminate-Requests\n"
     "- ANVL: Send Terminate-Ack packet\n"
     "-  DUT: Should stop sending Terminate-Request packets\n"
     "- ANVL: Check connection is closed\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcpIn;
	LCPAndExpectState_t *state;
	Packet_t *pkt;
	ubyte4 i;

	/** Max-Terminate must be greater than the number of 
	Terminate Request packets we ingore before sending Terminate Ack */ 
	DUTstart(11);/*see config IDs spec on epoc client side*/

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  state = LCPAndExpectStateCreate();
	  lcpIn = &state->ncpForm;

	  DUTLCPClose(config);

	  for(i=0; i<2; i++) {
		state->ncpType = lcpTypeTerminateRequest;
		state->timeOut.sec = LCP_CONFIG_REQ_TIME;
		LogWaiting("for Terminate-Request", &state->timeOut);
 	        DUTSendConfigCommand((ubyte *) "terminate$");
		pkt = LCPAndExpect(pppConn, 0, state);
		if(pkt == 0){
		  status = FALSE;
		}
	  }

	  if (status) {
		Log(LOGMEDIUM, "Sending Terminate-Ack\n");

		FORM_SET_FIELD(lcpIn, code, lcpTypeTerminateAck);
		LCPSend(pppConn, lcpIn, 0, 0);

		state->expectedPkts = 0;
		LogWaiting("for Terminate-Request", &state->timeOut);
 	        DUTSendConfigCommand((ubyte *) "terminate$");
		pkt = LCPAndExpect(pppConn, 0, state);
		if(pkt){
		  status = FALSE;
		}
	  }

	  Free(state);
	}
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "7.3",
     TEST_DESCRIPTION
     "Terminate-Request packets SHOULD continue to be sent until\n"
     "the lower layer indicates that it has gone down.\n",
     TEST_REFERENCE
     "RFC 1661 s5.5 p33 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
	 "- ANVL: Cause DUT to close connection\n"
     "-  DUT: Send Terminate-Request packets\n"
     "- ANVL: Ignore 2 Terminate-Request packets\n"
	 "- ANVL: Drop DTR\n"
     "-  DUT: Should stop sending Terminate-Request packets\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcpIn;
	LCPAndExpectState_t *state;
	Packet_t *pkt;
	ubyte4 i;

	/** Max-Terminate must be greater than the number of 
	Terminate Request packets we ingore before dropping DTR */ 
	DUTstart(11);/*see config IDs spec on epoc client side*/

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  state = LCPAndExpectStateCreate();
	  lcpIn = &state->ncpForm;

	  DUTLCPClose(config);

	  for(i=0; i<2; i++) {
		state->ncpType = lcpTypeTerminateRequest;
		state->timeOut.sec = LCP_CONFIG_REQ_TIME;
		LogWaiting("for Terminate-Request", &state->timeOut);
  	        DUTSendConfigCommand((ubyte *) "terminate$");
		pkt = LCPAndExpect(pppConn, 0, state);
		if(pkt == 0){
		  status = FALSE;
		}
	  }

	  if (status) {
		Log(LOGMEDIUM, "Dropping DTR\n");
		PktSrcSetState(pppConn->pktSrc, PKTSRC_DOWN);

		state->expectedPkts = 0;
		LogWaiting("for Terminate-Request", &state->timeOut);
 	        DUTSendConfigCommand((ubyte *) "terminate$");
		pkt = LCPAndExpect(pppConn, 0, state);
		if(pkt){
		  status = FALSE;
		}
	  }

	  Free(state);
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "7.4",
     TEST_DESCRIPTION
     "Terminate-Request packets SHOULD continue to be sent until\n"
     "a sufficiently large number have been transmitted\n",
     TEST_REFERENCE
     "RFC 1661 s5.5 p33 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
     "-  DUT: Send Max Terminate Terminate-Request packets\n"
	 "-  DUT: Close connection\n"
     "- ANVL: Check connection is closed\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPAndExpectState_t *state;
	Packet_t *pkt;
	
	DUTstart(0);/*see config IDs spec on epoc client side*/

	state = LCPAndExpectStateCreate();

	if (!ANVLPPPEstConn(config, pppConn, 0)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  DUTLCPClose(config);

	  state->ncpType = lcpTypeTerminateRequest;
	  state->expectedPkts = LCP_DEFAULT_MAX_TERMINATE;
	  state->timeOut.sec = state->expectedPkts * LCP_CONFIG_REQ_TIME;
	  Log(LOGMEDIUM, "Waiting for %lu Terminate-Requests",state->expectedPkts);
 	  DUTSendConfigCommand((ubyte *) "terminate$");
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (state->receivedPkts != state->expectedPkts) {
		status = FALSE;
	  }
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(state);
  }
  END_TEST;
    
  BEGIN_TEST(
     TEST_NUM
     "7.5",
     TEST_DESCRIPTION
     "Upon reception of Terminate-Request, a Terminate-Ack MUST be\n"
     "transmitted\n",
     TEST_REFERENCE
     "RFC 1661 s5.5 p33 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
     "- ANVL: Send DUT Terminate-Request\n"
     "-  DUT: Send Terminate-Ack\n"
     "- ANVL: Validate fields\n"
     "        Code                       6\n"
     "        Identifier                 same as in Terminate-Request\n"
     "        Length                     packet length (<= MRU)\n"
     "        Data                       len - 4 bytes of data\n"
     "-  DUT: Close connection\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPAndExpectState_t *state;
	LCPForm_t *lcp, response;
	ubyte identifier = NCPGetNextIdentifier();
	Packet_t *pkt;
	
	DUTstart(0);/*see config IDs spec on epoc client side*/

	state = LCPAndExpectStateCreate();
	lcp = LCPFormCreate();

	FORM_SET_FIELD(lcp, code, lcpTypeTerminateRequest);
	FORM_SET_FIELD(lcp, identifier, identifier);

	if (!ANVLPPPEstConn(config, pppConn, 0)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  Log(LOGMEDIUM, "Sending LCP Terminate-Request\n");
 	 /* DUTSendConfigCommand((ubyte *) "terminate$");*/

	  state->ncpType = lcpTypeTerminateAck;
	  pkt = LCPAndExpect(pppConn, lcp, state);

	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		response = state->ncpForm;
		Log(LOGMEDIUM, "Validating Terminate-Ack packet\n");

		 /* Validate: Code       : 6 */
		if (response.code != lcpTypeTerminateAck) {
		  Log(LOGMEDIUM, "! LCP Terminate-Ack Code should be 6 (%u)\n",
			  response.code);
		  status = FALSE;
		}

		 /* Validate: Identifier : same as in Terminate-Request */
		if (response.identifier != identifier) {
		  Log(LOGMEDIUM, "! LCP Terminate-Ack identifier (0x%02X)\n"
			  "  does not match Terminate-Request identifier (0x%02X)\n",
			  response.identifier, identifier);
		  status = FALSE;
		}
		
		/* Validate: Length     : packet length (<= MRU) */
		if (response.length >= 300) { /*+++dvb: MRU?? */
		  Log(LOGMEDIUM, 
			  "! LCP Terminate-Ack length (%u) is larger then the MRU (%u)\n",
			  response.length, 300);
		  status = FALSE;
		}

		/* Validate: Data       : len - 4 bytes of data */
		if (response.dataLenOK) {
		  if (response.dataLen != (ubyte4)(response.length - 4)) {
			Log(LOGMEDIUM, "! LCP Terminate-Ack data length (%lu) "
				"is mismatched with packet length (%u)\n", 
				response.dataLen, response.length);
			status = FALSE;
		  }
		}
	  }
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(lcp);
	Free(state);
  }
  END_TEST;
    
  
  BEGIN_TEST(
     TEST_NUM
     "7.7",
     TEST_DESCRIPTION
     "An unelicited Terminate-Ack means peer is in need of renegotiation\n",
     TEST_REFERENCE
     "RFC 1661 s5.5 p33 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
     "- ANVL: Send Terminate-Ack\n"
     "-  DUT: Send Configure-Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPAndExpectState_t *state;
	LCPForm_t *lcp;
	Packet_t *pkt;
	
	DUTstart(0);/*see config IDs spec on epoc client side*/

	state = LCPAndExpectStateCreate();
	lcp = LCPFormCreate();

	FORM_SET_FIELD(lcp, code, lcpTypeTerminateAck);

	if (!ANVLPPPEstConn(config, pppConn, 0)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  Log(LOGMEDIUM, "Sending unsolicited LCP Terminate-Ack\n");

	  state->ncpType = lcpTypeConfigureRequest;
	  pkt = LCPAndExpect(pppConn, lcp, state);

	  if (!pkt) {
		status = FALSE;
	  }
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(lcp);
	Free(state);
  }
  END_TEST;


  BEGIN_TEST(
     TEST_NUM
     "7.9",
     TEST_DESCRIPTION
     "Bad length Terminate-Request packets should be discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.5 p33-34 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Send Terminate-Request with incorrect lengths\n"
     "-  DUT: Should discard\n"
	 "- ANVL: Check connection still up\n"
     "- CASE: length = 0-3\n"
     "- CASE: stated length > actual\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPAndExpectState_t *state;
	LCPForm_t *lcp;
	Packet_t *pkt;
	ubyte2 bogusLen[] = { 0, 1, 2, 3, HUGE_LENGTH };
	ubyte i = 0;
	

	for (i=0; i<NUM_ELEMENTS(bogusLen); i++) {
	  state = LCPAndExpectStateCreate();
	  lcp = LCPFormCreate();
	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  
	  if (!ANVLPPPEstConn(config, pppConn, 0)) {
		Log(LOGMEDIUM, "! Unable to open PPP connection\n");
		status = FALSE;
	  }
	  else {
		Log(LOGMEDIUM,
			"\nSending LCP Terminate-Request with incorrect length %u\n",
			bogusLen[i]);

		FORM_SET_FIELD(lcp, code, lcpTypeTerminateRequest);
		state->ncpType = lcpTypeTerminateAck;
		
		FORM_SET_FIELD(lcp, length, bogusLen[i]);
		pkt = LCPAndExpect(pppConn, lcp, state);
		if (pkt) {
		  Log(LOGMEDIUM,
			  "! LCP Terminate-Request packet with length %u was accepted\n",
			  bogusLen[i]);
		  status = FALSE;
		}
	  }
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	  Free(lcp);
	  Free(state);
	}
  }
  END_TEST;


  BEGIN_TEST(
     TEST_NUM
     "7.10",
     TEST_DESCRIPTION
     "Bad length Terminate-Ack packets should be discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.5 p33-34 Terminate-Request and Terminate-Ack\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
	 "- ANVL: Cause DUT to close connection\n"
     "-  DUT: Send Terminate-Request\n"
     "- ANVL: Send Terminate-Ack with incorrect length\n"
     "-  DUT: Resend Terminate-Request with original id\n"
     "- CASE: length = 0-3\n"
     "- CASE: stated length > actual\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	ubyte2 bogusLength[] = { 0, 1, 2, 3, 45, 1500 };
	ubyte4 i;


	for (i=0; i<NUM_ELEMENTS(bogusLength); i++) {
	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  if (!ANVLPPPEstConn(config, pppConn, 0)) {
		Log(LOGMEDIUM, "! Unable to open PPP connection\n");
		status = FALSE;
	  }
	  else {
		state = LCPAndExpectStateCreate();
		lcp = LCPFormCreate();
		
		FORM_SET_FIELD(lcp, length, bogusLength[i]);
		state->ncpType = lcpTypeTerminateRequest;
		state->expectedPkts = 1;
		state->timeOut.sec = LCP_CONFIG_REQ_TIME * state->expectedPkts;

		DUTLCPClose(config);
		Log(LOGMEDIUM, "\nWaiting for LCP Terminate-Request "
			"(will Ack with incorrect length %u)\n",
			lcp->length);
         	DUTSendConfigCommand((ubyte *) "terminate$");
		LCPAndExpect(pppConn, 0, state);
		
		if (state->receivedPkts == 0) {
		  /* DUT did not send Terminate-Request--common function has log msg */
		  status = FALSE;
		}
		else {
		  /* DUT sent Terminate-Request */ 
		  /* send out LCP Terminate-Ack with bad length */
		  Log(LOGMEDIUM, "Sending LCP Terminate-Ack with length %u\n",
			  lcp->length);
		  LCPTermAckXmit(pppConn, &(state->ncpForm), lcp);

		  /* reset state */
		  Free(state);
		  state = LCPAndExpectStateCreate();
		  state->ncpType = lcpTypeTerminateRequest;
		  state->expectedPkts = 1;
		  state->timeOut.sec = LCP_CONFIG_REQ_TIME * state->expectedPkts;
		  
		  /* check that DUT resent Terminate-Request */
		  Log(LOGMEDIUM, "Waiting for LCP Terminate-Request\n");
		  LCPAndExpect(pppConn, 0, state);
		  
		  if (state->receivedPkts == 0) {
			/* DUT did not re-send Term-Req--common function has log msg */
			status = FALSE;
		  }
		}

		/* clean up */
		Free(state);
		Free(lcp);
	  }
	  
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;


  
  BEGIN_TEST(
     TEST_NUM
     "7.25",
     TEST_DESCRIPTION
     "Default Terminate Ack timeout is 3 (+/- 0.5) seconds \n",
     TEST_REFERENCE
     "RFC1661 s3.7 Link Termination Phase\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
     "- ANVL: Send DUT Terminate-Request\n"
     "-  DUT: Send Terminate-Ack\n"
     "- ANVL: Wait for at most 1 2/3 of timeout period\n"
     "- ANVL: Send Terminate-Request\n"
     "-  DUT: Do not respond.\n",	
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
      int defaultTermAckPeriod = 3;
      int epocTestConfigId = 0;
      int terminateReqSent      = 2;
      int lastTermAckRecvdStep  = 0; /* (steps: 0 to 1) */
      int termReqSendPeriodNormal     = defaultTermAckPeriod;
      int periodBeforeLastTermReqSend = defaultTermAckPeriod + 2;
      
      status = PPPTestTerminateAck(
                 config,
                 pppConn,
                 epocTestConfigId,
                 terminateReqSent, 
                 lastTermAckRecvdStep,
                 termReqSendPeriodNormal,
                 periodBeforeLastTermReqSend); 
  }
  END_TEST

  BEGIN_TEST(
     TEST_NUM
     "7.26",
     TEST_DESCRIPTION
     "If Terminate Request is received in Stopping state,\n"
     "Terminate Ack is retransmitted but the timeout is not restarted.\n", 
     TEST_REFERENCE
     "RFC1661 s3.7 Link Termination Phase, s4.1 State Transition Table\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
     "- ANVL: Send Terminate-Request\n"
     "-  DUT: Send Terminate-Ack\n"
     "- ANVL: Wait for 2 seconds.\n"
     "- ANVL: Send Terminate-Request\n"
     "-  DUT: Send Terminate-Ack\n"
     "- ANVL: Wait for 2 seconds \n"
     "- ANVL: Send Terminate Request\n"
     "-  DUT: Do not respond.\n",	
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
      int defaultTermAckPeriod = 3;
      int epocTestConfigId = 0;
      int terminateReqSent      = 3;
      int lastTermAckRecvdStep  = 1; /* (steps: 0 to 2) */
      int termReqSendPeriodNormal     = defaultTermAckPeriod - 1;
      int periodBeforeLastTermReqSend = defaultTermAckPeriod - 1;
      
      status = PPPTestTerminateAck(
                 config,
                 pppConn,
                 epocTestConfigId,
                 terminateReqSent, 
                 lastTermAckRecvdStep,
                 termReqSendPeriodNormal,
                 periodBeforeLastTermReqSend); 
  }
 END_TEST

 BEGIN_TEST(
     TEST_NUM
     "7.27",
     TEST_DESCRIPTION
     "Symbian Specific: Terminate Ack Timeout should be configurable.\n",
     TEST_REFERENCE
     "RFC1661 s3.7 Link Termination Phase\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with DUT\n"
     "- ANVL: Configure DUT with Terminate Ack timeout (9 sec) \n"
     "        different from default (3 +/- 0.5 sec). \n"
     "- ANVL: Send DUT Terminate-Request\n"
     "-  DUT: Send Terminate-Ack\n"
     "- ANVL: Wait for 2/3 of timeout to expire\n"
     "- ANVL: Send Terminate-Request\n"
     "-  DUT: Send Terminate-Ack\n"
     "- ANVL: Wait for 1 timeout to expire\n"
     "- ANVL: Send Terminate-Request\n"
     "-  DUT: Do not respond.\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
      int defaultTermAckPeriod = 3;
      int epocTestConfigId = 12;
      int terminateReqSent      = 3;
      int lastTermAckRecvdStep  = 1; /* (steps: 0 to 2) */
      int termReqSendPeriodNormal     = defaultTermAckPeriod * 2;
      int periodBeforeLastTermReqSend = defaultTermAckPeriod * 3;
      
      status = PPPTestTerminateAck(
                 config,
                 pppConn,
                 epocTestConfigId,
                 terminateReqSent, 
                 lastTermAckRecvdStep,
                 termReqSendPeriodNormal,
                 periodBeforeLastTermReqSend); 
  }
  END_TEST

  /* SECTION 5.6 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "8.1",
     TEST_DESCRIPTION
     "Reception of a LCP packet with an invalid Code MUST be reported\n"
     "by transmitting a Code-Reject\n",
     TEST_REFERENCE
     "RFC 1661 s5.6 p34 Code-Reject\n",
     TEST_METHOD
     "- ANVL: Send LCP packet with invalid code to DUT\n"
     "-  DUT: Send Code-Reject packet\n"
     "- ANVL: Validate fields\n"
     "        Code                       7\n"
     "        Identifier                 any\n"
     "        Length                     packet length (<= MRU)\n"
     "        Rejected-Packet            info field of rejected packet\n"
	 "                                   truncated to MRU\n"
     "- CASE: 0, 25, 128, 200\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp, *lcpIn;
	LCPAndExpectState_t *state;
	Packet_t *pkt;
	byte testStr[MAX_STRING];
	ubyte4 i, len, buffLen;
	ubyte buff[MAX_PACKET_LEN];
	ubyte bogusCode[] = { 0, 25, 128, 200 };

	DUTstart(0);/*see config IDs spec on epoc client side*/

	/* This test used to test for a bogus code of 12, too.
	   However, a code of 12 is a valid LCP Identification packet
	   so the test of 12 was invalid and therefore removed */

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  state = LCPAndExpectStateCreate();
	  lcpIn = &state->ncpForm;

	  /* Want to grab the reject */
	  state->ncpType = lcpTypeCodeReject;

	  for (i=0; i<NUM_ELEMENTS(bogusCode); i++) {
		lcp = LCPFormCreate();

		SPrintf(testStr, "ANVL-PPP-%s %lu", testNum, i);
		len = StrLen(testStr);
		FORM_SET_DATA(lcp, (ubyte *)testStr, len);	  
		FORM_SET_FIELD(lcp, code, bogusCode[i]);
		/* need to set identifier so it doesn't get incremented in LCPBuild()
		   below */
		FORM_SET_FIELD(lcp, identifier, 0);
		
		Log(LOGMEDIUM, "Sending LCP Packet with bad code (%u)\n", lcp->code);
		pkt = LCPAndExpect(pppConn, lcp, state);
		if(pkt == 0){
		  status = FALSE;
		}
		else{
		  Log(LOGMEDIUM, "Validating Code-Reject Fields\n");
		  /* Type is how we get pkt, identifier can be anything */
		  
		  /* Data + header sent + header received */
		  if(lcpIn->length != (len + LCP_HDR_LEN + LCP_HDR_LEN)){
			Log(LOGMEDIUM, "! Code-Reject length (%u) should be %lu\n",
				lcpIn->length, len + LCP_HDR_LEN * 2);
			status = FALSE;
		  }
		  else{
			buffLen = LCPBuild(pppConn, lcp, buff);
			if(!DATA_EQUAL(buff, buffLen, lcpIn->data, lcpIn->dataLen)){
			  Log(LOGMEDIUM,
				  "! Code-Reject data does not equal sent packet\n");
			  status = FALSE;
			}
		  }
		}
		Free(lcp);
	  } /* for */
	  Free(state);
	  DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;
    
  /*+++art: 8.2-8.6 may need different p/f criteria as shutting down is ok*/
  BEGIN_TEST(
     TEST_NUM
     "8.2",
     TEST_DESCRIPTION
     "The Identifier field MUST be changed for each Code-Reject sent.\n",
     TEST_REFERENCE
     "RFC 1661 s5.6 p34 Code-Reject\n",
     TEST_METHOD
     "- ANVL: Send LCP packet with unknown code to DUT\n"
     "-  DUT: Send Code-Reject packet\n"
     "- ANVL: Send LCP packet with unknown code to DUT\n"
     "-  DUT: Send Code-Reject packet with different Identifier\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp, *lcpIn;
	LCPAndExpectState_t *state;
	Packet_t *pkt;
	byte testStr[MAX_STRING];
	ubyte4 len, i;
	ubyte idLast = 0;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  lcp = LCPFormCreate();
	  /* This should cause a Code-Reject */
	  FORM_SET_FIELD(lcp, code, LCP_BOGUS_CODE);

	  for(i = 0; i < 5; i++){
		state = LCPAndExpectStateCreate();
		lcpIn = &state->ncpForm;

		/* Want to grab the reject */
		state->ncpType = lcpTypeCodeReject;
		SPrintf(testStr, "ANVL-PPP-%s #%ld", testNum, i);
		len = StrLen(testStr);
		FORM_SET_DATA(lcp, (ubyte *)testStr, len);	  

		Log(LOGMEDIUM, "Sending LCP Packet with bad code (%u)\n", lcp->code);
		pkt = LCPAndExpect(pppConn, lcp, state);
		if(pkt == 0){
		  status = FALSE;
		  break;
		}
		Log(LOGMEDIUM, "Code-Reject Identifier = 0x%02X\n", lcpIn->identifier);
		if((i != 0) && (lcpIn->identifier == idLast)){
		  Log(LOGMEDIUM, "! Code-Reject Identifier did not change\n");
		  status = FALSE;
		  break;
		}
		idLast = lcpIn->identifier;
		Free(state);
	  }
	  Free(lcp);
	  DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;
    
  BEGIN_TEST(
     TEST_NUM
     "8.3",
     TEST_DESCRIPTION
     "The Rejected-Information MUST be truncated to comply with the peer's\n"
	 "established MRU.\n",
     TEST_REFERENCE
     "RFC 1661 s5.6 p34 Code-Reject\n",
     TEST_METHOD
     "- ANVL: Send LCP packet with unknown code and len = MRU\n"
     "-  DUT: Send Code-Reject\n"
     "- ANVL: Validate Rejected-Packet field is truncated correctly\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp, *lcpIn;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;
	Packet_t *pkt;
	byte tmpStr[MAX_STRING];
	ubyte4 buffLen;
	ubyte buff[MAX_PACKET_LEN], testStr[MAX_STRING * 2];
	ubyte2 mru = MAX_STRING;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	/* Set our MRU low */
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), mru, mru);

	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else if(!LCP_CFG(lcpState->localOptsAckd)->mruOK ||
			(LCP_CFG(lcpState->localOptsAckd)->mru != mru)){
	  Log(LOGMEDIUM, "! MRU Request of %u was not Acknowleged\n", mru);
	  status = FALSE;
	}
	else{
	  state = LCPAndExpectStateCreate();
	  lcp = LCPFormCreate();
	  lcpIn = &state->ncpForm;

	  /* Want to grab the reject */
	  state->ncpType = lcpTypeCodeReject;

	  /* This should cause one */
	  SPrintf(tmpStr, "ANVL-%s-%s ", protocol, testNum);
	  RepeatStringIntoData(tmpStr, StrLen(tmpStr),
						   testStr, mru);

	  FORM_SET_FIELD(lcp, code, LCP_BOGUS_CODE);
	  /* need to set identifier so it doesn't get incremented in LCPBuild()
		 below */
	  FORM_SET_FIELD(lcp, identifier, 0);
	  FORM_SET_DATA(lcp, testStr, mru);

	  Log(LOGMEDIUM, "Sending LCP Packet with bad code (%u)\n", lcp->code);
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  if(pkt == 0){
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM, "Validating Code-Reject data and length\n");
		if(lcpIn->length != mru){
		  Log(LOGMEDIUM,
			  "! Code-Reject length (%u) not truncated to MRU (%u)\n",
			  lcpIn->length, mru);
		  status = FALSE;
		}
		if(lcpIn->dataLen != (ubyte4)(mru - LCP_HDR_LEN)){
		  Log(LOGMEDIUM,
			  "! Code-Reject data length (%lu) not "
			  "truncated to MRU - %u (%u)\n",
			  lcpIn->dataLen, LCP_HDR_LEN, mru - LCP_HDR_LEN);
		  status = FALSE;
		}
		else{
		  buffLen = LCPBuild(pppConn, lcp, buff);
		  if(!DATA_EQUAL(buff, lcpIn->dataLen, lcpIn->data, lcpIn->dataLen)){
			Log(LOGMEDIUM, "! Code-Reject data does not equal sent packet\n");
			status = FALSE;
		  }
		}
	  }
	  Free(state);
	  Free(lcp);
	  DUTPPPResetConn(config, pppConn);
	}
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "8.4",
     TEST_DESCRIPTION
     "Unsolicited Code-Reject should be discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.6 p34 Code-Reject\n",
     TEST_METHOD
     "- ANVL: Send Code-Reject for unknown code\n"
     "-  DUT: Should discard\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	NCPEstConnState_t *lcpState;
	ubyte4 magic;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);

	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  magic = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
		  LCP_CFG(lcpState->localOptsAckd)->magicNum:0;
	  lcp = LCPFormCreate();
	  FORM_SET_FIELD(lcp, code, lcpTypeCodeReject);
	  Log(LOGMEDIUM, "Sending unsolicited Code-Reject\n");
	  LCPSend(pppConn, lcp, 0, 0);

	  status = LCPEchoAYT(pppConn, magic, testNum);
	  Free(lcp);
	  DUTPPPResetConn(config, pppConn);
	}
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  /* +++dhwong: If a MRU greater than MAX_PACKET_LEN is negotiated,
	 then don't do case 6, which will send a packet of that length */
  BEGIN_TEST(
     TEST_NUM
     "8.5",
     TEST_DESCRIPTION
     "Bad length Code-Reject packets should be discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.6 p34 Code-Reject\n",
     TEST_METHOD
     "- ANVL: Send Code-Reject with incorrect lengths\n"
     "-  DUT: Should remain up\n"
     "- CASE: length = 0-3\n"
     "- CASE: stated length > actual\n"
     "- CASE: Rejected-Packet field length = MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	NCPEstConnState_t *lcpState = 0;
	ubyte4 magic, i, mru;
	ubyte buff[MAX_PACKET_LEN];
	boolean tmpStatus = FALSE, localState = FALSE, skipCase;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	CLEAR_DATA(buff);
	for (i = 1; i <= 6; i++){
	  /* if more cases are added that need to use skipCase, we need to
		 re-initialize it to FALSE */
	  skipCase = FALSE;
	  if (!localState){
		lcpState = NCPEstConnStateCreate(LCP);
		localState = TRUE;
	  }

	  if (!tmpStatus){
		tmpStatus = ANVLPPPEstConn(config, pppConn, lcpState);
		if (!tmpStatus){
		  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
		  status = FALSE;
		  if (localState){
			NCPEstConnStateDestroy(lcpState);
			localState = FALSE;
		  }
		  break;
		}
	  }
	  magic = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
		  LCP_CFG(lcpState->localOptsAckd)->magicNum:0;
	  lcp = LCPFormCreate();
	  FORM_SET_FIELD(lcp, code, lcpTypeCodeReject);

	  switch(i){
	  case 1:
	  case 2:
	  case 3:
	  case 4:
		FORM_SET_FIELD(lcp, length, (ubyte2)(i - 1));
		Log(LOGMEDIUM, "Sending Code-Reject with incorrect length (%u)\n",
			lcp->length);
		break;
	  case 5:
		FORM_SET_FIELD(lcp, length, MAX_PACKET_LEN);
		FORM_SET_DATA(lcp, buff, MAX_PACKET_LEN/2);
		Log(LOGMEDIUM,
			"Sending Code-Reject with stated length %u "
			"and actual length %lu\n",
			lcp->length, lcp->dataLen);
		break;
	  case 6:
		mru = LCP_CFG(lcpState->remoteOptsAckd)->mruOK?
		  LCP_CFG(lcpState->remoteOptsAckd)->mru:LCP_DEFAULT_MRU;

		/* This prevents ANVL from running a test in which the packet
		   sizes greater than MAX_PACKET_LEN, which could cause
		   problems if it's not checked elsewhere */
		if(mru > MAX_PACKET_LEN){
		  skipCase = TRUE;
		  Log(LOGMEDIUM,
			  "NOTE: Skipping this section of the test. DUT negotiated a\n"
			  "      MRU of %ld. Max packet size for ANVL is %d.\n",
			  mru, MAX_PACKET_LEN);
		}
		else{
		  FORM_SET_DATA(lcp, buff, mru);
		  Log(LOGMEDIUM,
			  "Sending Code-Reject with Rejected-Packet size = MRU (%lu)\n",
			  mru);
		}
		break;
	  default:
		Free(lcp);
		if (localState){
		  NCPEstConnStateDestroy(lcpState);
		  localState = FALSE;
		}
		Error(FATAL, "This should not have been reached\n");
	  }
	  if(!skipCase){
		LCPSend(pppConn, lcp, 0, 0);

		tmpStatus = LCPEchoAYT(pppConn, magic, testNum);
		if (!tmpStatus){
		  DUTPPPResetConn(config, pppConn);
		}
	  }
	  Free(lcp);
	  if (localState){
		NCPEstConnStateDestroy(lcpState);
		localState = FALSE;
	  }
	}
	status = (status && tmpStatus);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  /* +++dhwong: Is this behavior correct? On 2/3 implementations I
	 tested on, the DUT dropped the connection without sending
	 terminate-requests or terminate-acks. One DUT still had a
	 connection, even after the code-reject. */

  /* A code-reject of an Echo-Reply should terminate the connection */ 
  BEGIN_TEST(
     TEST_NUM
     "8.6",
     TEST_DESCRIPTION
     "A Code-Reject of a packet such as an Echo-Reply communicates an\n"
	 "unrecoverable error that terminates the connection\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.6 p34 Code-Reject\n"
	 "RFC 1661 s4.3 p20 Events\n",
     TEST_METHOD
     "- ANVL: Send Echo-Request\n"
	 "-  DUT: Send Echo-Reply\n"
     "- ANVL: Send Code-Reject for Echo-Reply\n"
     "- ANVL: Send Echo-Request\n"
	 "-  DUT: Do not respond\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp, *eLCP;
	LCPEchoDiscardForm_t *lcpEcho;
	NCPEstConnState_t *lcpState;
	LCPAndExpectState_t *state;
	ubyte4 magic;
	Packet_t *pkt;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);

	Log(LOGMEDIUM, "Establish PPP connection\n");
	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  state = LCPAndExpectStateCreate();
	  lcpEcho = LCPEchoDiscardFormCreate();
	  eLCP = LCPFormCreate();

	  magic = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
		  LCP_CFG(lcpState->localOptsAckd)->magicNum:0;

	  FORM_SET_FIELD(lcpEcho, magicNumber, magic);
	  Log(LOGMEDIUM, "Sending LCP Echo Request to DUT\n");
	  pkt = LCPEchoAndExpect(pppConn, lcpEcho, eLCP, state, testNum); 

	  if(pkt == 0){
		status = FALSE;
	  }
	  else{
		lcp = LCPFormCreate();
		FORM_SET_FIELD(lcp, code, lcpTypeCodeReject);
		FORM_SET_DATA(lcp, state->pktBuffer, pkt->len + NCP_HDR_LEN);
		Log(LOGMEDIUM, "Sending Code-Reject for Echo-Reply\n");
		LCPSend(pppConn, lcp, 0, 0);

		state->expectedPkts = 0;
		Log(LOGMEDIUM,
			"Sending Echo-Request to see if DUT is still responding\n");
		pkt = LCPEchoAndExpect(pppConn, lcpEcho, eLCP, state, testNum);
		
		status = (pkt == 0);
		Free(lcp);
	  }
	  Free(state);
	  Free(lcpEcho);
	  Free(eLCP);
	  DUTPPPResetConn(config, pppConn);
	}
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  /* SECTION 5.7 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "9.1",
     TEST_DESCRIPTION
     "Reception of a PPP packet with an unknown Protocol field\n"
     "MUST be reported back to the peer by transmitting a Protocol-Reject\n",
     TEST_REFERENCE
     "RFC 1661 s5.7 p35 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Send a PPP packet with unknown Protocol field to DUT\n"
     "-  DUT: Send back Protocol-Reject packet\n"
     "- ANVL: Validate fields:\n"
     "        Code                       8\n"
     "        Identifier                 any\n"
     "        Length                     packet length (<= MRU)\n"
     "        Rejected-Protocol          rejected protocol field\n"
     "        Rejected-Information       length - 6 bytes of Info. field\n"
     "                                   of rejected packet\n"
	 "- CASE: Protocol = 0xC022, 0, 0x8037, 0xFEFF, 0x0022\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	PPPForm_t *ppp;
	LCPProtocolRejectForm_t rForm;
	ubyte4 i, bogusLen;
	byte bogusData[MAX_STRING];
	ubyte2 bogusProto[] = { 0xC022, 0, 0x8037, 0xFEFF, 0x0022 };
	LCPAndExpectState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	ppp = PPPFormCreate();
	state = LCPAndExpectStateCreate();

	if (!ANVLPPPEstConn(config, pppConn, 0)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  for (i=0; i<NUM_ELEMENTS(bogusProto); i++) {
		Log(LOGMEDIUM, "Sending packet with unknown protocol 0x%04X\n",
			bogusProto[i]);
		
		FORM_SET_FIELD(ppp, protocol, bogusProto[i]);
		SPrintf(bogusData, "ANVL-PPP-%s", testNum);
		bogusLen = StrLen(bogusData);
		FORM_SET_DATA(ppp, (ubyte *)bogusData, bogusLen);

		PPPSend(pppConn, ppp, 0);

		state->ncpType = lcpTypeProtocolReject;
		pkt = LCPAndExpect(pppConn, 0, state);

		if (pkt) {
		  LCPProtocolRejectToForm(pkt->data, pkt->len, &rForm);
		  
		  /* check length */
		  if (state->ncpForm.length != (LCP_HDR_LEN + pkt->len)) {
			Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
				"length (%lu)\n",
				state->ncpForm.length, (LCP_HDR_LEN + pkt->len));
			status = FALSE;
		  }
		  
		  /* check rejected protocol */
		  if (rForm.rejectedProtocol != bogusProto[i]) {
			Log(LOGMEDIUM, "! LCP Protocol-Reject Rejected Protocol (0x%04X)"
				" does not match protocol sent (0x%04X)\n",
				rForm.rejectedProtocol, bogusProto[i]);
			status = FALSE;
		  }
		  
		  /* check rejected info */
		  if(!DATA_EQUAL(rForm.rejectedInfo,rForm.rejectedInfoLen,
						 bogusData, bogusLen)){
			Log(LOGMEDIUM,
				"! LCP Protocol-Reject Rejected Info does not match "
				"data sent\n");
			status = FALSE;
		  }
		}
		else {
		  status = FALSE;
		}
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
	Free(ppp);
  }
  END_TEST;


/* +++alan: this should be written so it doesn't require LIB_IP */

  BEGIN_TEST(
     TEST_NUM
     "9.2",
     TEST_DESCRIPTION
     "Upon reception of a Protocol-Reject, the implementation MUST stop\n"
     "sending packets of the indicated protocol\n",
     TEST_REFERENCE
     "RFC 1661 s5.7 p35 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Cause DUT to send non-LCP packets\n"
     "-  DUT: Send non-LCP packets\n"
     "- ANVL: Send Protocol-Reject packet for protocol\n"
     "-  DUT: Should stop sending packets for that protocol\n"
	 "- CASE: protocol = IP, non-LCP packet = ICMP echo\n"
     "- CASE: protocol = IPCP, non-LCP packet = IPCP packets\n"
     "- CASE: protocol = IPCP, non-LCP packet = ICMP echo\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	boolean pppStatus, ncpStatus, echoStatus;
	NCPEstConnState_t *ipcpState;
	LCPForm_t *lcp;
	LCPProtocolRejectForm_t *rej;
	byte msg[MAX_STRING], protStr[MAX_STRING];
	ubyte buff[MAX_STRING];
	ubyte4 buffLen;
	IPIF_t *ipif;
	ubyte i;

	ipif = ANVLConfigInterfaceNext(config, 0, ANVL_INTERFACE_PPP,
								   ANVL_PROTOCOL_IP);
	
	for (i=0; i<3; i++) {
	  lcp = LCPFormCreate();
	  rej = LCPProtocolRejectFormCreate();
	  
	  pppStatus = ANVLPPPEstConn(config, pppConn, 0);
	  if (!pppStatus) {
		status = FALSE;
		Log(LOGMEDIUM, "! Unable to establish connection\n");
	  }
	  else {
		/* Bring up connection */
		ncpStatus = ANVLNCPEstConnAndEcho(config, 0, IPCP, 0, 0);
		if (!ncpStatus) {
		  status = FALSE;
		  Log(LOGMEDIUM, "! Did not receive Echo Reply\n");
		}
		else {
		  Log(LOGMEDIUM, "Received NCP Echo Reply\n");
		  
		  switch (i) {
		  case 0: /* reject IP, no ICMP echo */
			FORM_SET_FIELD(lcp, code, lcpTypeProtocolReject);
			FORM_SET_FIELD(rej, rejectedProtocol, pppTypeIP);
			SPrintf(msg, "ANVL-PPP-%s", testNum);
			FORM_SET_VARFIELD(rej, rejectedInfo, (ubyte *)msg, StrLen(msg));
			buffLen = LCPProtocolRejectBuild(pppConn, rej, buff);
			FORM_SET_DATA(lcp, buff, buffLen);
			Log(LOGMEDIUM, "Sending Protocol-Reject for %s\n",
				PPPProtocolToString(rej->rejectedProtocol,protStr));
			LCPSend(pppConn, lcp, 0, 0);
			
			echoStatus = ANVLProtocolEcho(config, 0, IPCP);
			if (echoStatus) {
			  status = FALSE;
			  Log(LOGMEDIUM, 
				  "! Should not have received ICMP Echo after rejecting IP\n");
			}
			break;

		  case 1: /* reject IPCP, no more IPCP */
			FORM_SET_FIELD(rej, rejectedProtocol, pppTypeIPCP);
			FORM_SET_FIELD(lcp, code, lcpTypeProtocolReject);
			SPrintf(msg, "ANVL-PPP-%s", testNum);
			FORM_SET_VARFIELD(rej, rejectedInfo, (ubyte *)msg, StrLen(msg));
			buffLen = LCPProtocolRejectBuild(pppConn, rej, buff);
			FORM_SET_DATA(lcp, buff, buffLen);
			Log(LOGMEDIUM, "Sending Protocol-Reject for %s\n",
				PPPProtocolToString(rej->rejectedProtocol,protStr));
			LCPSend(pppConn, lcp, 0, 0);

			ipcpState = NCPEstConnStateCreate(IPCP);
			FORM_SET_FIELD(IPCP_CFG(ipcpState->localOpts), ipAddress, 
						   ipif->ip);
			ncpStatus =  NCPEstConn(pppConn, IPCP, ipcpState);
			if (ncpStatus) {
			  status = FALSE;
			  Log(LOGMEDIUM, 
				  "! IPCP was re-established after rejecting IPCP\n");
			}
			NCPEstConnStateDestroy(ipcpState);
			break;

		  case 2: /* reject IPCP, ICMP echo? */
			FORM_SET_FIELD(lcp, code, lcpTypeProtocolReject);
			FORM_SET_FIELD(rej, rejectedProtocol, pppTypeIPCP);
			SPrintf(msg, "ANVL-PPP-%s", testNum);
			FORM_SET_VARFIELD(rej, rejectedInfo, (ubyte *)msg, StrLen(msg));
			buffLen = LCPProtocolRejectBuild(pppConn, rej, buff);
			FORM_SET_DATA(lcp, buff, buffLen);
			Log(LOGMEDIUM, "Sending Protocol-Reject for %s\n",
				PPPProtocolToString(rej->rejectedProtocol,protStr));
			LCPSend(pppConn, lcp, 0, 0);
			
			echoStatus = ANVLProtocolEcho(config, 0, IPCP);
			if (echoStatus) {
			  Log(LOGMEDIUM, 
				  "Received ICMP Echo Reply after rejecting IPCP\n");
			}
			else {
			  Log(LOGMEDIUM, 
				  "Did not receive ICMP Echo after rejecting IPCP\n");
			}
			break;

		  default:
			Error(FATAL, "Invalid protocol to reject in test %s", testNum);
		  }
		  DUTPPPResetConn(config, pppConn);
		}
		Log(LOGMEDIUM, "\n");
	  }
	DUTSendConfigCommand((ubyte *) "stop$");
	  Free(lcp);
	  Free(rej);
	}
  }
  END_TEST;


  BEGIN_TEST(
     TEST_NUM
     "9.3",
     TEST_DESCRIPTION       
     "The Identifier field MUST be changed for each Protocol-Reject sent.\n",
     TEST_REFERENCE
     "RFC 1661 s5.7 p35 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Send PPP packet with unknown Protocol field to DUT\n"
     "-  DUT: Send Protocol-Reject packet\n"
     "- ANVL: Send another PPP packet with unknown Protocol\n"
     "-  DUT: Send another Protocol-Reject packet with different id\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *rej;
	LCPAndExpectState_t *state;
	PPPForm_t *ppp;
	Packet_t *pkt;
	ubyte buff[MAX_STRING];
	ubyte4 i;
	ubyte idLast = 0;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	status = ANVLPPPEstConn(config, pppConn, 0);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else{
	  ppp = PPPFormCreate();
	  FORM_SET_FIELD(ppp, protocol, 0);

	  for(i = 0; i < 5; i++){
		state = LCPAndExpectStateCreate();
		rej = &state->ncpForm;

		state->ncpType = lcpTypeProtocolReject;
		SPrintf((byte *)buff, "ANVL-PPP-%s #%ld", testNum, i);
		FORM_SET_DATA(ppp, buff, StrLen((byte *)buff));

		Log(LOGMEDIUM, "Sending PPP packet with invalid protocol (0x%04X)\n",
			ppp->protocol);
		PPPSend(pppConn, ppp, 0);

		LogWaiting("for Protocol-Reject", &state->timeOut);
		pkt = LCPAndExpect(pppConn, 0, state);
		if(pkt == 0){
		  status = FALSE;
		  break;
		}
		Log(LOGMEDIUM, "Protocol-Reject Identifier = 0x%02X\n",
			rej->identifier);
		if((i != 0) && (rej->identifier == idLast)){
		  Log(LOGMEDIUM, "! Protocol-Reject Identifier did not change\n");
		  status = FALSE;
		  break;
		}
		idLast = rej->identifier;
		Free(state);
	  }

	  Free(ppp);
	  DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;
    
  BEGIN_TEST(
     TEST_NUM
     "9.4",
     TEST_DESCRIPTION
     "The Rejected-Information MUST be truncated to comply with the peer's\n"
     "established MRU.\n",
     TEST_REFERENCE
     "RFC 1661 s5.7 p35 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Send PPP packet with unknown protocol and length = MRU\n"
     "-  DUT: Should truncate the Rejected-Information to comply with MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcpIn;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;
	PPPForm_t *ppp;
	Packet_t *pkt;
	byte tmpStr[MAX_STRING];
	ubyte testStr[MAX_STRING * 2];
	ubyte2 mru = MAX_STRING;

	DUTstart(0);
	/*see config IDs spec on epoc client side*/(0);

	lcpState = NCPEstConnStateCreate(LCP);
	/* Set our MRU low */
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), mru, mru);

	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if(!status){
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	}
	else if(!LCP_CFG(lcpState->localOptsAckd)->mruOK ||
			(LCP_CFG(lcpState->localOptsAckd)->mru != mru)){
	  Log(LOGMEDIUM, "! MRU Request of %u was not Acknowleged\n", mru);
	  status = FALSE;
	}
	else{
	  state = LCPAndExpectStateCreate();
	  ppp = PPPFormCreate();
	  lcpIn = &state->ncpForm;

	  /* Want to grab the reject */
	  state->ncpType = lcpTypeProtocolReject;

	  /* This should cause one */
	  SPrintf(tmpStr, "ANVL-%s-%s ", protocol, testNum);
	  RepeatStringIntoData(tmpStr, StrLen(tmpStr),
						   testStr, mru);
	  FORM_SET_FIELD(ppp, protocol, 0);
	  FORM_SET_DATA(ppp, (ubyte *)testStr, mru);	  

	  Log(LOGMEDIUM, "Sending PPP Packet with bad protocol (%u)\n",
		  ppp->protocol);
	  PPPSend(pppConn, ppp, 0);

	  LogWaiting("for Protocol-Reject", &state->timeOut);
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if(pkt == 0){
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM, "Validating Protocol-Reject data and length\n");
		if(lcpIn->length != mru){
		  Log(LOGMEDIUM,
			  "! Protocol-Reject length (%u) not truncated to MRU (%u)\n",
			  lcpIn->length, mru);
		  status = FALSE;
		}
		if(lcpIn->dataLen != (ubyte4)(mru - LCP_HDR_LEN)){
		  Log(LOGMEDIUM,
			  "! Protocol-Reject data length (%lu) not "
			  "truncated to MRU - %u (%u)\n",
			  lcpIn->dataLen, LCP_HDR_LEN, mru - LCP_HDR_LEN);
		  status = FALSE;
		}

		if(!DATA_EQUAL(ppp->data,
					   (ubyte4)(mru - (LCP_HDR_LEN+LCP_CONFIG_OPTION_HDR_LEN)),
					   lcpIn->data + 2,
					   lcpIn->dataLen - 2)){
		  Log(LOGMEDIUM,
			  "! Protocol-Reject data does not equal sent packet\n");
		  status = FALSE;
		}
	  }
	  Free(state);
	  Free(ppp);
	  DUTPPPResetConn(config, pppConn);
	}
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "9.5",
     TEST_DESCRIPTION
     "Protocol-Reject of LCP should cause full connection shutdown\n",
     TEST_REFERENCE
     "RFC 1661 s4.5 p19 Events\n",
     TEST_METHOD
     "- ANVL: Open an LCP connection\n"
     "- ANVL: Send Protocol-Reject for LCP\n"
     "-  DUT: Should shut down all layers\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	/* A Protocol-Reject for LCP at any point is catastrophic error
	   RXJ- and must shutdown the link */
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPForm_t *lcpRej, *lcpEcho;
	LCPProtocolRejectForm_t *rej;
	LCPEchoDiscardForm_t *echo;
	LCPAndExpectState_t *state;
	byte msg[MAX_STRING];
	ubyte buff[MAX_STRING];
	ubyte4 buffLen;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  status = FALSE;
	}
	else{
	  lcpRej = LCPFormCreate();
	  rej = LCPProtocolRejectFormCreate();
	  lcpEcho = LCPFormCreate();
	  echo = LCPEchoDiscardFormCreate();
	  state = LCPAndExpectStateCreate();

	  /* send protocol reject */
	  FORM_SET_FIELD(rej, rejectedProtocol, pppTypeLCP);
	  SPrintf(msg, "ANVL-PPP-%s", testNum);
	  FORM_SET_VARFIELD(rej, rejectedInfo, (ubyte *)msg, StrLen(msg));
	  buffLen = LCPProtocolRejectBuild(pppConn, rej, buff);

	  FORM_SET_FIELD(lcpRej, code, lcpTypeProtocolReject);
	  FORM_SET_DATA(lcpRej, buff, buffLen);
	  Log(LOGMEDIUM, "Sending Protocol-Reject for LCP\n");
	  LCPSend(pppConn, lcpRej, 0, 0);

	  Log(LOGMEDIUM, "Sending LCP Echo to make sure link was shut down\n");
	  state->expectedPkts = 0;
	  status = !(boolean)LCPEchoAndExpect(pppConn,echo,lcpEcho,state,testNum);
	  if(!status){
		Log(LOGMEDIUM, "! Protocol-Reject did not cause link shutdown\n");
	  }
	  Free(lcpRej);
	  Free(rej);
	  Free(lcpEcho);
	  Free(echo);
	  Free(state);
	}

	NCPEstConnStateDestroy(lcpState);
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "9.6",
     TEST_DESCRIPTION
     "Protocol-Rejects with bad lengths should be ignored\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.7 p35-36 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Establish connection\n"
     "- ANVL: Send Protocol-Rejects for LCP with incorrect lengths\n"
	 "- ANVL: Send Echo-Request\n"
     "-  DUT: Should reply\n"
     "- CASE: length = 0-5\n"
     "- CASE: stated length > actual\n"
     "- CASE: Rejected-Information length = MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	NCPGrabOptionState_t *state;
	LCPForm_t *lcp;
	LCPProtocolRejectForm_t *rej;
	ubyte i;
	boolean lcpStatus, echoStatus;
	ubyte buff[MAX_PACKET_LEN];
	byte msg[MAX_STRING];
	ubyte4 buffLen, magic;
	ubyte2 bogusLengths[] = { 0, 1, 2, 3, 4, 5, 
								  /* >actual */ LCP_DEFAULT_MRU-1,
								  /* len=MRU */ LCP_DEFAULT_MRU };


	for (i=0; i < NUM_ELEMENTS(bogusLengths); i++) {

	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  lcpState = NCPEstConnStateCreate(LCP);
	  state = NCPGrabOptionStateCreate();

	  lcpState->UserHandler = NCPGrabOptionHandler;
	  state->cfgCode = lcpTypeConfigureRequest;
	  state->optType = 0; /* grab any packet */
	  lcpState->userData = (void *)state;

	  Log(LOGMEDIUM, "Establish PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if(!lcpStatus){
		status = FALSE;
		Log(LOGMEDIUM, "! PPP Connection not established\n");
	  }
	  else {
		/* this assumes that state->cfgPKt != 0 */
		lcp = LCPFormCreate();
		rej = LCPProtocolRejectFormCreate();
	
		magic = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
			LCP_CFG(lcpState->localOptsAckd)->magicNum:0;

		FORM_SET_VARFIELD(rej, rejectedInfo, 
						  state->cfgPkt->data, state->cfgPkt->len);
		FORM_SET_FIELD(rej, rejectedProtocol, pppTypeLCP);

		buffLen = LCPProtocolRejectBuild(pppConn, rej, buff);
		FORM_SET_FIELD(lcp, code, lcpTypeProtocolReject);
		FORM_SET_FIELD(lcp, length, bogusLengths[i]);
		FORM_SET_DATA(lcp, buff, buffLen);

		Log(LOGMEDIUM, 
			"Sending Protocol Reject for LCP with incorrect length %u\n", 
			bogusLengths[i]);
		LCPSend(pppConn, lcp, 0, 0);

		SPrintf(msg, "%s #%u %d/0x%04X (after Protocol Reject)", 
				testNum, i, bogusLengths[i], bogusLengths[i]);
		echoStatus = LCPEchoAYT(pppConn, magic, msg);
		if (!echoStatus) {
		  Log(LOGMEDIUM, "! Did not receive expected Echo Reply\n");
		  status = FALSE;
		}
		PacketDestroy(state->cfgPkt);
		Free(lcp);
		Free(rej);
	  }
	  NCPEstConnStateDestroy(lcpState);
	  Free(state);
	  DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "9.7",
     TEST_DESCRIPTION
     "Discard Protocol-Reject with unknown protocol\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.7 p35-36 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Send Protocol-Reject for unknown protocol\n"
     "-  DUT: Discard Protocol-Reject\n"
     "- ANVL: Check that connection is still up\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	ubyte2 bogusProtocol[] = { 0xC022, 0, 0x8037, 0xFFFF, 0x0022 };
	NCPGrabOptionState_t *state;
	LCPForm_t *lcp;
	LCPProtocolRejectForm_t *rej;
	ubyte i;
	boolean lcpStatus, echoStatus;
	ubyte buff[MAX_PACKET_LEN];
	byte msg[MAX_STRING];
	ubyte4 buffLen, magic;


	for (i=0; i < NUM_ELEMENTS(bogusProtocol); i++) {

	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  lcpState = NCPEstConnStateCreate(LCP);
	  state = NCPGrabOptionStateCreate();

	  lcpState->UserHandler = NCPGrabOptionHandler;
	  state->cfgCode = lcpTypeConfigureRequest;
	  state->optType = 0; /* grab any packet */
	  lcpState->userData = (void *)state;

	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if(!lcpStatus){
		status = FALSE;
		Log(LOGMEDIUM, "! PPP Connection not established\n");
	  }
	  else {
		/* this assumes that state->cfgPKt != 0 */
		lcp = LCPFormCreate();
		rej = LCPProtocolRejectFormCreate();
	
		magic = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
			LCP_CFG(lcpState->localOptsAckd)->magicNum:0;

		FORM_SET_VARFIELD(rej, rejectedInfo, 
						  state->cfgPkt->data, state->cfgPkt->len);
		FORM_SET_FIELD(rej, rejectedProtocol, bogusProtocol[i]);

		buffLen = LCPProtocolRejectBuild(pppConn, rej, buff);
		FORM_SET_FIELD(lcp, code, lcpTypeProtocolReject);
		FORM_SET_DATA(lcp, buff, buffLen);

		SPrintf(msg, "%s #%u %d/0x%04X (before Protocol Reject)", 
				testNum, i, bogusProtocol[i], bogusProtocol[i]);
		echoStatus = LCPEchoAYT(pppConn, magic, msg);
		if (!echoStatus) {
		  status = FALSE;
		}

		Log(LOGMEDIUM, 
			"Sending Protocol Reject for invalid protocol 0x%04X\n", 
			bogusProtocol[i]);
		LCPSend(pppConn, lcp, 0, 0);

		SPrintf(msg, "%s #%u %d/0x%04X (after Protocol Reject)", 
				testNum, i, bogusProtocol[i], bogusProtocol[i]);
		echoStatus = LCPEchoAYT(pppConn, magic, testNum);
		if (!echoStatus) {
		  status = FALSE;
		}
		DUTPPPResetConn(config, pppConn);
	        DUTSendConfigCommand((ubyte *) "stop$");

		PacketDestroy(state->cfgPkt);
		Free(lcp);
		Free(rej);
	  }
	  NCPEstConnStateDestroy(lcpState);
	  Free(state);
	}
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "9.8",
     TEST_DESCRIPTION
     "Protocol-Reject packets can only be sent in the LCP Opened state\n",
     TEST_REFERENCE
     "RFC 1661 s5.7 p35-36 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Close connection\n"
     "- ANVL: Send packet with unknown protocol\n"
     "-  DUT: Should not send Protocol-Reject\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	PPPForm_t *ppp;
	ubyte2 bogusProto[] = { 0xC022, 0xC022, 0xC024, 0xFFFF };
	ubyte4 i;
	LCPAndExpectState_t *state;
	boolean tmpStatus;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	ppp = PPPFormCreate();
	state = LCPAndExpectStateCreate();

	Log(LOGMEDIUM, "No PPP connection established\n");
	for (i=0; i < NUM_ELEMENTS(bogusProto); i++) {
	  Log(LOGMEDIUM, "Sending packet with unknown protocol 0x%04X\n",
		  bogusProto[i]);

	  FORM_SET_FIELD(ppp, protocol, bogusProto[i]);
	  
	  PPPSend(pppConn, ppp, 0);

	  state->ncpType = lcpTypeProtocolReject;
	  state->expectedPkts = 0;
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (pkt) {
		status = FALSE;
	  }
	}

	/* Establish connection and then reset to make sure that things
       are still working properly. */
	tmpStatus = LCPEstConn(pppConn, 0);
	if (!tmpStatus){
	  Log(LOGMEDIUM, "! Could not establish connection\n");
	  status = FALSE;
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	
	Free(state);
	Free(ppp);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "9.9",
     TEST_DESCRIPTION
     "Protocol-Reject of LCP (RXJ- in state 2-9) causes full shutdown\n",
     TEST_REFERENCE
     "RFC 1661 s4.1 p12 State Transition Table (RXJ-)\n",
     TEST_METHOD
     "- ANVL: Begin opening an LCP connection\n"
     "- ANVL: Send Protocol-Reject for LCP packets during negotiation\n"
     "-  DUT: Shut down connection\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	/* A Protocol-Reject for LCP at any point, including before LCP
	   Open state is catastrophic error RXJ- and must shutdown the
	   link */
    NCPEstConnState_t *lcpState;
	struct ProtocolReject_s state;
	boolean lcpStatus;
	ubyte i;

	/* This loop is designed so that the first Protocol Reject will be
	   received by the box in different states.  If the first packet
	   is skipped (when sawOne is FALSE), then the box will not get a
	   Protocol Reject for LCP until the box is already in state 7
	   (Ack-Rcvd).  If the first packet is not skipped, the box will
	   get the first Protocol Reject for LCP in state 6 (Req-Sent)

	   Different behaviour for these two cases has been observed.
	   */


	for (i=0; i<2; i++) {
	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  state.sawOne = (i==0)?FALSE:TRUE;
	  state.protocol = pppTypeLCP;

	  lcpState = NCPEstConnStateCreate(LCP);
	  lcpState->UserHandler = (PacketHandler_t *)LCPRejHandler;
	  lcpState->userData = &state;
	  
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if(lcpStatus){
		status = FALSE;
		Log(LOGMEDIUM, "! Protocol-Rejects were incorrectly ignored\n");
	  }
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;


/* +++alan: this should be written so it doesn't require LIB_IP */

  BEGIN_TEST(
     TEST_NUM
     "9.10",
     TEST_DESCRIPTION
     "Protocol-Reject packets received in any state other than LCP Opened\n"
     "SHOULD be silently discarded\n",
     TEST_REFERENCE
     "RFC 1661 s5.7 p36 Protocol-Reject\n",
     TEST_METHOD
     "- ANVL: Begin opening an LCP connection\n"
     "- ANVL: Send Protocol-Reject for IPCP packets during negotiation\n"
     "-  DUT: Bring LCP link up\n"
	 "- ANVL: Negotiate IPCP connection using rejected protocol\n"
	 "-  DUT: Bring requested IPCP link up\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
    NCPEstConnState_t *lcpState;
	struct ProtocolReject_s state;
	boolean lcpStatus, ncpStatus;

	state.sawOne = FALSE;
	state.protocol = pppTypeIPCP;
    DUTstart(0);/*see config IDs spec on epoc client side*/
    lcpState = NCPEstConnStateCreate(LCP);
    lcpState->UserHandler = (PacketHandler_t *)LCPRejHandler;
    lcpState->userData = &state;


    lcpStatus = LCPEstConn(pppConn, lcpState);
    if(!lcpStatus){
	  status = FALSE;
      Log(LOGMEDIUM, "! Could not establish connection\n");
    }
	else {
	  /* bring up IPCP and do an echo, which should work */
	  ncpStatus = ANVLNCPEstConnAndEcho(config, 0, IPCP, 0, 0);
	  if (!ncpStatus) {
		status = FALSE;
		Log(LOGMEDIUM, 
			"! DUT did not respond to ICMP Echo on IPCP connection,\n"
			"! Protocol Rejects were not ignored\n");
	  }
	}
    NCPEstConnStateDestroy(lcpState);
    DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  /* Some compilers choke on really big functions, split this into 2 */
  PPPTestsRunPart2(config, protocol, pppConn,
				   requestTimeOut, echoRequestTimeOut, echoSend);
}


static void
PPPTestsRunPart2(ANVLConfig_t *config, byte *protocol, NetConn_t *pppConn,
				 Time_t requestTimeOut, Time_t echoRequestTimeOut,
				 boolean echoSend)
{
  boolean status;
  byte *testNum;

  /* SECTION 5.8 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "10.1",
     TEST_DESCRIPTION
     "Echo-Request fields correctly filled in\n",
     TEST_REFERENCE
     "RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Cause DUT to send Echo-Request\n"
     "-  DUT: Send Echo-Request packet\n"
     "- ANVL: Validate fields: \n"
     "        Code                       9\n"
     "        Identifier                 any\n"
     "        Length                     packet length (<= MRU)\n"
     "        Magic-Number               0 or negotiated Magic-Number\n"
     "        Data                       length - 8 bytes of data\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPEchoDiscardForm_t lcpEchoForm;
	LCPAndExpectState_t *state;

	state = LCPAndExpectStateCreate();

	 DUTstart(8);/*see config IDs spec on epoc client side*/

	if (!ANVLPPPEstConn(config, pppConn, 0)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  DUTSendPPPEchoRequest(config);

	  state->ncpType = lcpTypeEchoRequest;
	  state->timeOut = echoRequestTimeOut;
	  LogWaiting("for LCP Echo-Request", &state->timeOut);
	  pkt = LCPAndExpect(pppConn, 0, state);

	  if (pkt) {
		LCPEchoDiscardToForm(pkt->data, pkt->len, &lcpEchoForm);
		
		/* check length */
		if (state->ncpForm.length != (LCP_HDR_LEN + pkt->len)) {
		  Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
			  "length (%lu)\n",
			  state->ncpForm.length, (LCP_HDR_LEN + pkt->len));
		  status = FALSE;
		}
		
		/* +++alan: check magic number */
		
		/* +++alan: check data */
	  }
	  else {
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "10.2",
     TEST_DESCRIPTION
     "Upon reception of an Echo-Request, a correctly formatted Echo-Reply\n"
	 "is sent\n",
     TEST_REFERENCE
     "RFC 1661 s5.8 p36 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Send Echo-Request packet\n"
     "-  DUT: Send Echo-Reply packet\n"
     "- ANVL: Validate fields: \n"
     "        Code                       10\n"
     "        Identifier                 same as Echo-Request\n"
     "        Length                     packet length (<= MRU)\n"
     "        Magic-Number               0 unless Magic-Number negotiated\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho, lcpEchoForm;
	NCPEstConnState_t *lcpState;
	ubyte4 magicNum = LCP_DIFFERENT_MAGIC_NUM;
	ubyte ident = NCPGetNextIdentifier();
	byte echoStr[MAX_STRING];
	LCPAndExpectState_t *state;

	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	state = LCPAndExpectStateCreate();

    DUTstart(0);/*see config IDs spec on epoc client side*/

	SPrintf(echoStr, "ANVL-PPP-%s", testNum);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum, magicNum);
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  Log(LOGMEDIUM, "Sending LCP Echo-Request\n");

	  FORM_SET_FIELD(lcpEcho, magicNumber, magicNum);
	  FORM_SET_DATA(lcpEcho, (ubyte *)echoStr, StrLen(echoStr));

	  FORM_SET_FIELD(lcp, identifier, ident);

	  pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	  if (pkt) {
		LCPEchoDiscardToForm(pkt->data, pkt->len, &lcpEchoForm);
		
		Log(LOGMEDIUM, "Validating Echo-Reply packet\n");

		/* check identifier */
		if (state->ncpForm.identifier != ident) {
		  Log(LOGMEDIUM, "! LCP Echo-Reply identifier (0x%02X) does not match "
			  "Echo-Request identifier (0x%02X)\n",
			  state->ncpForm.identifier, ident);
		  status = FALSE;
		}
		
		/* check magic number */
		if (LCP_CFG(lcpState->remoteOptsAckd)->magicNumOK) {
		  /* magic number was negotiated */
		  if (lcpEchoForm.magicNumber !=
			  LCP_CFG(lcpState->remoteOptsAckd)->magicNum) {
			Log(LOGMEDIUM,
				"! LCP Echo-Reply Magic Number (0x%08lX) does not "
				"match negotiated\n"
				"  magic number (0x%08lX)\n",
				lcpEchoForm.magicNumber,
				LCP_CFG(lcpState->remoteOptsAckd)->magicNum);
			status = FALSE;
		  }
		  /* should not match OUR magic num */
		  if (lcpEchoForm.magicNumber == magicNum){
			Log(LOGMEDIUM,
				"! LCP Echo-Reply Magic Number (0x%08lX) is the "
				"same as in the Echo-Request\n", lcpEchoForm.magicNumber);
			status = FALSE;
		  }
		}
		else {
		  /* magic number not negotiated */
		  if (lcpEchoForm.magicNumber != 0) {
			Log(LOGMEDIUM,
				"! LCP Echo-Reply Magic Number (0x%08lX) incorrect.\n"
				"! Magic Number was not negotiated; should be 0\n",
				lcpEchoForm.magicNumber);
			status = FALSE;
		  }
		}

		/* check data */
		if (!DATA_EQUAL(lcpEchoForm.data,lcpEchoForm.dataLen,
						echoStr, StrLen(echoStr))){
		  Log(LOGMEDIUM,
			  "\nNOTE: Echo-Reply data does not match Echo-Request data.\n"
			  "RFC-1548 required that Echo-Reply data match the data\n"
			  "of the Echo-Request.  Althought RFC-1661 no longer has this\n"
			  "requirement, many devices still support this.  This may\n"
			  "indicate a problem with the DUT.\n");
		}
	  }
	  else {
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);

	Free(state);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(lcp);
	Free(lcpEcho);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "10.3",
     TEST_DESCRIPTION
     "On transmission, the Identifier field MUST be changed whenever\n"
     "a valid reply has been received for a previous request.\n",
     TEST_REFERENCE
     "RFC 1661 s5.8 p36 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Cause DUT to send Echo-Request packet\n"
     "-  DUT: Send Echo-Request packet\n"
     "- ANVL: Send Echo-Reply\n"
     "- ANVL: Cause DUT to send Echo-Request packet\n"
     "-  DUT: Send Echo-Request packet\n"
     "- ANVL: Send Echo-Reply\n"
     "- ANVL: Verify that Identifier is different\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	ubyte4 ident = 0;
	LCPAndExpectState_t *state;

	state = LCPAndExpectStateCreate();
    DUTstart(8);/*see config IDs spec on epoc client side*/

	if (!ANVLPPPEstConn(config, pppConn, 0)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  DUTSendPPPEchoRequest(config);

	  state->ncpType = lcpTypeEchoRequest;
	  state->timeOut = echoRequestTimeOut;
	  LogWaiting("for first LCP Echo-Request", &state->timeOut);
	  pkt = LCPAndExpect(pppConn, 0, state);

	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		ident = state->ncpForm.identifier;

		DUTSendPPPEchoRequest(config);
		
		LogWaiting("for second LCP Echo-Request", &state->timeOut);
		
		pkt = LCPAndExpect(pppConn, 0, state);
		
		if (pkt) {
		  /* check identifier */
		  if (state->ncpForm.identifier == ident) {
			Log(LOGMEDIUM, "! LCP Identifier (0x%02X) was not changed\n",
				state->ncpForm.identifier);
			status = FALSE;
		  }
		}
		else {
		  status = FALSE;
		}
	  }
	}
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "10.4",
     TEST_DESCRIPTION
     "Echo-Request and Echo-Reply packets MUST only be sent in the LCP\n"
     "Opened state; any received when not in that state are discarded.\n",
     TEST_REFERENCE
     "RFC 1661 s5.8 p36 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Close connection\n"
     "- ANVL: Cause DUT to send Echo-Request\n"
     "-  DUT: Should not send Echo-Request\n"
     "- ANVL: Send Echo-Request\n"
     "-  DUT: Should not send Echo-Reply\n"
     "- ANVL: Send Echo-Reply\n"
     "-  DUT: Should not crash\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	ubyte4 i, magicNum = LCP_DIFFERENT_MAGIC_NUM;
	LCPAndExpectState_t *state;

    DUTstart(8);/*see config IDs spec on epoc client side*/

	Log(LOGMEDIUM, "No PPP connection established\n");
	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();
	state = LCPAndExpectStateCreate();

	if (echoSend) {
	  state->ncpType = lcpTypeEchoRequest;
	  state->timeOut = echoRequestTimeOut;
	  state->expectedPkts = 0;
	  
	  DUTSendPPPEchoRequest(config);
	  LogWaiting("for LCP Echo-Request", &state->timeOut);
	  pkt = LCPAndExpect(pppConn, 0, state);
	  
	  if (pkt) {
		status = FALSE;
	  }
	}
	
	/* Send 3 Echo-Request, then 3 Echo-Reply */
	for (i=0; i<6; i++) {
	  Log(LOGMEDIUM, "Sending LCP Echo-%s\n",
		  i<3? "Request" : "Reply");

	  FORM_SET_FIELD(lcpEcho, magicNumber, magicNum);

	  FORM_SET_FIELD(lcp, code, i<3? lcpTypeEchoRequest : lcpTypeEchoReply);

	  state->ncpType = 0;
	  state->expectedPkts = 0;
	  state->ignoreType = lcpTypeConfigureRequest;
	  pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	  if (pkt) { 

		/*+++dbaker: it would be a good idea to check the received
		  packet to see if ANVL received a magic number in the
		  Echo-Reply and if so, make sure that it is not the same as
		  the magic number that ANVL sent in the Echo-Request because
		  that would indicate a loop-back situation and that is
		  bad. */
		status = FALSE; 
	  }
	}

	DUTSendConfigCommand((ubyte *) "stop$");
	Free(state);
	Free(lcp);
	Free(lcpEcho);
  }
  END_TEST;

/* 
  +++topher this test description was removed in RFC 1661, but it's a good
  test, so leave in the old reference.
 */
  BEGIN_TEST(
     TEST_NUM
     "10.5",
     TEST_DESCRIPTION
     "Echo-Reply Data must be truncated to accommodate peer's MRU\n",
     TEST_REFERENCE
     "RFC 1548 s5.8 p37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Open connection with ANVL MRU < DUT MRU\n"
     "- ANVL: Send Echo-Request with data len = DUT MRU\n"
     "-  DUT: Send Echo-Reply\n"
     "- ANVL: Check that data len is truncated to ANVL MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	NCPEstConnState_t *lcpState;
	ubyte4 magicNum = LCP_DIFFERENT_MAGIC_NUM;
	LCPAndExpectState_t *state;

    DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	state = LCPAndExpectStateCreate();

	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), mru, 300);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum, magicNum);
	Log(LOGMEDIUM, "Establish PPP connection requesting MRU of %u\n",
		LCP_CFG(lcpState->localOpts)->mru);
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  FORM_SET_FIELD(lcpEcho, magicNumber, magicNum);
	  FORM_SET_FIELD(lcpEcho, dataLen, 500);

	  Log(LOGMEDIUM, "Sending LCP Echo-Request with %lu bytes of data\n",
		  lcpEcho->dataLen);
	  pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	  if (pkt) {
		
		/* check length +++art: need to check that mru was acked*/
		if (state->ncpForm.dataLen > LCP_CFG(lcpState->localOpts)->mru) {
		  Log(LOGMEDIUM, "! LCP Echo-Reply data length (%lu) is greater "
			  "than negotiated MRU (%u)\n",
			  state->ncpForm.dataLen, LCP_CFG(lcpState->localOpts)->mru);
		  status = FALSE;
		}
	  }
	  else {
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);

	Free(state);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(lcp);
	Free(lcpEcho);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "10.6",
     TEST_DESCRIPTION
     "Unsolicited Echo-Reply is discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Send Echo-Reply\n"
     "-  DUT: Should not crash\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	NCPEstConnState_t *lcpState;
	ubyte4 i, magicNum = LCP_DIFFERENT_MAGIC_NUM;
	LCPAndExpectState_t *state;

    DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	state = LCPAndExpectStateCreate();

	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum, magicNum);
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  /* Send 3 unsolicited Echo-Replies */
	  for (i=0; i<3; i++) {
		Log(LOGMEDIUM, "Sending unsolicited LCP Echo-Reply\n");
		
		FORM_SET_FIELD(lcpEcho, magicNumber, magicNum);
		
		FORM_SET_FIELD(lcp, code, lcpTypeEchoReply);

		state->ncpType = 0;
		state->expectedPkts = 0;
		pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);
		
		if (pkt) {
		  status = FALSE;
		}
	  }
	}

	DUTPPPResetConn(config, pppConn);

	Free(state);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(lcp);
	Free(lcpEcho);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "10.7",
     TEST_DESCRIPTION
     "Echo-Reply with Identifier different from Echo-Request is ignored\n",
     TEST_REFERENCE
     "RFC 1661 s5.8 p36 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Cause DUT to send Echo-Request\n"
     "-  DUT: Send Echo-Request\n"
     "- ANVL: Send Echo-Reply with different id\n"
     "-  DUT: Send Echo-Request again\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcpReply;
	LCPEchoDiscardForm_t *lcpEchoReply;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;
	ubyte4 magicNum;

    DUTstart(8);/*see config IDs spec on epoc client side*/

	state = LCPAndExpectStateCreate();
	lcpState = NCPEstConnStateCreate(LCP);

	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  state->ncpType = lcpTypeEchoRequest;
	  state->timeOut = echoRequestTimeOut;
	  state->expectedPkts = 1;
	  /* don't run default handlers--would respond to Echo-Requests */
	  state->runDefaultHandlers = FALSE;

	  DUTSendPPPEchoRequest(config);
	  LogWaiting("for LCP Echo-Request\n", &state->timeOut);
	  LCPAndExpect(pppConn, 0, state);

	  if (state->receivedPkts == 0) {
		/* didn't send original Echo-Request--common function has log msg */
		status = FALSE;
	  }
	  else {
		/* DUT sent Echo-Request; create Echo-Reply */
		lcpReply = LCPFormCreate();
		lcpEchoReply = LCPEchoDiscardFormCreate();
		
		/* set magic number and bogus identifier */
		magicNum = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
		  LCP_CFG(lcpState->localOptsAckd)->magicNum:0;
		FORM_SET_FIELD(lcpEchoReply, magicNumber, magicNum);
		FORM_SET_FIELD(lcpReply, identifier, 0);

		/* send out the reply */		
		Log(LOGMEDIUM,"Sending LCP Echo-Reply with incorrect identifier "
			"(0x%08X)\n", lcpReply->identifier);
		LCPEchoReplyXmit(pppConn, &(state->ncpForm), lcpReply, lcpEchoReply);
		
		/* reset state */
		Free(state);
		state = LCPAndExpectStateCreate();
		
		state->ncpType = lcpTypeEchoRequest;
		state->timeOut = echoRequestTimeOut;
		state->expectedPkts = 1;
		state->runDefaultHandlers = FALSE;
		
		/* check that DUT resent Echo-Request */
		LogWaiting("for LCP Echo-Request\n", &state->timeOut);
		LCPAndExpect(pppConn, 0, state);

		if (state->receivedPkts == 0) {
		  /* DUT did not re-send Echo-Request--common function has log msg */
		  status = FALSE;
		}

		/* clean up */
		Free(lcpReply);
		Free(lcpEchoReply);
	  }
	}
	
	Free(state);
	NCPEstConnStateDestroy(lcpState);
	
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "10.8",
     TEST_DESCRIPTION
     "Echo-Requests with non-zero Magic-Numbers are discarded\n"
     "when Magic-Number has not been negotiated\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Open connection with no Magic-Numbers\n"
     "- ANVL: Send Echo-Request with Magic-Number\n"
     "-  DUT: Should not send Echo-Reply\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	ubyte4 magicNum = LCP_DIFFERENT_MAGIC_NUM;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;

	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();
	state = LCPAndExpectStateCreate();
	DUTstart(0);/*see config IDs spec on epoc client side*/
	lcpState = NCPEstConnStateCreate(LCP);

    /*see config IDs spec on epoc client side*/(0);

	Log(LOGMEDIUM, "Establish PPP connection with no Magic-Number option\n");
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else if (LCP_CFG(lcpState->localOptsAckd)->magicNumOK) {
	  /* DUT negotiated magic number; stop test */
	  Log(LOGMEDIUM, "! Unable to Establish PPP connection without "
		  "Magic-Number option\n");
	  status = FALSE;
	}
	else {
	  /* PPP connection established without Magic-Number option */
	  FORM_SET_FIELD(lcpEcho, magicNumber, magicNum);
	  state->expectedPkts = 0;

	  Log(LOGMEDIUM, "Sending LCP Echo-Request with magic number 0x%08lX\n",
		  lcpEcho->magicNumber);
	  LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);
	  
	  if (state->receivedPkts) {
		/* DUT sent Echo-Reply--common function has log msg */
		status = FALSE;
	  }
	}

	Free(lcp);
	Free(lcpEcho);
	Free(state);
	NCPEstConnStateDestroy(lcpState);
	
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "10.9",
     TEST_DESCRIPTION
     "Echo-Requests with bad Magic-Numbers are discarded\n"
     "when Magic-Number has been negotiated\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Open connection with Magic-Numbers\n"
     "- ANVL: Send Echo-Request with invalid Magic-Number\n"
     "-  DUT: Should not send Echo-Reply\n"
     "- CASE: Non-zero Magic-Number not equal to negotiated Magic-Number\n"
     "- CASE: Magic-Number = 0\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	NCPEstConnState_t *lcpState;
	ubyte4 i, invalidMagicNum[] = { LCP_DIFFERENT_MAGIC_NUM + 1, 0 };
	LCPAndExpectState_t *state;

	/* turn on magic number option on DUT */
	DUTSetPPPMagicNumber(config, TRUE);
    DUTstart(0);/*see config IDs spec on epoc client side*/

	
	lcpState = NCPEstConnStateCreate(LCP);

	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum,
				   LCP_DIFFERENT_MAGIC_NUM);
	Log(LOGMEDIUM,
		"Establish PPP connection requesting Magic Number 0x%08lX\n",
		LCP_CFG(lcpState->localOpts)->magicNum);
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else if (!LCP_CFG(lcpState->localOptsAckd)->magicNumOK) {
	  /* unable to negotiate magic number; stop test */
	  Log(LOGMEDIUM, "! Unable to Establish PPP connection with "
		  "Magic-Number option\n");
	  status = FALSE;
	}
	else {
	  /* PPP connection established with Magic-Number negotiated */
	  for (i = 0; i < NUM_ELEMENTS(invalidMagicNum); i++) {
		lcp = LCPFormCreate();
		state = LCPAndExpectStateCreate();
		lcpEcho = LCPEchoDiscardFormCreate();

		FORM_SET_FIELD(lcpEcho, magicNumber, invalidMagicNum[i]);
		state->expectedPkts = 0;

		Log(LOGMEDIUM, "Sending LCP Echo-Request with incorrect magic "
			"number (0x%08lX)\n", lcpEcho->magicNumber);
		LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

		if (state->receivedPkts) {
		  /* DUT sent Echo-Reply--common function has log msg */
		  status = FALSE;
		}

		/* clean up */
		Free(state);
		Free(lcpEcho);
		Free(lcp);
	  }
	}
	
	NCPEstConnStateDestroy(lcpState);
	
	/* turn off magic number option on DUT */
	DUTSetPPPMagicNumber(config, FALSE);
	
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "10.10",
     TEST_DESCRIPTION
     "Echo-Requests with bad lengths are discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Send Echo-Request with incorrect length\n"
     "-  DUT: Should not send Echo-Reply\n"
     "- ANVL: Send Echo-Request with correct length\n"
     "-  DUT: Should send Echo-Reply\n"
     "- CASE: length 0-7\n"
     "- CASE: stated length > actual length\n"
     "- CASE: data length = default MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	NCPEstConnState_t *lcpState;
	ubyte4 i, magicNum = LCP_DIFFERENT_MAGIC_NUM;
	ubyte2 bogusLength[] = { 0, 1, 2, 3, 4, 5, 6, 7, 45, LCP_DEFAULT_MRU };
	LCPAndExpectState_t *state;

    DUTstart(0);/*see config IDs spec on epoc client side*/
	lcpState = NCPEstConnStateCreate(LCP);

	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum, magicNum);
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  /* PPP connection established; get magic number if negotiated */
	  magicNum = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
		LCP_CFG(lcpState->localOptsAckd)->magicNum:0;
	  
	  for (i=0; i<NUM_ELEMENTS(bogusLength); i++) {
		/* create forms and set fields each time */
		lcp = LCPFormCreate();
		lcpEcho = LCPEchoDiscardFormCreate();
		state = LCPAndExpectStateCreate();

		FORM_SET_FIELD(lcpEcho, magicNumber, magicNum);
		FORM_SET_FIELD(lcp, length, bogusLength[i]);
		state->expectedPkts = 0;
		
		Log(LOGMEDIUM, "Sending LCP Echo-Request with incorrect length %u\n",
			bogusLength[i]);
		LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);
		
		if (state->receivedPkts) {
		  /* DUT sent Echo-Reply--common function has log msg */
		  status = FALSE;
		}

		/* Send good Echo Request, but clean-up/prepare the forms */
		Free(state);
		state = LCPAndExpectStateCreate();

		CLEAR_DATA(lcp);
		CLEAR_DATA(lcpEcho);
		FORM_SET_FIELD(lcpEcho, magicNumber, magicNum);
		state->expectedPkts = 1;
		
		Log(LOGMEDIUM, "Sending LCP Echo-Request with correct length\n");
		LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);
		
		if (!state->receivedPkts) {
		  /* DUT sent Echo-Reply--common function has log msg */
		  status = FALSE;
		}

		/* clean up */
		Free(lcp);
		Free(lcpEcho);
		Free(state);
	  }
	}
	NCPEstConnStateDestroy(lcpState);
	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "10.11",
     TEST_DESCRIPTION
     "Echo-Requests should be retransmitted if no Echo-Reply is received\n",
     TEST_REFERENCE
     "RFC 1661 s5.8 p37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Cause DUT to send Echo-Request\n"
     "-  DUT: Send Echo-Request\n"
     "- ANVL: Do not send Echo-Reply\n"
     "-  DUT: Retransmit Echo-Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPAndExpectState_t *state;

    DUTstart(8);/*see config IDs spec on epoc client side*/

	state = LCPAndExpectStateCreate();

	if (!ANVLPPPEstConn(config, pppConn, 0)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else {
	  state->ncpType = lcpTypeEchoRequest;
	  state->timeOut = echoRequestTimeOut;
	  /* don't run default handlers--would respond to Echo-Requests */
	  state->runDefaultHandlers = FALSE;
	  state->expectedPkts = 2;
	  
	  DUTSendPPPEchoRequest(config);
	  LogWaiting("for LCP Echo-Request", &state->timeOut);
	  LCPAndExpect(pppConn, 0, state);

	  if (state->expectedPkts != state->receivedPkts) {
		/* common function has error message */
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "10.12",
     TEST_DESCRIPTION
     "Echo-Replies with non-zero Magic-Numbers are discarded\n"
     "when Magic-Number has not been negotiated\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Open connection with no Magic-Numbers\n"
     "- ANVL: Cause DUT to send Echo-Request\n"
     "-  DUT: Send Echo-Request\n"
     "- ANVL: Send Echo-Reply with Magic-Number\n"
     "-  DUT: Resend Echo-Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPEchoDiscardForm_t *lcpEchoReply;
	ubyte4 magicNum = LCP_DIFFERENT_MAGIC_NUM;
	LCPAndExpectState_t *state;
	NCPEstConnState_t *lcpState;

	DUTstart(8);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	
	Log(LOGMEDIUM, "Establish PPP connection with no Magic Number request\n");
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else if (LCP_CFG(lcpState->localOptsAckd)->magicNumOK) {
	  /* DUT negotiated magic number; stop test */
	  Log(LOGMEDIUM, "! Unable to Establish PPP connection without "
		  "Magic-Number option\n");
	  status = FALSE;
	}
	else {
	  state = LCPAndExpectStateCreate();
	  
	  state->ncpType = lcpTypeEchoRequest;
	  state->timeOut = echoRequestTimeOut;
	  state->expectedPkts = 1;
	  /*
		don't run default handlers as they would respond to
		Echo-Requests with a valid Echo-Reply
	  */
	  state->runDefaultHandlers = FALSE;
	  
	  DUTSendPPPEchoRequest(config);
	  LogWaiting("for LCP Echo-Request", &state->timeOut);
	  LCPAndExpect(pppConn, 0, state);

	  if (state->receivedPkts == 0) {
		/* DUT did not send Echo-Request--common function has log msg */
		status = FALSE;
	  }
	  else {
		/* DUT sent Echo-Request; set magic number for Echo-Reply */
		lcpEchoReply = LCPEchoDiscardFormCreate();
		FORM_SET_FIELD(lcpEchoReply, magicNumber, magicNum);
		
		/* send out the reply */		
		Log(LOGMEDIUM, "Sending LCP Echo-Reply with invalid magic "
			"number 0x%08lX\n",	lcpEchoReply->magicNumber);
		LCPEchoReplyXmit(pppConn, &(state->ncpForm), 0, lcpEchoReply);
		
		/* reset state and check that DUT resends echo request */
		Free(state);

		state = LCPAndExpectStateCreate();
		state->ncpType = lcpTypeEchoRequest;
		state->timeOut = echoRequestTimeOut;
		state->expectedPkts = 1;
		state->runDefaultHandlers = FALSE;
		
		LogWaiting("for LCP Echo-Request\n", &state->timeOut);
		LCPAndExpect(pppConn, 0, state);

		if (state->receivedPkts == 0) {
		  /* DUT did not re-send Echo-Request--common function has log msg */
		  status = FALSE;
		}

		/* clean up */
		Free(lcpEchoReply);
	  }

	  /* clean up */
	  Free(state);
	}
	
	NCPEstConnStateDestroy(lcpState);
  
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "10.13",
     TEST_DESCRIPTION
     "Echo-Replies with bad Magic-Numbers are discarded\n"
     "when Magic-Number has been negotiated\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Open connection with Magic-Numbers\n"
     "- ANVL: Cause DUT to send Echo-Request\n"
     "-  DUT: Send Echo-Request\n"
     "- ANVL: Send Echo-Reply with invalid Magic-Number\n"
     "-  DUT: Resend Echo-Request\n"
     "- CASE: Non-zero Magic-Number not equal to negotiated Magic-Number\n"
	 "- CASE: Magic-Number = 0\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPEchoDiscardForm_t *lcpEchoReply;
	NCPEstConnState_t *lcpState;
	ubyte4 i, invalidMagicNum[] = { LCP_DIFFERENT_MAGIC_NUM + 1, 0 };
	LCPAndExpectState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	/* turn on magic number option on DUT */
	DUTSetPPPMagicNumber(config, TRUE);
	
	lcpState = NCPEstConnStateCreate(LCP);

	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum,
				   LCP_DIFFERENT_MAGIC_NUM);
	Log(LOGMEDIUM,
		"Establish PPP connection requesting Magic Number 0x%08lX\n",
		LCP_CFG(lcpState->localOpts)->magicNum);
	if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
	  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
	  status = FALSE;
	}
	else if (!LCP_CFG(lcpState->localOptsAckd)->magicNumOK) {
	  /* unable to negotiate magic number; stop test */
	  Log(LOGMEDIUM, "! Unable to Establish PPP connection with "
		  "Magic-Number option\n");
	  status = FALSE;
	}
	else {
	  /* PPP connection established with Magic-Number negotiated */

	  for (i = 0; i < NUM_ELEMENTS(invalidMagicNum); i++) {
		
		state = LCPAndExpectStateCreate();
	  
		state->ncpType = lcpTypeEchoRequest;
		state->timeOut = echoRequestTimeOut;
		state->expectedPkts = 1;
		/*
		  don't run default handlers as they would respond to
		  Echo-Requests with a valid Echo-Reply
		*/
		state->runDefaultHandlers = FALSE;
		
		DUTSendPPPEchoRequest(config);
		LogWaiting("for LCP Echo-Request\n", &state->timeOut);
		LCPAndExpect(pppConn, 0, state);

		if (state->receivedPkts == 0) {
		  /* DUT did not send original Echo-Request--common func has log msg */
		  status = FALSE;
		}
		else {
		  /* DUT sent Echo-Request; set invalid magic number */
		  lcpEchoReply = LCPEchoDiscardFormCreate();
		  FORM_SET_FIELD(lcpEchoReply, magicNumber, invalidMagicNum[i]);
		
		  /* send out the reply */		
		  Log(LOGMEDIUM,"Sending LCP Echo Reply with incorrect magic "
			  "number (0x%08lX)\n",
			  lcpEchoReply->magicNumber);
		  LCPEchoReplyXmit(pppConn, &(state->ncpForm), 0, lcpEchoReply);
		
		  /* reset state and check that DUT resends Echo-Request */
		  Free(state);
		  state = LCPAndExpectStateCreate();
		  
		  state->ncpType = lcpTypeEchoRequest;
		  state->timeOut = echoRequestTimeOut;
		  state->expectedPkts = 1;
		  state->runDefaultHandlers = FALSE;
		  
		  LogWaiting("for LCP Echo-Request\n", &state->timeOut);
		  LCPAndExpect(pppConn, 0, state);
		  
		  if (state->receivedPkts == 0) {
			/* DUT did not send Echo-Request--common function has log msg */
			status = FALSE;
		  }
		  
		  /* clean up */
		  Free(lcpEchoReply);
		}

		/* clean up */
		Free(state);
	  }
	}
	
	NCPEstConnStateDestroy(lcpState);

	/* turn off magic number option on DUT */
	DUTSetPPPMagicNumber(config, FALSE);
	
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "10.14",
     TEST_DESCRIPTION
     "Echo-Replies with bad lengths are discarded\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s5.8 p36-37 Echo-Request and Echo-Reply\n",
     TEST_METHOD
     "- ANVL: Cause DUT to send Echo-Request\n"
     "-  DUT: Send Echo-Request\n"
     "- ANVL: Send Echo-Reply with incorrect length\n"
     "-  DUT: Resend Echo-Request\n"
     "- CASE: length 0-7\n"
     "- CASE: stated length > actual length\n"
     "- CASE: data length = default MRU\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcpReply;
	LCPEchoDiscardForm_t *lcpEchoReply;
	NCPEstConnState_t *lcpState;
	ubyte4 i, magicNum = LCP_DIFFERENT_MAGIC_NUM;
	ubyte2 bogusLength[] = { 0, 1, 2, 3, 4, 5, 6, 7, 45, LCP_DEFAULT_MRU };
	LCPAndExpectState_t *state;

	DUTstart(14);/*see config IDs spec on epoc client side*/

	for (i=0; i<NUM_ELEMENTS(bogusLength); i++) {
	  lcpState = NCPEstConnStateCreate(LCP);

	  FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum, magicNum);
	  if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
		Log(LOGMEDIUM, "! Unable to open PPP connection\n");
		status = FALSE;
	  }
	  else {
		state = LCPAndExpectStateCreate();
		
		state->ncpType = lcpTypeEchoRequest;
		state->timeOut = echoRequestTimeOut;
		state->expectedPkts = 1;
		/*
		  don't run default handlers as they would respond to
		  Echo-Requests with a valid Echo-Reply
		*/
		state->runDefaultHandlers = FALSE;
		
		DUTSendPPPEchoRequest(config);
		LogWaiting("for LCP Echo-Request\n", &state->timeOut);
		LCPAndExpect(pppConn, 0, state);
		
		if (state->receivedPkts == 0) {
		  /* DUT did not send Echo-Request--common function has log msg */
		  status = FALSE;
		}
		else {
		  /* DUT sent Echo-Request; set magic number and invalid length */
		  lcpEchoReply = LCPEchoDiscardFormCreate();
		  lcpReply = LCPFormCreate();
		  
		  magicNum = LCP_CFG(lcpState->localOptsAckd)->magicNumOK?
			LCP_CFG(lcpState->localOptsAckd)->magicNum:0;
		  FORM_SET_FIELD(lcpEchoReply, magicNumber, magicNum);
		  FORM_SET_FIELD(lcpReply, length, bogusLength[i]);
		
		  /* send out the reply */		
		  Log(LOGMEDIUM, "Sending LCP Echo-Reply with incorrect length %u\n",
			  bogusLength[i]);
		  LCPEchoReplyXmit(pppConn, &(state->ncpForm), lcpReply, lcpEchoReply);
		
		  /* reset state and check that DUT resends echo request */
		  Free(state);
		  state = LCPAndExpectStateCreate();
		  
		  state->ncpType = lcpTypeEchoRequest;
		  state->timeOut = echoRequestTimeOut;
		  state->expectedPkts = 1;
		  state->runDefaultHandlers = FALSE;
		  
		  LogWaiting("for LCP Echo-Request\n", &state->timeOut);
		  LCPAndExpect(pppConn, 0, state);
		
		  if (state->receivedPkts == 0) {
			/* DUT did not re-send Echo-Request--common function has log msg */
			status = FALSE;
		  }
		  
		  /* clean up */
		  Free(lcpEchoReply);
		  Free(lcpReply);
		}
		
		/* clean up for next case */
		Free(state);
	  }
	  
	  NCPEstConnStateDestroy(lcpState);
	
	  DUTPPPResetConn(config, pppConn);
	}
	
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;



  /* SECTION 6.1 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "12.1",
     TEST_DESCRIPTION
     "Validate format of Maximum-Receive-Unit Option fields\n",
     TEST_REFERENCE
     "RFC 1661 s6.1 p41 Maximum-Receive-Unit\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with MRU Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate Maximum-Receive-Unit Option\n"
     "        Type                       1\n"
     "        Length                     4\n"
     "        MRU                        configured value\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	ubyte2 mru;
	NCPGrabOptionState_t *state;

	DUTSetPPPMRU(config, LCP_DIFFERENT_MRU);
	DUTstart(4);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureRequest;
	state->optType = lcpConfigTypeMaximumReceiveUnit;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM, "! LCP Configure-Request did not contain MRU option\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating MRU Option Fields\n");
	  opt = LCPConfigOptionFormCreate();
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, opt);
	  
	  /* Type doesn't need checking, because that's how we found it */
	  if(opt->length != 4){
		Log(LOGMEDIUM, "! MRU Option has len = %u (should be 4)\n",
			opt->length);
		status = FALSE;
	  }
	  Unpack(opt->data, "S", &mru);
	  if(mru != LCP_DIFFERENT_MRU){
		Log(LOGMEDIUM, "! MRU Option has value = %u (configured as %u)\n",
			mru, LCP_DIFFERENT_MRU);
		status = FALSE;
	  }
	  Free(opt);	
	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);
	DUTSetPPPMRU(config, LCP_DEFAULT_MRU);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "12.2",
     TEST_DESCRIPTION
     "Check Maximum-Receive-Unit Option is functional\n",
     TEST_REFERENCE
     "RFC 1661 s6.1 p41 Maximum-Receive-Unit\n",
     TEST_METHOD
     "- ANVL: Open connection requesting MRU of < 1500 octets\n"
     "- ANVL: Send Echo-Request with len = 1500\n"
     "-  DUT: Send Echo-Response with len = MRU\n"
     "- ANVL: Close connection\n"
     "- ANVL: Cause DUT to open connection with MRU < 1500 octets\n"
     "- ANVL: Send Echo-Request with len > 1500\n"
     "-  DUT: Should not respond\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPEchoDiscardForm_t *lcpEcho;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus, tmpStatus;
	byte4 i;
	byte echoStr[MAX_ECHO_DATA + 10];

	DUTstart(4);/**//*see config IDs spec on epoc client side*/
	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();
	lcpState = NCPEstConnStateCreate(LCP);
	state = LCPAndExpectStateCreate();

	Log(LOGMEDIUM, "Establish a PPP Connection with an MRU of %d\n", 
		LCP_DIFFERENT_MRU);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), mru, LCP_DIFFERENT_MRU);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}

	/* clear out echo data then fill with some data */
	MemSet(echoStr, '\0', MAX_ECHO_DATA + 10);
	for (i = 0; i < MAX_ECHO_DATA; i++){
	  SPrintf(&(echoStr[i]), "%ld", i%10);
	}

	Log(LOGMEDIUM, "Sending LCP Echo-Request with size %d\n", LCP_DEFAULT_MRU);
	
	FORM_SET_DATA(lcpEcho, (ubyte *)echoStr, StrLen(echoStr));

	pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	if (pkt) {
	  if (pkt->len > LCP_DIFFERENT_MRU - LCP_HDR_LEN){
		Log(LOGMEDIUM, "! LCP Echo-Reply packet had length %lu, "
			"larger than MRU of %d\n", pkt->len, LCP_DIFFERENT_MRU);
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM, "LCP Echo-Reply has correct data length of %lu\n", 
			pkt->len);
	  }
	}
	else{
	  Log(LOGMEDIUM, "! No LCP Echo-Reply packet was received\n");
	  status = FALSE;
	}

	DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");

	NCPEstConnStateDestroy(lcpState);
	Free(state);

	DUTstart(4);/**//*see config IDs spec on epoc client side*/
	state = LCPAndExpectStateCreate();
	lcpState = NCPEstConnStateCreate(LCP);

	DUTSetPPPMRU(config, LCP_DIFFERENT_MRU);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}

	/* clear out echo data then fill with some data */
	MemSet(echoStr, '\0', MAX_ECHO_DATA + 10);
	for (i = 0; i < MAX_ECHO_DATA + 10; i++){
	  SPrintf(&(echoStr[i]), "%ld", i%10);
	}

	Log(LOGMEDIUM, "Sending LCP Echo-Request with size %d\n", 
		LCP_DEFAULT_MRU + 1);
	
	FORM_SET_DATA(lcpEcho, (ubyte *)echoStr, StrLen(echoStr));

	state->expectedPkts = 0;
	pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	/* LCPEchoAndExpect will take care of Log messages */
	tmpStatus = (pkt == 0);
	status = status && tmpStatus;

	DUTPPPResetConn(config, pppConn);

	DUTSetPPPMRU(config, LCP_DEFAULT_MRU);

	Free(lcp);
	Free(lcpEcho);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(state);
  }
  END_TEST;
    
  BEGIN_TEST(
     TEST_NUM
     "12.3",
     TEST_DESCRIPTION
     "The default Maximum-Receive-Unit is 1500 octets.\n",
     TEST_REFERENCE
     "RFC 1661 s6.1 p41 Maximum-Receive-Unit\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with default Options\n"
     "- ANVL: Send Echo-Request of length 1500\n"
     "-  DUT: Send Echo-Reply\n"
     "- ANVL: Send Echo-Request of length 1501\n"
     "-  DUT: Should not reply\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPEchoDiscardForm_t *lcpEcho;
	boolean lcpStatus;
	byte4 i;
	byte echoStr[MAX_ECHO_DATA + 1];
	Time_t longTimeOut = { 30, 0 };

	DUTstart(0);/*see config IDs spec on epoc client side*/
	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();
	state = LCPAndExpectStateCreate();

	lcpStatus = LCPEstConn(pppConn, 0);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}

	MemSet(echoStr, '\0', MAX_ECHO_DATA + 1);
	for (i = 0; i < MAX_ECHO_DATA; i++){
	  SPrintf(&(echoStr[i]), "%ld", i%10);
	}

	Log(LOGMEDIUM, "Sending LCP Echo-Request with size %d\n", LCP_DEFAULT_MRU);
	
	FORM_SET_DATA(lcpEcho, (ubyte *)echoStr, StrLen(echoStr));

	state->expectedPkts = 1;
	state->timeOut = longTimeOut;

	pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	if(!pkt){
	  status = FALSE;
	}

	CLEAR_DATA(lcp);
	CLEAR_DATA(lcpEcho);

	/* add one byte to the echo data to bump it over the edge */
	SPrintf(&(echoStr[MAX_ECHO_DATA]), "%d", 0);

	Log(LOGMEDIUM, "Sending LCP Echo-Request with size %d\n",
		LCP_DEFAULT_MRU+1);
	
	FORM_SET_DATA(lcpEcho, (ubyte *)echoStr, StrLen(echoStr));

	state->expectedPkts = 0;
	state->timeOut = longTimeOut;
	pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	if (pkt) {
	  status = FALSE;
	}

	DUTPPPResetConn(config, pppConn);

	DUTSendConfigCommand((ubyte *) "stop$");
	Free(lcp);
	Free(lcpEcho);
	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "12.4",
     TEST_DESCRIPTION
     "If a Maximum-Receive-Unit of less than 1500 is negotiated, packets\n"
     "of length 1500 must still be accepted\n",
     TEST_REFERENCE
     "RFC 1661 s6.1 p41 Maximum-Receive-Unit\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with MRU < 1500\n"
     "- ANVL: Send Echo-Request of length 1500\n"
     "-  DUT: Send Echo-Reply\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	LCPEchoDiscardForm_t *lcpEcho;
	byte echoStr[MAX_ECHO_DATA];
	byte4 i;

	DUTSetPPPMRU(config, LCP_DIFFERENT_MRU);
	DUTstart(4);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if (LCP_CFG(lcpState->remoteOptsAckd)->mru != LCP_DIFFERENT_MRU){
	  Log(LOGMEDIUM, 
		  "! A PPP Connection with an MRU of %d could not be established\n",
		  LCP_DIFFERENT_MRU);
	  status = FALSE;
	}
	else{
	  lcp = LCPFormCreate();
	  lcpEcho = LCPEchoDiscardFormCreate();
	  state = LCPAndExpectStateCreate();

	  /* clear out echo data then fill with some data */
	  MemSet(echoStr, '\0', MAX_ECHO_DATA);
	  for (i = 0; i < MAX_ECHO_DATA; i++){
		SPrintf(&(echoStr[i]), "%ld", i%10);
	  }

	  Log(LOGMEDIUM, "Sending LCP Echo-Request with size %d\n", 
		  LCP_DEFAULT_MRU);
	
	  FORM_SET_DATA(lcpEcho, (ubyte *)echoStr, StrLen(echoStr));

	  pkt = LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	  if (!pkt) {
		Log(LOGMEDIUM, "! No LCP Echo-Reply packet was received\n");
		status = FALSE;
	  }

	  Free(lcp);
	  Free(lcpEcho);
	  Free(state);
	}

	DUTPPPResetConn(config, pppConn);
	DUTSetPPPMRU(config, LCP_DEFAULT_MRU);

	DUTSendConfigCommand((ubyte *) "stop$");
	NCPEstConnStateDestroy(lcpState);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "12.6",
     TEST_DESCRIPTION
     "A Maximum-Receive-Unit of less than 4 should not be allowed\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s6.1 p41 Maximum-Receive-Unit\n",
     TEST_METHOD
     "- ANVL: Open connection with invalid MRU\n"
     "-  DUT: Send Configure-Nak\n"
     "- CASE: MRU = 0-3\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	byte4 i;

	lcp = LCPFormCreate();
	for (i = 0; i < 4; i++){
	  DUTstart(0);/*see config IDs spec on epoc client side*/

	  lcpState = NCPEstConnStateCreate(LCP);

	  Log(LOGMEDIUM, "Attempt to Establish a Bogus "
		  "PPP Connection with an MRU of %ld\n", i);

	  FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), mru, (ubyte2)i);

	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if(!lcpStatus){
		Log(LOGMEDIUM, "! PPP Connection Not Established\n");
		status = FALSE;
	  }
	  else{
		if (lcpState->nakReceived){
		  Log(LOGMEDIUM, "LCP Configure-Nak was correctly received\n");
		}
		else{
		  Log(LOGMEDIUM, "! Did not receive LCP Configure-Nak\n");
		  status = FALSE;
		}
	  }

	  DUTPPPResetConn(config, pppConn);
	  NCPEstConnStateDestroy(lcpState);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}

	Free(lcp);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "12.7",
     TEST_DESCRIPTION
	 "If an Option is received (MRU) in a Configure-Request but\n"
	 "with an incorrect Length, a Configure-Nak SHOULD be transmitted\n"
	 "unless the Length goes beyond the end of the packet\n",
     TEST_REFERENCE
     "RFC 1661 s6 p41 LCP Configuration Options\n",
     TEST_METHOD
     "- ANVL: Send Configure-Requests with MRU Option with incorrect length\n"
     "-  DUT: Should Configure-Nak\n"
     "- CASE: Length < 4\n"
	 "-  DUT: Should silently discard packet\n"
     "- CASE: Length > 4\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	ubyte4 i;
	for (i = 0; i < 6; i++) {
	  
	  if (i == 4){
		continue;
	  }

	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  DUTstart(0);/*see config IDs spec on epoc client side*/
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if (!lcpStatus) {
		Log(LOGMEDIUM, "! Unable to establish connection with DUT\n");
		status = FALSE;
	  }
	  else {
		LCPAndExpectState_t *state;
		LCPConfigOptionForm_t *opt;
		LCPForm_t *lcp;
		ubyte buffer[MAX_PACKET_LEN], mruData[16];
		byte4 len;
		ubyte2 mru;

		opt = LCPConfigOptionFormCreate();
		lcp = LCPFormCreate();
		state = LCPAndExpectStateCreate();

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

		FORM_SET_FIELD(opt, type, lcpConfigTypeMaximumReceiveUnit);
		FORM_SET_FIELD(opt, length, (ubyte)i);
		mru = LCP_DEFAULT_MRU;
		CLEAR_DATA(mruData);
		len = Pack(mruData, "S", &mru);
		FORM_SET_DATA(opt, mruData, len);

		len = LCPConfigOptionBuild(pppConn, opt, buffer);
		FORM_SET_DATA(lcp, buffer, len);

		Log(LOGMEDIUM, "Sending LCP Configure-Requests with "
			"MRU Option with length %ld\n", i);
 
		if(i<4) {
		  state->ncpType = lcpTypeConfigureNak;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			LCPConfigOptionForm_t *lcpNak;
			
			lcpNak = LCPConfigOptionFormCreate();
			
			LCPConfigOptionToForm(state->ncpForm.data, lcpNak);
			if (lcpNak->length != 4){
			  Log(LOGMEDIUM, "! An LCP Configure-Nak with an "
				  "incorrect option length %u was received\n", lcpNak->length);
			  status = FALSE;
			}
			else{
			  Log(LOGMEDIUM, "An LCP Configure-Nak was correctly received\n");
			}
			
			Free(lcpNak);
		  }
		  else{
			/* log message done in LCPAndExpect */
			status = FALSE;
		  }
		}
		else{
		  /* make sure we don't receive anything,
			 except maybe a ConfigureRequest      */
		  state->ncpType = 0;
		  state->ignoreType = lcpTypeConfigureRequest;
		  state->expectedPkts = 0;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			/* we received something, so fail */
			status = FALSE;
		  }
		  else{
			Log(LOGMEDIUM, "DUT correctly discarded malformed packet\n");
		  }
		}

		Free(state);
		Free(lcp);
		Free(opt);
	  }
	  DUTSendConfigCommand((ubyte *) "stop$");
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	}
  }
  END_TEST;

  /* SECTION 6.2 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "13.1",
     TEST_DESCRIPTION
     "Validate format of Async-Control-Character-Map Option fields\n",
     TEST_REFERENCE
     "RFC 1662 s7.1 p14-15 Async-Control-Character-Map\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with ACCM Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate ACCM Option:\n"
     "        Type                       2\n"
     "        Length                     6\n"
     "        ACCM                       configured value\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	ubyte4 accm;
	NCPGrabOptionState_t *state;

	DUTSetPPPACCM(config, LCP_DIFFERENT_ACCM);
	DUTstart(5);/*see config IDs spec on epoc client side*/
	
	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureRequest;
	state->optType = lcpConfigTypeAsyncControlCharacterMap;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM, "! LCP Configure-Request did not contain ACCM option\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating ACCM Option Fields\n");
	  opt = LCPConfigOptionFormCreate();
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, opt);
	  
	  /* Type doesn't need checking, because that's how we found it */
	  if(opt->length != 6){
		Log(LOGMEDIUM, "! MRU Option has len = %u (should be 6)\n",
			opt->length);
		status = FALSE;
	  }
	  Unpack(opt->data, "L", &accm);
	  if(accm != LCP_DIFFERENT_ACCM){
		Log(LOGMEDIUM,
			"! ACCM Option has value = 0x%08lX (configured as 0x%08X)\n",
			accm, LCP_DIFFERENT_ACCM);
		status = FALSE;
	  }
	  Free(opt);	
	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);
	DUTSetPPPACCM(config, LCP_DEFAULT_ACCM);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "13.2",
     TEST_DESCRIPTION
     "Validate functionality of Async-Control-Character-Map Option\n",
     TEST_REFERENCE
     "RFC 1661 s7.1 p14-16 Async-Control-Character-Map\n",
     TEST_METHOD
     "- ANVL: For each successive control character:\n"
     "- ANVL: Open connection with ACCM for that character\n"
     "- ANVL: Send Echo-Request containing that character\n"
     "-  DUT: Send Echo-Reply\n"
     "- ANVL: Check that character was mapped\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	NCPEstConnState_t *lcpState;
	LCPAndExpectState_t *state;
	boolean lcpStatus, validPkt = FALSE;
	ubyte2 i;
	ubyte4 len;
	byte echoStr[MAX_STRING];
	ubyte4 accmMap;
	HandlerID_t hand;

	lcp = LCPFormCreate();
	lcpEcho = LCPEchoDiscardFormCreate();

	for (i = 0; i < NUM_OF_CHARS; i++){

          DUTstart(0);/*see config IDs spec on epoc client side*/
	  state = LCPAndExpectStateCreate();
	  lcpState = NCPEstConnStateCreate(LCP);

	  Log(LOGMEDIUM, "Establishing PPP connection with ACCM map \n"
		  "for control character 0x%02X\n", i);
	  hand = ANVLHandlerInstall(pppConn, LCP,
								lcpTypeEchoReplyPacked, LCP_TYPE_LEN,
								LCPGrabRawEchoHandler, state);

	  accmMap = 1<<i;
	  FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), accm, accmMap);

	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if(!lcpStatus){
		Log(LOGMEDIUM, "! PPP connection not established\n");
		status = FALSE;
	  }

	  SPrintf(echoStr, "ANVL-PPP-%s #", testNum);
	  len = StrLen(echoStr);
	  echoStr[StrLen(echoStr)] = (byte)i;
	  len++;

	  CLEAR_DATA(lcp);
	  CLEAR_DATA(lcpEcho);
	  FORM_SET_FIELD(lcp, identifier, (ubyte)i);
	  FORM_SET_DATA(lcpEcho, (ubyte *)echoStr, len);

	  Log(LOGMEDIUM, "Sending an Echo-Request with "
		  "control character 0x%02X in echo data\n", i);
	  LCPEchoAndExpect(pppConn, lcpEcho, lcp, state, testNum);

	  if (state->receivedPkts) {
		ubyte4 j;

		Log(LOGMEDIUM, "Validating that control character was mapped\n");

		for (j = 0; j < state->pkt.len; j++){
		  if (!MemCmp(&(state->pkt.data[j]), echoStr, (Size_t)(len - 3))){
			validPkt = TRUE;
			break;
		  }
		}
		
		if (!validPkt){
		  Log(LOGMEDIUM, "! LCP Echo-Reply contains invalid data\n");
		  status = FALSE;
		}
		else{
		  if ((state->pkt.data[j + (len - 1)] == HDLC_ESCAPE) &&
			  (state->pkt.data[j + len] == 0x20 + i)){
			Log(LOGMEDIUM, "LCP Echo-Reply escaped 0x%02X correctly\n", i);
		  }
		  else{
			Log(LOGMEDIUM, 
				"! LCP Echo-Reply did not escape 0x%02X correctly\n", i);
			status = FALSE;
		  }
		}
	  }
	  else{
		Log(LOGMEDIUM, "! No LCP Echo-Reply packet was received\n");
		status = FALSE;
	  }
	  NCPEstConnStateDestroy(lcpState);
	  Free(state);

	  HandlerRemove(pppConn, hand, LCP);
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}

	Free(lcp);
	Free(lcpEcho);
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "13.3",
     TEST_DESCRIPTION
	 "If an Option is received (ACCM) in a Configure-Request but\n"
	 "with an incorrect Length, a Configure-Nak SHOULD be transmitted\n"
	 "unless the Length goes beyond the end of the packet\n",
     TEST_REFERENCE
     "RFC 1661 s6 p40 LCP Configuration Options\n",
     TEST_METHOD
     "- ANVL: Send Configure-Requests with ACCM Option with incorrect length\n"
     "-  DUT: Should Configure-Nak\n"
     "- CASE: Length < 6, length > 6\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	LCPAndExpectState_t *state;
	LCPConfigOptionForm_t *opt;
	LCPForm_t *lcp;
	ubyte buffer[MAX_PACKET_LEN], accmData[16];
	byte4 len;
	ubyte4 accm, i;
        DUTstart(0);/*see config IDs spec on epoc client side*/
	
	for (i = 0; i < 8; i++) {/*test only change! Remove!!!*/
	  
	  if (i == 6){
		continue;
	  }

	  opt = LCPConfigOptionFormCreate();
	  lcp = LCPFormCreate();
	  state = LCPAndExpectStateCreate();
	  
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);
	  
	  FORM_SET_FIELD(opt, type, lcpConfigTypeAsyncControlCharacterMap);
	  FORM_SET_FIELD(opt, length, (ubyte)i);
	  accm = LCP_DEFAULT_ACCM;
	  CLEAR_DATA(accmData);
	  len = Pack(accmData, "L", &accm);
	  FORM_SET_DATA(opt, accmData, len);
	  
	  len = LCPConfigOptionBuild(pppConn, opt, buffer);
	  FORM_SET_DATA(lcp, buffer, len);
	  
	  Log(LOGMEDIUM, "Sending LCP Configure-Requests with "
		  "ACCM Option with length %ld\n", i);
	  
	  state->ncpType = lcpTypeConfigureNak;
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  if (pkt) {
		LCPConfigOptionForm_t *lcpNak;
		
		lcpNak = LCPConfigOptionFormCreate();
		
		LCPConfigOptionToForm(state->ncpForm.data, lcpNak);
		if (lcpNak->length != 6){
		  Log(LOGMEDIUM, "! An LCP Configure-Nak with an "
			  "incorrect option length %u was received\n", lcpNak->length);
		  status = FALSE;
		}
		else{
		  Log(LOGMEDIUM, "An LCP Configure-Nak was correctly received\n");
		}
		
		Free(lcpNak);
	  }
	  else{
		/* log message done in LCPAndExpect */
		status = FALSE;
	  }
	  Free(state);
	  Free(lcp);
	  Free(opt);
	}
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;

  /* SECTION 6.3 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "14.1",
     TEST_DESCRIPTION
     "Validate format of Authentication-Protocol Option fields\n",
     TEST_REFERENCE
     "RFC 1661 s6.2 p41 Authentication-Protocol\n",
     TEST_METHOD
	 "- ANVL: Cause DUT to open connection with Auth-Protocol Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate fields of Authentication-Protocol Option:\n"
     "        Type                       3\n"
     "        Length                     >=4\n"
     "        Authentication-Protocol    c023 (PAP) or c223 (CHAP)\n"
     "        Data                       zero (PAP) or more octets (CHAP)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	ubyte2 proto;
	ubyte algorithm;
	NCPGrabOptionState_t *state;

	DUTSetPPPPAP(config, TRUE);
	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureRequest;
	state->optType = lcpConfigTypeAuthProtocol;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM, "! LCP Configure-Request did not contain Auth option\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Auth Option Fields\n");
	  opt = LCPConfigOptionFormCreate();
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, opt);
	  
	  /* Type doesn't need checking, because that's how we found it */
	  if(opt->length < 4){
		Log(LOGMEDIUM, "! Auth Option has len = %u (should be >= 4)\n",
			opt->length);
		status = FALSE;
	  }
	  else{
		Unpack(opt->data, "S", &proto);
		if(proto == pppTypePAP){
		  if(opt->length != 4){
			Log(LOGMEDIUM, "! PAP Auth Option has len = %u (should be 4)\n",
				opt->length);
			status = FALSE;
		  }
		}
		else if(proto == pppTypeCHAP){
		  if(opt->length != 5){
			Log(LOGMEDIUM, "! CHAP Auth Option has len = %u (should be 5)\n",
				opt->length);
			status = FALSE;
		  }
		  Unpack(opt->data, "+2 B", &algorithm);
		  if(algorithm != chapAlgorithmMD5){
			Log(LOGMEDIUM, "! CHAP Auth Option has unknown algorithm %u\n",
				algorithm);
			status = FALSE;
		  }
		}
		else{
		  Log(LOGMEDIUM,
			  "! Auth Option does not contain valid protocol 0x%04X\n",
			  proto);
		  status = FALSE;
		}
	  }

	  Free(opt);	
	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPPAP(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
	Free(state);
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "14.2",
     TEST_DESCRIPTION
     "An implementation MUST NOT include multiple Authentication-\n"
     "Protocol Configuration Options in its Configure-Request packets.\n",
     TEST_REFERENCE
     "RFC 1661 s6.2 p41 Authentication-Protocol\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request requesting both\n"
	 "        PAP and CHAP Authentication\n"
	 "-  DUT: Do not send a Configure-Ack for the Configure-Request\n"
	 "- ANVL: Reset PPP Connection\n"
	 "-  DUT: Send Configure-Request\n"
     "- ANVL: Send Configure-Nak with both PAP and CHAP Authentication\n"
     "-  DUT: Send another Configure-Request\n"
	 "- ANVL: Verify that the Configure-Request does not contain both\n"
	 "        PAP and CHAP Authentication\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPForm_t *lcp;
	LCPConfigDataForm_t *lcpData;
	LCPAndExpectState_t *state;
	Packet_t *pkt;
	ubyte buffer[MAX_PACKET_LEN];
	byte4 len;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcp = LCPFormCreate();
	lcpData = LCPConfigDataFormCreate();
	state = LCPAndExpectStateCreate();
	  
	FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);
	  
	FORM_SET_FIELD(lcpData, authProtocol, pppTypeCHAP);
	FORM_SET_FIELD(lcpData, authCHAPAlgorithm, chapAlgorithmMD5);
	len = LCPConfigDataBuildByType(pppConn, lcpConfigTypeAuthProtocol,
								   lcpData, buffer);
	CLEAR_DATA(lcpData);
	FORM_SET_FIELD(lcpData, authProtocol, pppTypePAP);
	FORM_SET_FIELD(lcpData, authPAP, TRUE);
	len += LCPConfigDataBuildByType(pppConn, lcpConfigTypeAuthProtocol,
									lcpData, buffer + len);
	
	FORM_SET_DATA(lcp, buffer, len);
	
	Log(LOGMEDIUM, "Sending Configure-Request with both PAP and CHAP\n");
	
	state->ncpType = lcpTypeConfigureAck;
	state->expectedPkts = 0;

	pkt = LCPAndExpect(pppConn, lcp, state);

	if (pkt != 0) {
	  status = FALSE;
	  Log(LOGMEDIUM, 
		  "! Received a Configure-Ack for packet requesting "
		  "both PAP and CHAP Authentication\n");
	  status = FALSE;
	}

	Free(state);

	Log(LOGMEDIUM, "\nReset connection\n");
	DUTPPPResetConn(config, pppConn);

	state = LCPAndExpectStateCreate();
	CLEAR_DATA(lcp);
	CLEAR_DATA(lcpData);

	/* set up DUT so it will Configure-Request with an Auth Protocol */
	DUTSetPPPPAP(config, TRUE);
	
	DUTLCPOpen(config, pppConn);

	state->ncpType = lcpTypeConfigureRequest;
	pkt = LCPAndExpect(pppConn, 0, state);
	if(!pkt){
	  /* LCPAndExpect will Log error */
	  status = FALSE;
	}
	else{
	  FORM_SET_FIELD(lcp, identifier, state->ncpForm.identifier);
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureNak);
	  
	  FORM_SET_FIELD(lcpData, authProtocol, pppTypeCHAP);
	  FORM_SET_FIELD(lcpData, authCHAPAlgorithm, chapAlgorithmMD5);
	  len = LCPConfigDataBuildByType(pppConn, lcpConfigTypeAuthProtocol,
									  lcpData, buffer);
	  CLEAR_DATA(lcpData);
	  FORM_SET_FIELD(lcpData, authProtocol, pppTypePAP);
	  FORM_SET_FIELD(lcpData, authPAP, TRUE);
	  len += LCPConfigDataBuildByType(pppConn, lcpConfigTypeAuthProtocol,
									  lcpData, buffer + len);

	  FORM_SET_DATA(lcp, buffer, len);
	  
	  Log(LOGMEDIUM, "Sending Configure-Nak with both PAP and CHAP\n");

	  state->ncpType = lcpTypeConfigureRequest;
	  pkt = LCPAndExpect(pppConn, lcp, state);
	  if (pkt) {
		LCPConfigOptionForm_t *lcpOption;
		ubyte *nextOption = pkt->data;
		boolean authProtocolFound = FALSE;

		lcpOption = LCPConfigOptionFormCreate();

		/* can't use ConfigDataToForm because if both PAP and CHAP are in the 
		   Ack, the second one will overwrite the first */

		while (nextOption < pkt->data + pkt->len){
		  nextOption = LCPConfigOptionToForms(nextOption, lcpOption, lcpData);
		  
		  if (lcpOption->type == lcpConfigTypeAuthProtocol){
			/* if we already found one, barf */
			Log(LOGMEDIUM, "Configure-Request contained Auth Protocol 0x%X\n",
				lcpData->authProtocol);

			if (authProtocolFound){
			  Log(LOGMEDIUM, 
				  "! Configure-Request contained two Auth Protocol options\n");
			  status = FALSE;
			}
			authProtocolFound = TRUE;
		  }
		}

		if (!authProtocolFound){
		  Log(LOGMEDIUM, "! Configure-Request did not contain "
			  "any Auth-Protocol option\n");
		  status = FALSE;
		}

		Free(lcpOption);
	  }
	  else{
		Log(LOGMEDIUM, 
			"! Did not receive Configure-Request after Configure-Nak\n");
		status = FALSE;
	  }
	}
	Free(lcp);
	Free(lcpData);
	Free(state);

	DUTPPPResetConn(config, pppConn);
	DUTSetPPPPAP(config, FALSE);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  

  BEGIN_TEST(
     TEST_NUM
     "14.5",
     TEST_DESCRIPTION
	 "If an Option is received (Auth Protocol) in a Configure-Request but\n"
	 "with an incorrect Length, a Configure-Nak SHOULD be transmitted\n"
	 "unless the Length goes beyond the end of the packet\n",
     TEST_REFERENCE
     "RFC 1661 s6 p40 LCP Configuration Options\n",
     TEST_METHOD
     "- ANVL: Send Configure-Requests with Authentication Protocol\n"
	 "        Option with incorrect length\n"
     "-  DUT: Should Configure-Nak\n"
     "- CASE: Length < 4\n"
	 "-  DUT: Should siletly discard packet\n"
     "- CASE: Length > 4\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	ubyte4 i;

	for (i = 0; i < 6; i++) {
	  
	  if (i == 4){
		continue;
	  }
      
      DUTstart(0);/*see config IDs spec on epoc client side*/

	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  Log(LOGMEDIUM, "\nEstablishing PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if (!lcpStatus) {
		Log(LOGMEDIUM, "! Unable to establish connection with DUT\n");
		status = FALSE;
	  }
	  else {
		LCPAndExpectState_t *state;
		LCPConfigOptionForm_t *opt;
		LCPForm_t *lcp;
		ubyte buffer[MAX_PACKET_LEN], authData[16];
		byte4 len;
		ubyte2 auth;

		opt = LCPConfigOptionFormCreate();
		lcp = LCPFormCreate();
		state = LCPAndExpectStateCreate();

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

		FORM_SET_FIELD(opt, type, lcpConfigTypeAuthProtocol);
		FORM_SET_FIELD(opt, length, (ubyte)i);
		auth = pppTypePAP;
		CLEAR_DATA(authData);
		len = Pack(authData, "S", &auth);
		FORM_SET_DATA(opt, authData, len);

		len = LCPConfigOptionBuild(pppConn, opt, buffer);
		FORM_SET_DATA(lcp, buffer, len);

		Log(LOGMEDIUM, "Sending LCP Configure-Requests with "
			"Auth-Protocol Option with length %ld\n", i);

		if(i<4) {
		  state->ncpType = lcpTypeConfigureNak;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			LCPConfigOptionForm_t *lcpNak;
			
			lcpNak = LCPConfigOptionFormCreate();
			
			LCPConfigOptionToForm(state->ncpForm.data, lcpNak);
			if (lcpNak->length != 4){
			  Log(LOGMEDIUM, "! An LCP Configure-Nak with an "
				  "incorrect option length %u was received\n", lcpNak->length);
			status = FALSE;
			}
			else{
			  Log(LOGMEDIUM, "An LCP Configure-Nak was correctly received\n");
			}
			
			Free(lcpNak);
		  }
		  else{
			/* log message done in LCPAndExpect */
			status = FALSE;
		  }
		}
		else{
		  /* make sure we don't receive anything,
			 except maybe a ConfigureRequest      */
		  state->ncpType = 0;
		  state->ignoreType = lcpTypeConfigureRequest;
		  state->expectedPkts = 0;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			/* we received something, so fail */
			status = FALSE;
		  }
		  else{
			Log(LOGMEDIUM, "DUT correctly discarded malformed packet\n");
		  }
		}

		Free(state);
		Free(lcp);
		Free(opt);
	  }
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);

	   DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;

  /* SECTION 6.4 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "15.1",
     TEST_DESCRIPTION
     "Validate format of Quality-Protocol Option fields\n",
     TEST_REFERENCE
     "RFC 1661 s6.3 p42-43 Quality-Protocol\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with LQM Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate fields of Quality-Protocol Option\n"
     "        Type                       4\n"
     "        Length                     >=4\n"
     "        Quality-Protocol           c025\n"
     "        Data                       zero or more octets\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	ubyte2 lqr;
	NCPGrabOptionState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	DUTSetPPPLQM(config, TRUE);
	
	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureRequest;
	state->optType = lcpConfigTypeQualityProtocol;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! LCP Configure-Request did not contain Quality option\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Quality Option Fields\n");
	  opt = LCPConfigOptionFormCreate();
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, opt);
	  
	  /* Type doesn't need checking, because that's how we found it */
	  if(opt->length < 4){
		Log(LOGMEDIUM, "! Quality Option has len = %u (should be >= 4)\n",
			opt->length);
		status = FALSE;
	  }
	  else{
		Unpack(opt->data, "S", &lqr);
		if(lqr != pppTypeLQM){
		  Log(LOGMEDIUM,
			  "! Quality Option does not contain valid protocol 0x%04X\n",
			  lqr);
		  status = FALSE;
		}
	  }

	  Free(opt);	
	  PacketDestroy(state->cfgPkt);
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	Free(state);

	DUTSetPPPLQM(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;
    
  
  BEGIN_TEST(
     TEST_NUM
     "15.4",
     TEST_DESCRIPTION
	 "If an Option is received (Quality Protocol) in a Configure-Request but\n"
	 "with an incorrect Length, a Configure-Nak SHOULD be transmitted\n",
     TEST_REFERENCE
     "RFC 1661 s6 p40 LCP Configuration Options\n",
     TEST_METHOD
     "- ANVL: Send Configure-Requests with Quality Protocol Option with\n"
	 "        incorrect length\n"
     "-  DUT: Should Configure-Nak\n"
     "- CASE: Length < 4\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	ubyte4 i;

	for (i = 0; i < 4; i++) {
 
		DUTstart(0);/*see config IDs spec on epoc client side*/

	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  Log(LOGMEDIUM, "\nEstablish PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if (!lcpStatus) {
		Log(LOGMEDIUM, "! Unable to establish connection with DUT\n");
		status = FALSE;
	  }
	  else {
		LCPAndExpectState_t *state;
		LCPConfigOptionForm_t *opt;
		LCPForm_t *lcp;
		ubyte buffer[MAX_PACKET_LEN], qualData[16];
		byte4 len;
		ubyte2 qual;

		opt = LCPConfigOptionFormCreate();
		lcp = LCPFormCreate();
		state = LCPAndExpectStateCreate();

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

		FORM_SET_FIELD(opt, type, lcpConfigTypeQualityProtocol);
		FORM_SET_FIELD(opt, length, (ubyte)i);
		qual = pppTypeLQM;
		CLEAR_DATA(qualData);
		len = Pack(qualData, "S", &qual);
		FORM_SET_DATA(opt, qualData, len);

		len = LCPConfigOptionBuild(pppConn, opt, buffer);
		FORM_SET_DATA(lcp, buffer, len);

		Log(LOGMEDIUM, "Sending LCP Configure-Requests with "
			"Quality-Protocol Option with length %ld\n", i);
 
		state->ncpType = lcpTypeConfigureNak;
		pkt = LCPAndExpect(pppConn, lcp, state);
		if (pkt) {
		  LCPConfigOptionForm_t *lcpNak;

		  lcpNak = LCPConfigOptionFormCreate();

		  LCPConfigOptionToForm(state->ncpForm.data, lcpNak);
		  if ((lcpNak->length == 4) || (lcpNak->length == 8)){
			Log(LOGMEDIUM, 
				"An LCP Configure-Nak (length %u) was correctly received\n",
				lcpNak->length);
		  }
		  else{
			Log(LOGMEDIUM, "! An LCP Configure-Nak with an "
				"incorrect option length %u was received\n", lcpNak->length);
			status = FALSE;
		  }

		  Free(lcpNak);
		}
		else{
		  /* log message done in LCPAndExpect */
		  status = FALSE;
		}

		Free(state);
		Free(lcp);
		Free(opt);
	  }
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;

  /* SECTION 6.5 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "16.1",
     TEST_DESCRIPTION
     "Validate format of Magic-Number Option fields\n",
     TEST_REFERENCE
     "RFC 1661 s6.4 p44-46 Magic-Number\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Magic-Number Option\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate fields of Magic-Number Option\n"
     "        Type                       5\n"
     "        Length                     6\n"
     "        Magic-Number               not 0\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	ubyte4 magic;
	NCPGrabOptionState_t *state;
	
	DUTstart(0);/*see config IDs spec on epoc client side*/

	DUTSetPPPMagicNumber(config, TRUE);
	
	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureRequest;
	state->optType = lcpConfigTypeMagicNumber;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! LCP Configure-Request did not contain Magic Number option\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Magic Number Option Fields\n");
	  opt = LCPConfigOptionFormCreate();
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, opt);
	  
	  /* Type doesn't need checking, because that's how we found it */
	  if(opt->length != 6){
		Log(LOGMEDIUM,
			"! Magic Number Option has len = %u (should be 6)\n",
			opt->length);
		status = FALSE;
	  }
	  Unpack(opt->data, "L", &magic);
	  if(magic == 0){
		Log(LOGMEDIUM, "! Magic Number cannot be 0\n");
		status = FALSE;
	  }
	  Free(opt);	
	  PacketDestroy(state->cfgPkt);
	}

	Free(state);
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPMagicNumber(config, FALSE);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "16.2",
     TEST_DESCRIPTION
     "A Magic-Number of zero is illegal and MUST always be Nak'd, if it\n"
     "is not Rejected outright.\n",
     TEST_REFERENCE
     "RFC 1661 s6.4 p46 Magic-Number\n",
     TEST_METHOD
     "- ANVL: Send Configure-Request with Magic-Number set to zero\n"
     "-  DUT: Send Configure-Nak or Configure-Reject\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;

	Log(LOGMEDIUM, 
		"Attempt to establish a PPP Connection with a Magic Number = 0\n");

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum, 0);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else{
	  if (lcpState->nakReceived){
		Log(LOGMEDIUM, "DUT correctly sent LCP Configure-Nak\n");
	  }
	  else if (lcpState->rejReceived){
		Log(LOGMEDIUM, "DUT correctly sent LCP Configure-Reject\n");
	  }
	  else{
		Log(LOGMEDIUM, "! Bad Magic Number was not Nak'd or Rejected\n");
		status = FALSE;
	  }
	}

	DUTPPPResetConn(config, pppConn);

	NCPEstConnStateDestroy(lcpState);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "16.3",
     TEST_DESCRIPTION
     "If Magic-Number Option in a received Configure-Request is different\n"
     "than the last Magic-Number Option sent, it SHOULD be acknowleged\n",
     TEST_REFERENCE
     "RFC 1661 s6.4 p44 Magic-number\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Magic-Number Option\n"
     "-  DUT: Send Configure-Request with Magic-Number Option\n"
     "- ANVL: Send Configure-Request with different Magic-Number\n"
     "-  DUT: Send Configure-Ack with ANVL's Magic-Number\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	LCPConfigDataForm_t *optData;
	ubyte4 magicNum;
	NCPGrabOptionState_t *state;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	DUTSetPPPMagicNumber(config, TRUE);
	
	Log(LOGMEDIUM, "Establish a PPP connection\n");	

	lcpState = NCPEstConnStateCreate(LCP);
	/* +++badger change the LCPEstConn to ensure 2 different magic nums */
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum,
				   LCP_DIFFERENT_MAGIC_NUM);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureAck;
	state->optType = lcpConfigTypeMagicNumber;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  if (lcpState->rejReceived || lcpState->nakReceived){
		Log(LOGMEDIUM, "! PPP Connection with Magic Number was Rejected\n");
	  }
	  else{
		Log(LOGMEDIUM, 
			"! LCP Configure-Ack did not contain Magic Number option\n");
	  }
	  status = FALSE;
	}
	else{
	  opt = LCPConfigOptionFormCreate();
	  optData = LCPConfigDataFormCreate();
	  NCPConfigOptionToForms(LCP, state->cfgPkt->data + state->optOffset,
							 opt, (NCPConfigDataForm_t *)optData);

	  magicNum = optData->magicNum;

	  if (magicNum != LCP_DIFFERENT_MAGIC_NUM){
		Log(LOGMEDIUM,
			"! LCP Configure-Ack Magic Num 0x%08lX was not equal to "
			"requested value 0x%08X\n",
			magicNum, LCP_DIFFERENT_MAGIC_NUM);
		status = FALSE;
	  }
	  else{
		Log(LOGMEDIUM, "LCP Configure-Ack Magic Number equals the "
			"requested Magic Number\n");
	  }

	  PacketDestroy(state->cfgPkt);
	  Free(opt);
	  Free(optData);
	}

	Free(state);
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPMagicNumber(config, FALSE);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "16.4",
     TEST_DESCRIPTION
     "If a received Configure-Request has the same Magic-Number Option\n"
     "as the last Configure-Request sent, MUST send Configure-Nak\n",
     TEST_REFERENCE
     "RFC 1661 s6.4 p44 Magic-Number\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Magic-Number\n"
     "-  DUT: Send Configure-Request with Magic-Number\n"
	 "- ANVL: Send Configure-Ack\n"
     "- ANVL: Send back Configure-Request with the same Magic-Number\n"
     "-  DUT: Send Configure-Nak with different Magic-Number\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState1, *lcpState2;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	LCPConfigDataForm_t *optData;
	ubyte4 magicNum1, magicNum2;
	NCPGrabOptionState_t *state1, *state2;

	DUTstart(0);/*see config IDs spec on epoc client side*/
	DUTSetPPPMagicNumber(config, TRUE);
	
	Log(LOGMEDIUM, "Establish a PPP connection\n");
	
	lcpState1 = NCPEstConnStateCreate(LCP);
	lcpState1->UserHandler = NCPGrabOptionHandler;
	/* We do not want to send an Ack because we want to grab their
	   Config-Request and stop this dialog before we Ack each other
	   and finish the connection */
	lcpState1->sendAckAfterNReqs = 10;
	state1 = NCPGrabOptionStateCreate();
	state1->cfgCode = lcpTypeConfigureRequest;
	state1->optType = lcpConfigTypeMagicNumber;
	/* Stop so we can go make up a new Configure-Request */
	state1->stopWhenGrabbed = TRUE;
	lcpState1->userData = (void *)state1;

	/* Unfortunately, this spits out an erroneous LCP Connection
       Denied error message */
	lcpStatus = LCPEstConn(pppConn, lcpState1);

	if(state1->cfgPkt == 0){
	  Log(LOGMEDIUM, 
		  "! LCP Configure-Request did not contain Magic Number option\n");
	}
	else{
	  /* Now we are going to make a new Configure-Request with a
         Magic-Number option and continue the dialog we started above */
	  opt = LCPConfigOptionFormCreate();
	  optData = LCPConfigDataFormCreate();
	  NCPConfigOptionToForms(LCP, state1->cfgPkt->data + state1->optOffset,
							 opt, (NCPConfigDataForm_t *)optData);

	  magicNum1 = optData->magicNum;

	  /* They should be sending us a Nak with a new Magic Number */
	  lcpState2 = NCPEstConnStateCreate(LCP);
	  FORM_SET_FIELD(LCP_CFG(lcpState2->localOpts), magicNum, magicNum1);
	  lcpState2->UserHandler = NCPGrabOptionHandler;
	  state2 = NCPGrabOptionStateCreate();
	  state2->cfgCode = lcpTypeConfigureNak;
	  state2->optType = lcpConfigTypeMagicNumber;
	  lcpState2->userData = (void *)state2;

	  lcpStatus = LCPEstConn(pppConn, lcpState2);
	  if(!lcpStatus){
		Log(LOGMEDIUM, 
			"! PPP Connection with different Magic Number Not Established\n");
		status = FALSE;
	  }
	  else if(state2->cfgPkt == 0){
		if (lcpState2->nakReceived){
		  Log(LOGMEDIUM, 
			  "! LCP Configure-Nak did not contain Magic Number option\n");
		}
		else{
		  Log(LOGMEDIUM, "! No LCP Configure-Nak received\n");
		}
		status = FALSE;
	  }
	  else{
		CLEAR_DATA(opt);
		CLEAR_DATA(optData);
		NCPConfigOptionToForms(LCP, state2->cfgPkt->data + state2->optOffset,
							   opt, (NCPConfigDataForm_t *)optData);

		magicNum2 = optData->magicNum;

		if (magicNum1 == magicNum2){
		  Log(LOGMEDIUM, 
			  "! LCP Configure-Nak contained the same "
			  "Magic Number originally requested\n");
		  status = FALSE;
		}
		else{
		  Log(LOGMEDIUM,
			  "LCP Configure-Nak had different Magic-Number than "
			  "Configure-Request\n");
		}
		PacketDestroy(state2->cfgPkt);
	  }

	  PacketDestroy(state1->cfgPkt);
	  Free(state2);
	  Free(optData);
	  Free(opt);
	  NCPEstConnStateDestroy(lcpState2);
	}

	Free(state1);
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState1);
	DUTSetPPPMagicNumber(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  BEGIN_TEST(
     TEST_NUM
     "16.5",
     TEST_DESCRIPTION
     "A new Configure-Request SHOULD NOT be sent to the peer until\n"
     "normal processing would cause it to be sent\n",
     TEST_REFERENCE
     "RFC 1661 s6.4 p44 Magic-Number\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Magic-Number\n"
     "-  DUT: Send Configure-Request with Magic-Number\n"
     "- ANVL: Send Configure-Request with the same Magic-Number\n"
     "-  DUT: Wait for Restart Timer before sending new Configure-Request\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPAndExpectState_t *state;
	LCPConfigOptionForm_t *opt;
	NCPConfigDataForm_t *data;
	LCPForm_t *lcp;
	Packet_t *pkt;
	ubyte4 magicNum;

	DUTstart(0);

	DUTSetPPPMagicNumber(config, TRUE);
	DUTLCPOpen(config, pppConn);

	Log(LOGMEDIUM, "Establish a PPP connection\n");

	state = LCPAndExpectStateCreate();
	opt = LCPConfigOptionFormCreate();
	lcp = LCPFormCreate();
	data = NCPConfigDataFormCreate(LCP);

	FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);
	FORM_SET_FIELD(lcp, identifier, NCPGetNextIdentifier());
	state->ncpType = lcpTypeConfigureRequest;

	pkt = LCPAndExpect(pppConn, lcp, state);
	if (!pkt){
	  Log(LOGMEDIUM, "! No LCP Configure-Request received\n");
	  status = FALSE;
	}
	else{
	  struct TimingStats_s stats;
	  boolean lcpStatus;
	  NCPEstConnState_t *ncpState;
	  Time_t startTime;

	  /*
		This needs to be done so that the NCP machine will recognize
		an Ack sent by the DUT to our first request above.
	  */
	  NCPSetNextIdentifier(lcp->identifier);
	  ncpState = NCPEstConnStateCreate(LCP);

	  NCPConfigDataToForm(LCP, pkt->data, pkt->len, data);
	  magicNum = LCP_CFG(data)->magicNum;
	  
	  FORM_SET_FIELD(LCP_CFG(ncpState->localOpts), magicNum, magicNum);

	  ncpState->userData = &stats;
	  ncpState->UserHandler = (PacketHandler_t *)LCPTimingHandler;
	  stats.numSeen = 0;
	  stats.lcpType = lcpTypeConfigureRequest;

	  /* get the time now */
	  GetTime(&startTime);

	  lcpStatus = LCPEstConn(pppConn, ncpState);

	  if (!lcpStatus) {
		Log(LOGMEDIUM, "! Could not establish LCP Connection\n");
		status = FALSE;
	  }
	  else{
		Time_t totalTime;
		real4 floatTime;

		totalTime = TimeSubtract(&startTime, &(stats.times[0]));
		floatTime = totalTime.sec + (totalTime.usec/(real4)1000000.0);
		Log(LOGMEDIUM, "Restart Timer went off in %1.2f seconds\n", floatTime);

		if (floatTime > 
			(1 + RESTART_TIMER_PERCENT) * LCP_DEFAULT_RESTART_TIMER){
		  Log(LOGMEDIUM, "! Restart Timer took too long\n");
		  status = FALSE;
		}
		else if (floatTime <
				 (1 - RESTART_TIMER_PERCENT) * LCP_DEFAULT_RESTART_TIMER){
		  Log(LOGMEDIUM, "! Restart Timer went off too soon\n");
		  status = FALSE;
		}
		else{
		  Log(LOGMEDIUM, "Restart Timer went off correctly\n");
		}
	  }
	  NCPEstConnStateDestroy(ncpState);
	}
	DUTPPPResetConn(config, pppConn);

	Free(state);
	Free(opt);
	Free(lcp);
	Free(data);
	DUTSetPPPMagicNumber(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
	
  }
  END_TEST;
  

  BEGIN_TEST(
     TEST_NUM
     "16.7",
     TEST_DESCRIPTION
	 "If Magic-Number requested, MUST NOT reject request for Magic-Number\n",
     TEST_REFERENCE
     "RFC 1661 s6.4 p45 Magic-Number\n",
     TEST_METHOD
	 "- ANVL: Cause DUT to open connection with Magic-Number\n"
     "-  DUT: Send Configure-Request with Magic-Number\n"
     "- ANVL: Send Configure-Request with different Magic-Number\n"
     "-  DUT: Do not sent Configure-Reject for Magic-Number\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	DUTSetPPPMagicNumber(config, TRUE);
	
	Log(LOGMEDIUM, "Establish a PPP connection\n");
	
	lcpState = NCPEstConnStateCreate(LCP);
	/* +++badger change the LCPEstConn to ensure 2 different magic nums */
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), magicNum,
				   LCP_DIFFERENT_MAGIC_NUM);

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if (lcpState->rejReceived || lcpState->nakReceived){
	  Log(LOGMEDIUM, "! PPP Connection with Magic Number was Rejected\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "No LCP Configure-Reject was received\n");
	}

	DUTPPPResetConn(config, pppConn);

	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPMagicNumber(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
	
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "16.8",
     TEST_DESCRIPTION
     "If Magic-Number Option is rejected, it SHOULD be treated as if\n"
     "a Configure-Ack had been received\n",
     TEST_REFERENCE
     "RFC 1661 s6.4 p45 Magic-Number\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Magic-Number Option\n"
     "-  DUT: Send Configure-Request with Magic-Number Option\n"
     "- ANVL: Send Configure-Reject rejecting the Magic-Number Option\n"
     "- ANVL: Send Configure-Request with no Options\n"
     "-  DUT: Send Configure-Ack\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;

	DUTstart(0);

	DUTSetPPPMagicNumber(config, TRUE);
	
	Log(LOGMEDIUM, "Establish a PPP connection\n");
	
	lcpState = NCPEstConnStateCreate(LCP);
	FORM_SET_FIELD(LCP_CFG(lcpState->rejOpts), magicNum, 1);

	LCPEstConn(pppConn, lcpState);
	lcpStatus = (lcpState->rejSent && lcpState->ackReceived);	
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if (lcpState->reqReceived > 1){
	  Log(LOGMEDIUM, "! DUT did not treat Configure-Reject as Ack\n");
	  status = FALSE;
	}
	else if (LCP_CFG(lcpState->remoteOptsAckd)->magicNumOK){
	  Log(LOGMEDIUM, 
		  "! A Magic Number was established in the PPP Connection\n"); 
	  status = FALSE;
	}

	DUTPPPResetConn(config, pppConn);

	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPMagicNumber(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
	
  }
  END_TEST;

    
  BEGIN_TEST(
     TEST_NUM
     "16.10",
     TEST_DESCRIPTION
	 "If an Option is received (Magic Number) in a Configure-Request but\n"
	 "with an incorrect Length, a Configure-Nak SHOULD be transmitted\n"
	 "unless the Length goes beyond the end of the packet\n",
     TEST_REFERENCE
     "RFC 1661 s6 p40 LCP Configuration Options\n",
     TEST_METHOD
     "- ANVL: Send Configure-Requests with Magic Number Option with\n"
	 "        incorrect length\n"
     "-  DUT: Should Configure-Nak\n"
     "- CASE: Length < 6\n"
	 "-  DUT: Should silently discard packet\n"
     "- CASE: Length > 6\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	ubyte4 i;

	for (i = 0; i < 8; i++) {
	  
	  if (i == 6){
		continue;
	  }
    
	  DUTstart(0);/*see config IDs spec on epoc client side*/

	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  Log(LOGMEDIUM, "\nEstablish PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if (!lcpStatus) {
		Log(LOGMEDIUM, "! Unable to establish connection with DUT\n");
		status = FALSE;
	  }
	  else {
		LCPAndExpectState_t *state;
		LCPConfigOptionForm_t *opt;
		LCPForm_t *lcp;
		ubyte buffer[MAX_PACKET_LEN], magicData[16];
		byte4 len;
		ubyte4 magic;

		opt = LCPConfigOptionFormCreate();
		lcp = LCPFormCreate();
		state = LCPAndExpectStateCreate();

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

		FORM_SET_FIELD(opt, type, lcpConfigTypeMagicNumber);
		FORM_SET_FIELD(opt, length, (ubyte)i);
		magic = 0x12345678;
		CLEAR_DATA(magicData);
		len = Pack(magicData, "L", &magic);
		FORM_SET_DATA(opt, magicData, len);

		len = LCPConfigOptionBuild(pppConn, opt, buffer);
		FORM_SET_DATA(lcp, buffer, len);

		Log(LOGMEDIUM, "Sending LCP Configure-Requests with "
			"Magic-Number Option with length %ld\n", i);
		
		if(i<6) {
		  state->ncpType = lcpTypeConfigureNak;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			LCPConfigOptionForm_t *lcpNak;
			
			lcpNak = LCPConfigOptionFormCreate();
			
			LCPConfigOptionToForm(state->ncpForm.data, lcpNak);
			if (lcpNak->length != 6){
			  Log(LOGMEDIUM, "! An LCP Configure-Nak with an "
				  "incorrect option length %u was received\n", lcpNak->length);
			  status = FALSE;
			}
			else{
			  Log(LOGMEDIUM, "An LCP Configure-Nak was correctly received\n");
			}
			
			Free(lcpNak);
		  }
		  else{
			/* log message done in LCPAndExpect */
			status = FALSE;
		  }
		}
		else{
		  /* make sure we don't receive anything,
			 except maybe a ConfigureRequest      */
		  state->ncpType = 0;
		  state->ignoreType = lcpTypeConfigureRequest;
		  state->expectedPkts = 0;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			/* we received something, so fail */
			status = FALSE;
		  }
		  else{
			Log(LOGMEDIUM, "DUT correctly discarded malformed packet\n");
		  }
		}

        	DUTSendConfigCommand((ubyte *) "stop$");
		Free(state);
		Free(lcp);
		Free(opt);
	  }
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	}
  }
  END_TEST;

  /* SECTION 6.6 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "17.1",
     TEST_DESCRIPTION
     "Validate format of Protocol-Field-Compression Option fields\n",
     TEST_REFERENCE
     "RFC 1661 s6.5 p47-48 Protocol-Field-Compression\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with Protocol-Field-Compression\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate fields of Protocol-Field-Compression Option:\n"
     "        Type                       7\n"
     "        Length                     2\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	NCPGrabOptionState_t *state;

	DUTstart(6);/*see config IDs spec on epoc client side*/

	DUTSetPPPProtocolCompression(config, TRUE);
	
	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureRequest;
	state->optType = lcpConfigTypeProtocolFieldCompression;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! LCP Configure-Request did not contain Proto Field Comp option\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating Protocol Field Compression Option Fields\n");
	  opt = LCPConfigOptionFormCreate();
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, opt);
	  
	  /* Type doesn't need checking, because that's how we found it */
	  if(opt->length != 2){
		Log(LOGMEDIUM,
			"! Protocol Field Comp Option has len = %u (should be 2)\n",
			opt->length);
		status = FALSE;
	  }
	  Free(opt);	
	  PacketDestroy(state->cfgPkt);
	}

	Free(state);
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPProtocolCompression(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;


/* +++alan: these should be written so they don't require LIB_IP */

  BEGIN_TEST(
     TEST_NUM
     "17.2",
     TEST_DESCRIPTION
     "When negotiated, PPP implementations MUST accept PPP packets with\n"
     "either double-octet or single-octet Protocol fields\n",
     TEST_REFERENCE
     "RFC 1661 s6.5 p47 Protocol-Field-Compression\n",
     TEST_METHOD
	 "- SETUP: Enable Protocol Field Compression on DUT\n"
     "- ANVL: Negotiate LCP with Protocol-Field-Compression\n"
     "- ANVL: Establish NCP with DUT\n"
     "- ANVL: Send protocol echo packet with Protocol field compressed\n"
     "-  DUT: Should respond to echo packet\n"
     "- ANVL: Verify that echo reply packet has Protocol field compressed\n"
     "- ANVL: Send protocol echo packet with Protocol field uncompressed\n"
     "-  DUT: Should respond to echo packet\n"
     "- ANVL: Verify that echo reply packet has Protocol field compressed\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	struct PPPCheckCompressionState_s protoCompState;
	boolean ncpStatus;
	ubyte4 i;

	DUTstart(6);/*see config IDs spec on epoc client side*/

	DUTSetPPPProtocolCompression(config, TRUE);
	
	for (i=1; i<=2; i++) {
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), protocolComp, TRUE);
	  if (!ANVLPPPEstConn(config, pppConn, lcpState)) {
		status = FALSE;
	  }
	  if (!(status &&
			((LCP_CFG(lcpState->localOptsAckd)->protocolCompOK) &&
			 (LCP_CFG(lcpState->remoteOptsAckd)->protocolCompOK)))) {
		Log(LOGMEDIUM, "! Unable to establish connection with PFC\n");
		status = FALSE;
	  }
	  else {
		PPPForm_t *ppp;
		Link_t *link;
		
		link = LinkCreate(LINK_PPP);
		ppp = PPPFormCreate();
		
		FORM_SET_FIELD(ppp, protocolCompressed, i==1? TRUE : FALSE);
		link->forms.ppp.pppForm = ppp;
		
		CLEAR_DATA(&protoCompState);
		
		ncpStatus =
			ANVLNCPEstConnAndEcho(config, link, IPCP, 
							  PPPCheckCompressionHandler,
							  &protoCompState);
		if (!ncpStatus) {
		  Log(LOGMEDIUM,
			  "! Could not establish NCP connection and receive echo reply\n");
		  status = FALSE;
		}
		else {
		  if (protoCompState.protocolCompressed) {
			Log(LOGMEDIUM, "Received protocol Echo Reply with Protocol "
				"field compressed\n");
		  }
		  else {
			Log(LOGMEDIUM, "! Received protocol Echo Reply with Protocol "
				"field uncompressed\n");
			status = FALSE;
		  }
		}
		
		Free(ppp);
		Free(link);
	  }
	  
	  DUTPPPResetConn(config, pppConn);
	  NCPEstConnStateDestroy(lcpState);
	}

	DUTSetPPPProtocolCompression(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "17.3",
     TEST_DESCRIPTION
     "Reject single-octet Protocol fields when compression not negotiated\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s6.5 p47 Protocol-Field-Compression\n",
     TEST_METHOD
	 "- SETUP: Disable Protocol Field Compression on DUT\n"
     "- ANVL: Negotiate LCP with no Protocol-Field-Compression\n"
     "- ANVL: Establish NCP with DUT\n"
     "- ANVL: Send protocol echo packet with Protocol field compressed\n"
     "-  DUT: Should not respond to echo packet\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean ncpStatus;

	DUTstart(0);

	lcpState = NCPEstConnStateCreate(LCP);
	
	FORM_SET_FIELD(LCP_CFG(lcpState->rejOpts), protocolComp, TRUE);
	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if (!status ||
		((LCP_CFG(lcpState->localOptsAckd)->protocolCompOK) &&
		 (LCP_CFG(lcpState->remoteOptsAckd)->protocolCompOK))) {
	  Log(LOGMEDIUM, "! Unable to establish connection with no PFC\n");
	  status = FALSE;
	}
	else {
	  PPPForm_t *ppp;
	  Link_t *link;
	  
	  link = LinkCreate(LINK_PPP);
	  ppp = PPPFormCreate();
	  
	  FORM_SET_FIELD(ppp, protocolCompressed, TRUE);
	  link->forms.ppp.pppForm = ppp;
	  
	  ncpStatus = ANVLNCPEstConnAndEcho(config, link, IPCP, 0, 0);
	  if (ncpStatus) {
		Log(LOGMEDIUM,
			"! DUT replied to echo with Protocol field compressed\n");
		status = FALSE;
	  }
	  
	  Free(ppp);
	  Free(link);
	}
	
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  
  BEGIN_TEST(
     TEST_NUM
     "17.5",
     TEST_DESCRIPTION
     "When a Protocol field is compressed, the Data Link Layer FCS field\n"
     "is calculated on the compressed frame, not the original frame.\n",
     TEST_REFERENCE
     "RFC 1661 s6.5 p47 Protocol-Field-Compression\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with Protocol field compression on\n"
     "- ANVL: Validate that FCS fields on packets sent from DUT are\n"
     "        calculated correctly\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	struct PPPCheckCompressionState_s protoCompState;
	boolean ncpStatus;

	DUTstart(6);/*see config IDs spec on epoc client side*/

	DUTSetPPPProtocolCompression(config, TRUE);
	
	lcpState = NCPEstConnStateCreate(LCP);
	
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), protocolComp, TRUE);
	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if (!(status &&
		  ((LCP_CFG(lcpState->localOptsAckd)->protocolCompOK) &&
		   (LCP_CFG(lcpState->remoteOptsAckd)->protocolCompOK)))) {
	  Log(LOGMEDIUM, "! Unable to establish connection with PFC\n");
	  status = FALSE;
	}
	else {
	  CLEAR_DATA(&protoCompState);
	  
	  ncpStatus =
		  ANVLNCPEstConnAndEcho(config, 0, IPCP, 
							PPPCheckCompressionHandler,
							&protoCompState);
	  if (!ncpStatus) {
		Log(LOGMEDIUM,
			"! Could not establish NCP connection and receive echo reply\n");
		status = FALSE;
	  }
	  else if (!protoCompState.protocolCompressed) {
		Log(LOGMEDIUM, "! Received protocol Echo Reply with Protocol "
			"field uncompressed\n");
		status = FALSE;
	  }
	  else if (!protoCompState.fcsCorrect) {
		Log(LOGMEDIUM, "! Received compressed protocol Echo Reply with "
			"incorrect FCS\n");
		status = FALSE;
	  }
	  else {
		Log(LOGMEDIUM, "Compressed protocol Echo Reply has correct FCS\n");
	  }
	}
	
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);

	DUTSetPPPProtocolCompression(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

    
  BEGIN_TEST(
     TEST_NUM
     "17.6",
     TEST_DESCRIPTION
	 "If an Option is received (PFC) in a Configure-Request but\n"
	 "with an incorrect Length, a Configure-Nak SHOULD be transmitted\n"
	 "unless the Length goes beyond the end of the packet\n",
     TEST_REFERENCE
     "RFC 1661 s6 p40 LCP Configuration Options\n",
     TEST_METHOD
     "- ANVL: Send Configure-Requests with Protocol Field\n"
	 "        Compression Option with incorrect length\n"
     "-  DUT: Should Configure-Nak\n"
     "- CASE: Length < 2\n"
	 "-  DUT: Should silently discard packet\n"
     "- CASE: Length > 2\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	ubyte4 i;

	for (i = 0; i < 5; i++) {

	  /* 2 is the right value, so skip it */
	  if (i == 2){
		continue;
	  }

	  DUTstart(0);/*see config IDs spec on epoc client side*/

	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  Log(LOGMEDIUM, "\nEstablish PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if (!lcpStatus) {
		Log(LOGMEDIUM, "! Unable to establish connection with DUT\n");
		status = FALSE;
	  }
	  else {
		LCPAndExpectState_t *state;
		LCPConfigOptionForm_t *opt;
		LCPForm_t *lcp;
		ubyte buffer[MAX_PACKET_LEN];
		byte4 len;

		opt = LCPConfigOptionFormCreate();
		lcp = LCPFormCreate();
		state = LCPAndExpectStateCreate();

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

		FORM_SET_FIELD(opt, type, lcpConfigTypeProtocolFieldCompression);
		FORM_SET_FIELD(opt, length, (ubyte)i);
		len = LCPConfigOptionBuild(pppConn, opt, buffer);
		FORM_SET_DATA(lcp, buffer, len);

		Log(LOGMEDIUM, "Sending LCP Configure-Requests with "
			"Protocol-Field-Compression with length %ld\n", i);

		if(i<2) {
		  state->ncpType = lcpTypeConfigureNak;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			LCPConfigOptionForm_t *lcpNak;
			
			lcpNak = LCPConfigOptionFormCreate();
			
			LCPConfigOptionToForm(state->ncpForm.data, lcpNak);
			if (lcpNak->length != 2){
			  Log(LOGMEDIUM, "! An LCP Configure-Nak with an "
				  "incorrect option length %u was received\n", lcpNak->length);
			  status = FALSE;
			}
			else{
			  Log(LOGMEDIUM, "An LCP Configure-Nak was correctly received\n");
			}
			
			Free(lcpNak);
		  }
		  else{
			/* log message done in LCPAndExpect */
			status = FALSE;
		  }
		}
		else{
		  /* make sure we don't receive anything,
			 except maybe a ConfigureRequest      */
		  state->ncpType = 0;
		  state->ignoreType = lcpTypeConfigureRequest;
		  state->expectedPkts = 0;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			/* we received something, so fail */
			status = FALSE;
		  }
		  else{
			Log(LOGMEDIUM, "DUT correctly discarded malformed packet\n");
		  }
		}
       

		Free(state);
		Free(lcp);
		Free(opt);
	  }
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
          DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;
    
  /* SECTION 6.7 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "18.1",
     TEST_DESCRIPTION
     "Validate format of Address-and-Control-Field-Compression Option\n"
     "fields\n",
     TEST_REFERENCE
     "RFC 1661 s6.6 p49 Address-and-Control-Field-Compression\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection with ACFC Option Fields\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Validate fields of ACFC Option\n"
     "        Type                       8\n"
     "        Length                     2\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	LCPConfigOptionForm_t *opt;
	NCPGrabOptionState_t *state;

	DUTstart(7);/*see config IDs spec on epoc client side*/

	DUTSetPPPACFCompression(config, TRUE);
	
	lcpState = NCPEstConnStateCreate(LCP);
	lcpState->UserHandler = NCPGrabOptionHandler;
	state = NCPGrabOptionStateCreate();
	state->cfgCode = lcpTypeConfigureRequest;
	state->optType = lcpConfigTypeACFC;
	lcpState->userData = (void *)state;

	lcpStatus = LCPEstConn(pppConn, lcpState);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else if(state->cfgPkt == 0){
	  Log(LOGMEDIUM,
		  "! LCP Configure-Request did not contain ACFC option\n");
	  status = FALSE;
	}
	else{
	  Log(LOGMEDIUM, "Validating ACFC Option Fields\n");
	  opt = LCPConfigOptionFormCreate();
	  LCPConfigOptionToForm(state->cfgPkt->data + state->optOffset, opt);
	  
	  /* Type doesn't need checking, because that's how we found it */
	  if(opt->length != 2){
		Log(LOGMEDIUM,
			"! ACFC Option has len = %u (should be 2)\n",
			opt->length);
		status = FALSE;
	  }
	  Free(opt);	
	  PacketDestroy(state->cfgPkt);
	}

	Free(state);
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);
	DUTSetPPPACFCompression(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;


/* +++alan: these should be written so they don't require LIB_IP */

  BEGIN_TEST(
     TEST_NUM
     "18.2",
     TEST_DESCRIPTION
     "Validate correct functioning of Address and Control field\n"
     "compression\n",
     TEST_REFERENCE
     "RFC 1661 s6.6 p49 Address-and-Control-Field-Compression\n",
     TEST_METHOD
	 "- SETUP: Enable Address and Control Field Compression on DUT\n"
     "- ANVL: Negotiate LCP with Address-and-Control-Field-Compression\n"
     "- ANVL: Establish NCP with DUT\n"
     "-  DUT: Should send NCP packets with Address and Control fields\n"
     "        compressed\n"
     "- ANVL: Send protocol echo packet with Address and Control fields\n"
     "        compressed\n"
     "-  DUT: Should respond to echo packet with Address and Control fields\n"
     "        compressed\n"
     "- ANVL: Verify that echo reply packet has Address and Control fields\n"
     "        compressed\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	struct PPPCheckCompressionState_s ncpCompState, protoCompState;

	DUTstart(7);/*see config IDs spec on epoc client side*/

	DUTSetPPPACFCompression(config, TRUE);

	lcpState = NCPEstConnStateCreate(LCP);
	
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), addrControlComp, TRUE);
	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if (!(status &&
		((LCP_CFG(lcpState->localOptsAckd)->addrControlCompOK) &&
		 (LCP_CFG(lcpState->remoteOptsAckd)->addrControlCompOK)))) {
	  Log(LOGMEDIUM, "! Unable to establish connection with ACFC\n");
	  status = FALSE;
	}
	else {
	  HDLCForm_t *hdlc;
	  HandlerID_t hand;
	  Link_t *link;

	  link = LinkCreate(LINK_PPP);
	  hdlc = HDLCFormCreate();
	  FORM_SET_FIELD(hdlc, addressControlCompressed, TRUE);
	  link->forms.ppp.hdlcForm = hdlc;

	  CLEAR_DATA(&ncpCompState);
	  CLEAR_DATA(&protoCompState);

	  hand = ANVLHandlerInstall(pppConn, IPCP, 
								ipcpTypeConfigureRequestPacked, IPCP_TYPE_LEN, 
								PPPCheckCompressionHandler,
								&ncpCompState);
	  status = ANVLNCPEstConnAndEcho(config, link, IPCP, 
								 PPPCheckCompressionHandler,
								 &protoCompState);
	  HandlerRemove(pppConn, hand, IPCP);
	  if (!status) {
		Log(LOGMEDIUM,
			"! Could not establish NCP connection and receive echo reply\n");
	  }
	  else {
		if (ncpCompState.addressControlCompressed) {
		  Log(LOGMEDIUM, "Received NCP Configure-Request with Address and "
			  "Control fields compressed\n");
		}
		else {
		  Log(LOGMEDIUM, "! Received NCP Configure-Request with Address and "
			  "Control fields uncompressed\n");
		  status = FALSE;
		}
		
		if (protoCompState.addressControlCompressed) {
		  Log(LOGMEDIUM, "Received protocol Echo Reply with Address and "
			  "Control fields compressed\n");
		}
		else {
		  Log(LOGMEDIUM, "! Received protocol Echo Reply with Address and "
			  "Control fields uncompressed\n");
		  status = FALSE;
		}
	  }

	  Free(hdlc);
	  Free(link);
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);

	DUTSetPPPACFCompression(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

    
  BEGIN_TEST(
     TEST_NUM
     "18.3",
     TEST_DESCRIPTION
     "The Address and Control fields MUST NOT be compressed when sending\n"
     "any LCP packet; this rule guarantees unambiguous recognition of LCP's\n",
     TEST_REFERENCE
     "RFC 1661 s6.6 p49 Address-and-Control-Field-Compression\n",
     TEST_METHOD
     "- ANVL: Set up connection with Address and Control field compression\n"
     "- ANVL: Send Echo-Request packet to DUT with uncompressed fields\n"
     "-  DUT: Send Echo-Reply\n"
     "- ANVL: Verify that the Address and Control fields of the Echo-Reply\n"
     "        are not compressed\n"
     "- ANVL: Send Echo-Request packet to DUT with compressed fields\n"
     "-  DUT: Send Echo-Reply\n"
     "- ANVL: Verify that the Address and Control fields of the Echo-Reply\n"
     "        are not compressed\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	struct PPPCheckCompressionState_s state;
	HandlerID_t hand;
	LCPForm_t *lcp;
	LCPEchoDiscardForm_t *lcpEcho;
	HDLCForm_t *hdlc;
	ubyte buffer[MAX_PACKET_LEN];
	ubyte4 len;

	DUTstart(7);/*see config IDs spec on epoc client side*/

	DUTSetPPPACFCompression(config, TRUE);

	lcpState = NCPEstConnStateCreate(LCP);
	
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), addrControlComp, TRUE);
	status = LCPEstConn(pppConn, lcpState);
	if (!(status &&
		((LCP_CFG(lcpState->localOptsAckd)->addrControlCompOK) &&
		 (LCP_CFG(lcpState->remoteOptsAckd)->addrControlCompOK)))) {
	  Log(LOGMEDIUM, "! Unable to establish connection with ACFC\n");
	  status = FALSE;
	}
	else {
	  lcp = LCPFormCreate();
	  lcpEcho = LCPEchoDiscardFormCreate();
	  hdlc = HDLCFormCreate();

	  CLEAR_DATA(&state);

	  /* install PPPCheckCompressionHandler */
	  hand = ANVLHandlerInstall(pppConn, LCP, 
								lcpTypeEchoReplyPacked, LCP_TYPE_LEN,
								PPPCheckCompressionHandler, &state);
	  
	  Log(LOGMEDIUM, "Sending LCP Echo-Request with uncompressed fields\n");
	  FORM_SET_FIELD(hdlc, addressControlCompressed, FALSE);

	  FORM_SET_FIELD(lcp, code, lcpTypeEchoRequest);
	  len = LCPEchoDiscardBuild(pppConn, lcpEcho, buffer);
	  FORM_SET_DATA(lcp, buffer, len);

	  LCPSend(pppConn, lcp, 0, hdlc);

	  RunHandlers(&requestTimeOut);

	  /* check state */
	  if (!state.receivedPkt) {
		Log(LOGMEDIUM, "! Did not receive reply to Echo-Request\n");
		status = FALSE;
	  }
	  else if (state.addressControlCompressed) {
		Log(LOGMEDIUM, "! Received Echo-Reply with Address and Control "
			"fields compressed\n");
		status = FALSE;
	  }
	  else {
		Log(LOGMEDIUM, "Received Echo-Reply with Address and Control "
			"fields uncompressed\n");
	  }

	  CLEAR_DATA(&state);
	  
	  Log(LOGMEDIUM, "Sending LCP Echo-Request with compressed fields\n");
	  CLEAR_DATA(hdlc);
	  FORM_SET_FIELD(hdlc, addressControlCompressed, TRUE);
	  LCPSend(pppConn, lcp, 0, hdlc);
	  
	  RunHandlers(&requestTimeOut);
	  
	  /* check state */
	  if (!state.receivedPkt) {
		Log(LOGMEDIUM, "Did not receive reply to Echo-Request\n");
	  }
	  else if (state.addressControlCompressed) {
		Log(LOGMEDIUM, "! Received Echo-Reply with Address and Control "
			"fields compressed\n");
		status = FALSE;
	  }
	  else {
		Log(LOGMEDIUM, "Received Echo-Reply with Address and Control "
			"fields uncompressed\n");
	 }

	  HandlerRemove(pppConn, hand, LCP);
	  Free(hdlc);
	  Free(lcp);
	  Free(lcpEcho);
	}

	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);

	DUTSetPPPACFCompression(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;


/* +++alan: these should be written so they don't require LIB_IP */

  BEGIN_TEST(
     TEST_NUM
     "18.4",
     TEST_DESCRIPTION
     "When Address and Control fields are compressed, the Data Link Layer\n"
     "Layer FCS field is calculated on compressed frame, not the original\n",
     TEST_REFERENCE
     "RFC 1661 s6.6 p49 Address-and-Control-Field-Compression\n",
     TEST_METHOD
     "- ANVL: Set up PPP connection with Address and Control field \n"
     "        compression turned on\n"
     "- ANVL: Validate that FCS fields on packets sent from DUT are\n"
     "        calculated correctly\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	NCPEstConnState_t *lcpState;
	struct PPPCheckCompressionState_s protoCompState;
	boolean ncpStatus;

	DUTSetPPPACFCompression(config, TRUE);
	
	DUTstart(7);/*see config IDs spec on epoc client side*/

	lcpState = NCPEstConnStateCreate(LCP);
	
	FORM_SET_FIELD(LCP_CFG(lcpState->localOpts), addrControlComp, TRUE);
	FORM_SET_FIELD(LCP_CFG(lcpState->nakOpts), addrControlComp, TRUE);
	status = ANVLPPPEstConn(config, pppConn, lcpState);
	if (!(status &&
		  ((LCP_CFG(lcpState->localOptsAckd)->addrControlCompOK) &&
		   (LCP_CFG(lcpState->remoteOptsAckd)->addrControlCompOK)))) {
	  Log(LOGMEDIUM, "! Unable to establish connection with ACFC\n");
	  status = FALSE;
	}
	else {
	  CLEAR_DATA(&protoCompState);
	  
	  ncpStatus =
		  ANVLNCPEstConnAndEcho(config, 0, IPCP, 
								PPPCheckCompressionHandler,
								&protoCompState);
	  if (!ncpStatus) {
		Log(LOGMEDIUM,
			"! Could not establish NCP connection and receive echo reply\n");
		status = FALSE;
	  }
	  else if (!protoCompState.addressControlCompressed) {
		Log(LOGMEDIUM, "! Received protocol Echo Reply with Address and "
			"Control fields uncompressed\n");
		status = FALSE;
	  }
	  else if (!protoCompState.fcsCorrect) {
		Log(LOGMEDIUM, "! Received compressed protocol Echo Reply with "
			"incorrect FCS\n");
		status = FALSE;
	  }
	  else {
		Log(LOGMEDIUM, "Compressed protocol Echo Reply has correct FCS\n");
	  }
	}
	
	DUTPPPResetConn(config, pppConn);
	NCPEstConnStateDestroy(lcpState);

	DUTSetPPPACFCompression(config, FALSE);
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;


  BEGIN_TEST(
     TEST_NUM
     "18.5",
     TEST_DESCRIPTION
	 "If an Option is received (ACFC) in a Configure-Request but\n"
	 "with an incorrect Length, a Configure-Nak SHOULD be transmitted\n"
	 "unless the Length goes beyond the end of the packet\n",
     TEST_REFERENCE
     "RFC 1661 s6 p40 LCP Configuration Options\n",
     TEST_METHOD
     "- ANVL: Send Configure-Requests with Address and Control Field\n"
	 "        Compression Option with incorrect length\n"
     "-  DUT: Should Configure-Nak\n"
     "- CASE: Length < 2\n"
	 "-  DUT: Should silently discard packet\n"
     "- CASE: Length > 2\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	NCPEstConnState_t *lcpState;
	boolean lcpStatus;
	ubyte4 i;

	for (i = 0; i < 5; i++) {

	  /* 2 is the right value, so skip it */
	  if (i == 2){
		continue;
	  }

	  DUTstart(0);

	  /* need to create a new lcpState each time, NCPEstConnStateCreate(LCP)
		 sets some variables so you can't use CLEAR_DATA() */
	  lcpState = NCPEstConnStateCreate(LCP);
	  
	  Log(LOGMEDIUM, "\nEstablish PPP connection\n");
	  lcpStatus = LCPEstConn(pppConn, lcpState);
	  if (!lcpStatus) {
		Log(LOGMEDIUM, "! Unable to establish connection with DUT\n");
		status = FALSE;
	  }
	  else {
		LCPAndExpectState_t *state;
		LCPConfigOptionForm_t *opt;
		LCPForm_t *lcp;
		ubyte buffer[MAX_PACKET_LEN];
		byte4 len;

		opt = LCPConfigOptionFormCreate();
		lcp = LCPFormCreate();
		state = LCPAndExpectStateCreate();

		FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

		FORM_SET_FIELD(opt, type, lcpConfigTypeACFC);
		FORM_SET_FIELD(opt, length, (ubyte)i);
		len = LCPConfigOptionBuild(pppConn, opt, buffer);
		FORM_SET_DATA(lcp, buffer, len);
		
		Log(LOGMEDIUM, "Sending LCP Configure-Requests with\n"
			"Address-And-Control-Field-Compression with length %ld\n", i);
 
		if(i<2) {
		  state->ncpType = lcpTypeConfigureNak;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			LCPConfigOptionForm_t *lcpNak;
			
			lcpNak = LCPConfigOptionFormCreate();
			
			LCPConfigOptionToForm(state->ncpForm.data, lcpNak);
			if (lcpNak->length != 2){
			  Log(LOGMEDIUM, "! An LCP Configure-Nak with an "
				  "incorrect option length %u was received\n", lcpNak->length);
			  status = FALSE;
			}
			else{
			  Log(LOGMEDIUM, "An LCP Configure-Nak was correctly received\n");
			}
			
			Free(lcpNak);
		  }
		  else{
			/* log message done in LCPAndExpect */
			status = FALSE;
		  }
		}
		else{
		  /* make sure we don't receive anything,
			 except maybe a ConfigureRequest      */
		  state->ncpType = 0;
		  state->ignoreType = lcpTypeConfigureRequest;
		  state->expectedPkts = 0;
		  pkt = LCPAndExpect(pppConn, lcp, state);
		  if (pkt) {
			/* we received something, so fail */
			status = FALSE;
		  }
		  else{
			Log(LOGMEDIUM, "DUT correctly discarded malformed packet\n");
		  }
		}
		
     	DUTSendConfigCommand((ubyte *) "stop$");
		Free(state);
		Free(lcp);
		Free(opt);
	  }
	  NCPEstConnStateDestroy(lcpState);
	  DUTPPPResetConn(config, pppConn);
	}
  }
  END_TEST;

  /* SECTION 6 ******************************************/
  BEGIN_TEST(
     TEST_NUM
     "19.1",
     TEST_DESCRIPTION
	 "None of the Configuration Options in this specification\n"
	 "can be listed more than once.\n",
     TEST_REFERENCE
     "RFC 1661 s6 p39 LCP Configuration Options\n",
     TEST_METHOD
	 "- ANVL: Establish a PPP Connection\n"
     "- ANVL: Send Configure-Request with 2 MRU Options\n"
     "-  DUT: Do not send a Configure-Ack\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	LCPConfigDataForm_t *lcpData;
	LCPForm_t *lcp;
	LCPAndExpectState_t *state;
	Packet_t *pkt;
	ubyte buffer[MAX_PACKET_LEN];
	byte4 len;
	boolean lcpStatus;

	DUTstart(0);/*see config IDs spec on epoc client side*/

	lcpData = LCPConfigDataFormCreate();
	lcp = LCPFormCreate();
	state = LCPAndExpectStateCreate();
	  
	lcpStatus = LCPEstConn(pppConn, 0);
	if(!lcpStatus){
	  Log(LOGMEDIUM, "! PPP Connection Not Established\n");
	  status = FALSE;
	}
	else{
	  FORM_SET_FIELD(lcp, code, lcpTypeConfigureRequest);

	  FORM_SET_FIELD(lcpData, mru, LCP_DIFFERENT_MRU);
	  len = LCPConfigDataBuildByType(pppConn, lcpConfigTypeMaximumReceiveUnit,
									 lcpData, buffer);
	  FORM_SET_FIELD(lcpData, mru, LCP_DIFFERENT_MRU * 2);
	  len += LCPConfigDataBuildByType(pppConn, lcpConfigTypeMaximumReceiveUnit,
									  lcpData, buffer + len);

	  FORM_SET_DATA(lcp, buffer, len);
	  
	  state->ncpType = lcpTypeConfigureAck;
	  state->expectedPkts = 0;

	  Log(LOGMEDIUM, "Send Configure-Request with two MRU Options\n");
	  pkt = LCPAndExpect(pppConn, lcp, state);

	  /* 
	   * log messages are taken care of in LCPAndExpect, we don't want to
	   * get an Ack for this invalid packet
	   */
	  status = (pkt == 0);
	}
	Free(lcp);
	Free(lcpData);
	Free(state);

	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;
  
  /* REGRESSION TESTS ******************************************/


/* +++alan: these should be written so they don't require LIB_IP */

  BEGIN_TEST(
     TEST_NUM
     "20.1",
     TEST_DESCRIPTION
     "Try to send packets with bad FCS\n",
     TEST_REFERENCE
     "Customer Request\n",
     TEST_METHOD
     "- ANVL: Establish LCP connection\n"
	 "- ANVL: Establish IPCP Connection\n"
     "- ANVL: Send an ICMP Echo with a bad HDLC FCS\n"
     "-  DUT: Should not respond\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	boolean ncpStatus;

	DUTstart(0);

	status = ANVLPPPEstConn(config, pppConn, 0);
	if (!status){
	  Log(LOGMEDIUM, "! Unable to establish connection with PFC\n");
	  status = FALSE;
	}
	else {
	  HDLCForm_t *hdlc;
	  Link_t *link;
	  
	  link = LinkCreate(LINK_PPP);
	  hdlc = HDLCFormCreate();
	  
	  FORM_SET_FIELD(hdlc, fcs, 0);
	  link->forms.ppp.hdlcForm = hdlc;
	  
	  ncpStatus =  ANVLNCPEstConnAndEcho(config, link, IPCP, 0, 0);
	  if (!ncpStatus) {
		Log(LOGMEDIUM,
			"Did not receive an Echo-Reply to Echo-Request with bad FCS\n");
	  }
	  else {
		Log(LOGMEDIUM, "! Received Echo-Reply to Echo-Request with bad FCS\n");
		status = FALSE;
	  }
	  
	  Free(hdlc);
	  Free(link);
	}
	DUTPPPResetConn(config, pppConn);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST;


  BEGIN_TEST(
     TEST_NUM
     "20.2",
     TEST_DESCRIPTION
     "Send an HDLC packet of 100K to try to overflow input buffers\n",
     TEST_REFERENCE
     "Customer Request: Beth Miaoulis\n",
     TEST_METHOD
     "- ANVL: Send an HDLC Framing byte\n"
     "- ANVL: Continue sending 100K of data without sending another\n"
	 "        HDLC framing packet (would signal the end of the packet)\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	AHDLCPktSrc_t *ahdlcSrc = (AHDLCPktSrc_t *)pppConn->pktSrc->opaque;
	byte4 i;
	ubyte buffer[MAX_PACKET_LEN], *bufPtr;
	byte tmpStr[MAX_STRING];
	Packet_t localPkt;
	Time_t timeOut = {1, 0};

	DUTstart(0);/*see config IDs spec on epoc client side*/

	Log(LOGMEDIUM, "Sending HDLC framing packet to start transmission\n");
	CLEAR_DATA(buffer);
	buffer[0] = HDLC_FLAG;
	buffer[1] = hdlcTypeAllStations;
	buffer[2] = HDLC_CONTROL_UI;
	localPkt.data = buffer;
	localPkt.len = 3;
	PktSrcWrite(ahdlcSrc->ttyPkt, &localPkt);

	CLEAR_DATA(buffer);
	bufPtr = buffer;

	SPrintf(tmpStr, "ANVL-%s-%s ", protocol, testNum);
	RepeatStringIntoData(tmpStr, StrLen(tmpStr),
						 buffer, 1024);
	localPkt.len = 1024;
	
	for (i = 0; i < 100; i++){
	  Log(LOGMEDIUM, "Sending 1024-byte-packet #%lu over HDLC\n", i);
	  PktSrcWrite(ahdlcSrc->ttyPkt, &localPkt);
	  Sleep(&timeOut);
	}
	DUTSendConfigCommand((ubyte *) "stop$");
  }
  END_TEST;

  /*+++art: This test is specific to some implementations method of using
	echos as a surrogate LQM. */
  BEGIN_TEST(
     TEST_NUM
     "20.3",
     TEST_DESCRIPTION
     "If using Echo-Requests to monitor link quality, DUT should terminate\n"
     "link after some number of Echo-Replies are not received\n",
     TEST_REFERENCE
     "Customer Request: Corwin\n",
     TEST_METHOD
     "-  DUT: Send n periodic Echo-Requests to verify link is up\n"
     "- ANVL: Only send Echo-Reply to 2 out of n Echo-Requests\n"
     "-  DUT: Should terminate link after only 2 out of n Echo-Requests\n"
     "        have been answered\n"
     "(Repeat for various patterns of which 2 Echo-Requests are answered)",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do Test */
  {
	Packet_t *pkt;
	boolean postStatus;
	LCPAndExpectState_t *state;
	ubyte4 i, j, k, numEchoPkts;

	DUTstart(0);

	state = LCPAndExpectStateCreate();

	numEchoPkts = 5; /* +++alan: DUTEchoLQMNumEchoPkts() */
	state->timeOut.sec = 61; /* +++alan: DUTEchoLQMPeriod() */

	for(i=0; i < (numEchoPkts-2); i++) {
	  for(j=i+1; j < numEchoPkts; j++) {

		if (!ANVLPPPEstConn(config, pppConn, 0)) {
		  Log(LOGMEDIUM, "! Unable to open PPP connection\n");
		  status = FALSE;
		}
		else {

		  for(k=0; k<numEchoPkts; k++) {
			state->ncpType = lcpTypeEchoRequest;
			state->timeOut = echoRequestTimeOut;
			state->runDefaultHandlers = ((k==i) || (k==j))? TRUE : FALSE;
			state->expectedPkts = 1;

			LogWaiting("for LCP Echo-Request", &state->timeOut);
			pkt = LCPAndExpect(pppConn, 0, state);
			
			if (state->expectedPkts != state->receivedPkts) {
			  status = FALSE;
			}
			else {
			  Log(LOGMEDIUM,
				  "Echo-Reply %ssent in response to Echo-Request\n",
				  state->runDefaultHandlers? "" : "not ");
			}
		  }
			
		  postStatus = LCPWatchForTerminateOrConfigure(pppConn);
		  if (!postStatus) {
			status = FALSE;
			Log(LOGMEDIUM, "! No Terminate/Configure Request received\n");
		  }

		}
		DUTPPPResetConn(config, pppConn);
	  }
	}

	Free(state);
 	DUTSendConfigCommand((ubyte *) "stop$");
 }
  END_TEST

  BEGIN_TEST(
     TEST_NUM
     "20.4",
     TEST_DESCRIPTION
     "Ensure that when a Config-Ack is sent before Max Configure\n"
	 "is reached, a Connection can be properly established\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Do not send Configure-Ack until after n Configure-Requests\n"
     "-  DUT: Continue to bring link up\n"
     "- CASE: n = 1-9\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	NCPEstConnState_t *state;
	ubyte4 i;

	for (i = 1; i < LCP_DEFAULT_MAX_CONFIGURE; i++) {

	  state = NCPEstConnStateCreate(LCP);
          DUTstart(0);/*see config IDs spec on epoc client side*/

	  /* respond after N requests */
	  state->sendReqAfterNReqs = state->sendAckAfterNReqs = i;

	  Log(LOGMEDIUM, 
		  "\nAttempt to bring up PPP Connection, "
		  "Acking after %lu Config-Reqs\n", i);

	  if (!ANVLPPPEstConn(config, pppConn, state)) {
		Log(LOGMEDIUM, 
			"! Unable to bring up PPP Connection when sending Config-Ack "
			"after %lu Config-Reqs\n", state->sendAckAfterNReqs);
		status = FALSE;
	  }
	  DUTPPPResetConn(config, pppConn);
	  NCPEstConnStateDestroy(state);
	  DUTSendConfigCommand((ubyte *) "stop$");
	}
  }
  END_TEST;
  
  BEGIN_TEST(
     TEST_NUM
     "20.5",
     TEST_DESCRIPTION
     "Ensure that when a Config-Ack is sent after the Restart Timer\n"
	 "that a Config-Req is retransmitted\n",
     TEST_REFERENCE
     "NEGATIVE: RFC 1661 s4.6 p24 Counters and Timers\n",
     TEST_METHOD
     "- ANVL: Cause DUT to open connection\n"
     "-  DUT: Send Configure-Request\n"
     "- ANVL: Do not send Configure-Ack until after n seconds\n"
     "-  DUT: Resend Configure-Request\n"
     "- CASE: n = 4-8\n",
     TEST_FLAGS
     TF_UNSPEC
  );
  /* Do test */
  {
	Packet_t *pkt, *pktResend;
	LCPForm_t *lcp, *lcpAck;
	LCPAndExpectState_t *state, *stateResend;
	ubyte4 i;
	Time_t waitTime = {0, 0};

	for (i = 0; i < 4; i++){

	  waitTime.sec = (LCP_DEFAULT_RESTART_TIMER + 1) + i;

      DUTstart(0);
	  lcp = LCPFormCreate();
	  lcpAck = LCPFormCreate();
	  state = LCPAndExpectStateCreate();
	  stateResend = LCPAndExpectStateCreate();

	  Log(LOGMEDIUM, "\nOpening PPP Connection\n");
	  DUTLCPOpen(config, pppConn);

	  state->ncpType = lcpTypeConfigureRequest;
	  pkt = LCPAndExpect(pppConn, 0, state);
	  if (!pkt) {
		status = FALSE;
	  }
	  else {
		Log(LOGMEDIUM, "Received Configure-Request packet\n");

		/* check length */
		if (state->ncpForm.length != (LCP_HDR_LEN + pkt->len)) {
		  Log(LOGMEDIUM, "! LCP Length (%u) does not match actual packet "
			  "length (%lu)\n",
			  state->ncpForm.length, (LCP_HDR_LEN + pkt->len));
		  status = FALSE;
		}
		else{
		  /* wait for the appropriate amount of time before Acking */
		  Log(LOGMEDIUM, 
			  "Waiting for %ld seconds before sending Config-Ack\n",
			  waitTime.sec);
		  RunHandlers(&waitTime);

		  stateResend->ncpType = lcpTypeConfigureRequest;
		  FORM_SET_FIELD(lcpAck, code, lcpTypeConfigureAck);
		  FORM_SET_FIELD(lcpAck, identifier, state->ncpForm.identifier);
		  FORM_SET_DATA(lcpAck, state->ncpForm.data, state->ncpForm.dataLen);

		  Log(LOGMEDIUM, 
			  "Sending Config-Ack for Config-Req with identifier %u\n",
			  lcpAck->identifier);
		  pktResend = LCPAndExpect(pppConn, lcpAck, stateResend);
		  if (!pktResend) {
			status = FALSE;
		  }
		  else if (!DATA_EQUAL(state->ncpForm.data, 
							   state->ncpForm.dataLen,
							   stateResend->ncpForm.data,
							   stateResend->ncpForm.dataLen)) {
			Log(LOGMEDIUM,
				"! Data in retransmitted Config-Req is not identical "
				"to the original data\n");
			status = FALSE;
		  }
		  else{
			Log(LOGMEDIUM, 
				"DUT Correctly retransmitted Config-Req (identifier %u)\n",
				stateResend->ncpForm.identifier);
		  }
		}
	  }
	  DUTPPPResetConn(config, pppConn);
	  DUTSendConfigCommand((ubyte *) "stop$");
	  Free(state);
	  Free(lcp);
	  Free(stateResend);
	  Free(lcpAck);
	}
  }
  END_TEST;

}

boolean
LCPBogusConfigureHandler(NetConn_t *n, Packet_t *pkt,
						 HandlerPurpose_t purpose,
						 ProcessState_t *ps, void *context)
{
  NCPEstConnState_t *lcpState = (NCPEstConnState_t *)context;
  LCPForm_t *lcp;
  struct LCPBogusConfigureState_s *cfgState = 
	  (struct LCPBogusConfigureState_s *)lcpState->userData;
  boolean retVal = TRUE;

  /* Convert to usable form */
  lcp = LCPFormCreate();
  LCPPacketToForm(pkt, lcp);

  if(lcp->code == lcpTypeConfigureRequest){
	lcpState->reqReceived++;

	/* Log incoming packet */
	Log(LOGMEDIUM, "Received LCP Configure-Request\n");
	PrintCurrentPacket(n, ps);
	
	LCPSendBogusResponse(n, lcp, cfgState);

	/* Save the ack'd options */
	LCPConfigDataToForm(lcp->data, lcp->dataLen,
						LCP_CFG(lcpState->remoteOptsAckd));
	
	lcpState->ackSent++;
	
	/* We're done if one has been sent AND received */
	if(lcpState->ackReceived){
	  StopHandlers();
	}

	/* override LCPEstConnHandler() */
	retVal = FALSE;
  }

  Free(lcp);
  return retVal;
}

void
LCPSendBogusResponse(NetConn_t *n, LCPForm_t *lcp,
					 struct LCPBogusConfigureState_s *cfgState) 
{
  ubyte type, cfgBuff[MAX_PACKET_LEN];
  ubyte4 len, cfgLen, cfgLast;
  byte codeStr[MAX_STRING], optStr[MAX_STRING];
  LCPConfigDataForm_t *opts;
  boolean optsOK;
  LCPConfigOptionForm_t *cfgOpt;

  opts = LCPConfigDataFormCreate();
  cfgOpt = LCPConfigOptionFormCreate();

  /* Change into a response */
  FORM_SET_FIELD(lcp, code, cfgState->responseCode);
  FORM_UNSET_FIELD(lcp, length);
  if (cfgState->bogusIdent) {
	FORM_SET_FIELD(lcp, identifier, lcp->identifier+1);
	Log(LOGMEDIUM,
		"Sending LCP %s with incorrect identifier %u\n",
		LCPCodeToString(cfgState->responseCode, codeStr),
		lcp->identifier);
  }
  if (cfgState->bogusLengthOK) {
	Log(LOGMEDIUM, "Sending LCP %s with incorrect length %u\n",
		LCPCodeToString(cfgState->responseCode, codeStr),
		cfgState->bogusLength);
	FORM_SET_FIELD(lcp, length, cfgState->bogusLength);
  }
  switch (cfgState->bogusData) {
  case REVERSE_OPTIONS:
	LCPConfigDataToForm(lcp->data, lcp->dataLen, opts);
	cfgLen = cfgLast = 0;
	for(type=lcpConfigTypeACFC; type>0; type--) {
	  len = LCPConfigDataBuildByType(n, type, opts, cfgBuff + cfgLen);
	  if (len > 0) {
		cfgLast = cfgLen;
		cfgLen += len;
	  }
	}
	if (cfgLast == 0) {
	  Log(LOGMEDIUM,
		  "DUT did not send 2 or more options, sending normal %s\n",
		  LCPCodeToString(cfgState->responseCode, codeStr));
	}
	else {
	  Log(LOGMEDIUM,
		  "Sending LCP %s with options in reverse order\n",
		  LCPCodeToString(cfgState->responseCode, codeStr));
	  FORM_SET_DATA(lcp, cfgBuff, cfgLen);
	}
	break;
  case OPTION_MISSING:
	LCPConfigDataToForm(lcp->data, lcp->dataLen, opts);
	cfgLen = cfgLast = 0;
	for(type=lcpConfigTypeMaximumReceiveUnit;
		type<=lcpConfigTypeACFC; type++) {
	  len = LCPConfigDataBuildByType(n, type, opts, cfgBuff + cfgLen);
	  if (len > 0) {
		cfgLast = cfgLen;
		cfgLen += len;
	  }
	}
	if (cfgLast == 0) {
	  Log(LOGMEDIUM,
		  "DUT did not send 2 or more options, sending normal %s\n",
		  LCPCodeToString(cfgState->responseCode, codeStr));
	}
	else {
	  Log(LOGMEDIUM,
		  "Sending LCP %s with last option missing\n",
		  LCPCodeToString(cfgState->responseCode, codeStr));
	  FORM_SET_DATA(lcp, cfgBuff, cfgLast);  /* leave off last option */
	}
	break;
  case BOGUS_VALUES:
	LCPConfigDataToForm(lcp->data, lcp->dataLen, opts);
	optsOK = FALSE;
	if (opts->accmOK) {
	  Log(LOGMEDIUM, "Changing ACCM from 0x%08lX to 0x%08lX\n",
		  opts->accm, opts->accm + 1);
	  opts->accm += 1;
	  optsOK = TRUE;
	}
	if (opts->magicNumOK) {
	  Log(LOGMEDIUM, "Changing Magic Number from 0x%08lX to 0x%08lX\n",
		  opts->magicNum, opts->magicNum + 1);
	  opts->magicNum += 1;
	  optsOK = TRUE;
	}
	if (opts->mruOK) {
	  Log(LOGMEDIUM, "Changing MRU from %u to %u\n",
		  opts->mru, opts->mru - 1);
	  opts->mru -= 1;
	  optsOK = TRUE;
	}
	if (!optsOK) {
	  Log(LOGMEDIUM, "DUT did not send any options with values, sending "
		  "normal %s\n", LCPCodeToString(cfgState->responseCode, codeStr));
	}
	else {
	  Log(LOGMEDIUM,
		  "Sending LCP %s with different values in options\n",
		  LCPCodeToString(cfgState->responseCode, codeStr));
	  cfgLen = NCPConfigDataBuild(n, LCP, (NCPConfigDataForm_t *)opts,cfgBuff);
	  FORM_SET_DATA(lcp, cfgBuff, cfgLen);
	}
	break;
  case EXTRA_OPTION:
	cfgLen = lcp->dataLen;
	MemMove(cfgBuff, lcp->data, cfgLen);
	FORM_SET_FIELD(cfgOpt, type, lcpConfigTypeReserved);
	cfgLen += LCPConfigOptionBuild(n, cfgOpt, cfgBuff+cfgLen);
	FORM_SET_DATA(lcp, cfgBuff, cfgLen);
	Log(LOGMEDIUM, "Sending LCP %s with extra option of type %s\n",
		LCPCodeToString(cfgState->responseCode, codeStr),
		LCPConfigOptionTypeToString(cfgOpt->type, optStr));
	break;
  default:
	break;
  }
  
  LCPSend(n, lcp, 0, 0);

  Free(cfgOpt);
  Free(opts);
}

static boolean
LCPGrabRawEchoHandler(NetConn_t *n, Packet_t *pkt, HandlerPurpose_t purpose,
					  ProcessState_t *ps, void *context)
{
  LCPForm_t lcp;
  LCPAndExpectState_t *state;

  state = context;

  LCPPacketToForm(PktHistory(ps, LCP), &lcp);

  if (lcp.code != lcpTypeEchoReply){
	return TRUE;
  }

  PrintCurrentPacket(n, ps);

  state->pkt = *PktHistory(ps, n->pktSrc->entryPoint);
  state->receivedPkts++;

  StopHandlers();
  return FALSE;
}

static boolean
LCPTimingHandler(NetConn_t *n, Packet_t *pkt, HandlerPurpose_t purpose,
                 ProcessState_t *ps, void *context)
{
  LCPForm_t *lcp;
  byte codeStr[MAX_STRING];
  struct TimingStats_s *stats = ((NCPEstConnState_t *)context)->userData;

  lcp = LCPFormCreate();
  LCPPacketToForm(pkt, lcp);

  /* Only interested in packets of type lcpType */
  if(lcp->code != stats->lcpType){
	Free(lcp);
	return TRUE;
  }

  /* the Terminate-Request tests do not run under an EstConn() routine, so
	 we need to print a log msg */
  if (lcp->code == lcpTypeTerminateRequest) {
	Log(LOGMEDIUM, "Received LCP Terminate-Request\n");
	PrintCurrentPacket(n, ps);
  }

  Free(lcp);

  /* Don't overflow the array */
  if(stats->numSeen >= MAX_CONFIG_REQUESTS){
	Log(LOGMEDIUM, "! Too many %ss have been seen: %lu\n",
		LCPCodeToString(stats->lcpType, codeStr),
		stats->numSeen);
	return TRUE;
  }

  /* Store the data */
  GetTime(&(stats->times[stats->numSeen]));
  stats->numSeen++;

  return TRUE;
}

/*>>

  (boolean) status = ValidateTimingStats(struct TimingStats_s *state,
                                         Time_t *targetValue,
										 Time_t *allowance)

  REQUIRES:
 
  DESCRIPTION:

  Determines if the intervals between packet times in "state" are
  within "allowance" of "targetTime".  A table of the interpacket
  times will be logged for the user's information.
  
  ARGS:
  state             structure containing the timing samples to validate
  targetValue       target value of interpacket gap
  allowance         length of time that the interpacket gap can vary from
                    the targetValue without causing failure

  RETURNS:
  TRUE if each interpacket gap is within "allowance" of "targetValue";
  FALSE otherwise

<<*/
boolean
ValidateTimingStats(struct TimingStats_s *stats, Time_t *targetValue,
					Time_t *allowance)
{
  ubyte4 i;
  Time_t upperBound, lowerBound;
  Time_t diffTime = {0, 0};
  boolean status = TRUE;
  real4 timeVal;
  byte str1[MAX_STRING], str2[MAX_STRING];

  /* calculate upper and lower bounds */
  upperBound = lowerBound = *targetValue;

  /* adjust upper and lower bounds by allowance */
  TimeAddDelta(&upperBound, allowance);
  TimeSubtractDelta(&lowerBound, allowance);
	
  /* log header */
  Log(LOGMEDIUM,
	  "Packet      Relative time (seconds)\n"
	  "------      -----------------------\n");
  
  /* log initial packet time */
  Log(LOGMEDIUM,
	  "%4u %20.2f\n",
	  1, (real4)0.0);
  
  for (i = 1; i < stats->numSeen; i++) {
   	diffTime = TimeSubtract(&stats->times[i-1], &stats->times[i]);
	timeVal = (real4)(diffTime.sec + diffTime.usec/1000000.0);

	Log(LOGMEDIUM,
		"%4lu %20.2f\n",
		i + 1, timeVal);
	
	/* validate each interpacket gap is within allowed difference of
       target value */
	if (!(TimeGreater(&diffTime, &lowerBound) &&
		  TimeGreater(&upperBound, &diffTime))) {
	  /* diffTime is either less than lowerBound or greater than
         upperBound, which indicates that the interpacket gap is not
         within the allowed difference of target value */
	  status = FALSE;
	}
  }

  Log(LOGMEDIUM,
	  "\n"
	  "%sEach interpacket gap %s within %s seconds of %s seconds\n",
	  status?"":"! ", status?"was":"was not",
	  TimeToString(allowance, str1), TimeToString(targetValue, str2));

  return status;
}


/*>>

  (boolean) status = LCPEchoAYT(NetConn_t *n, ubyte4 magic, byte *testNum)

  REQUIRES:
 
  DESCRIPTION:
  Sends an LCP Echo-Request packet over the interface specified by "n" and
  verifies than an Echo-Reply packet is received.

  ARGS:
  n                 network connection on which to send Echo-Request
  magic             negotiated magic number to use in Echo-Request
  testNum           test number of current ANVL test, for packet contents

  RETURNS:
    TRUE if LCP Echo-Reply packet was received, otherwise FALSE.

<<*/

static boolean
LCPEchoAYT(NetConn_t *n, ubyte4 magic, byte *testNum)
{
  LCPAndExpectState_t *state;
  LCPForm_t *lcp;
  Packet_t *pkt;
  LCPEchoDiscardForm_t *lcpEcho;
  
  lcp = LCPFormCreate();
  lcpEcho = LCPEchoDiscardFormCreate();
  state = LCPAndExpectStateCreate();

  FORM_SET_FIELD(lcpEcho, magicNumber, magic);

  Log(LOGMEDIUM, "Sending Echo-Request to see if DUT is still responding\n");
  pkt = LCPEchoAndExpect(n, lcpEcho, lcp, state, testNum);

  Free(lcp);
  Free(lcpEcho);
  Free(state);

  return (pkt != 0);
}

static boolean
LCPRejHandler(NetConn_t *n, Packet_t *pkt, HandlerPurpose_t purpose,
			  ProcessState_t *ps, void *context)
{
  LCPForm_t *lcp, *lcpRej;
  LCPProtocolRejectForm_t *rej;
  NCPEstConnState_t *lcpState = context;
  struct ProtocolReject_s *rejState = lcpState->userData;
  ubyte buff[MAX_PACKET_LEN];
  ubyte4 buffLen;
  byte codeStr[MAX_STRING];
 
  /* Ignore the first packet in the exchange for enhanced effect */
  if(!rejState->sawOne){
	lcp = LCPFormCreate();
	LCPPacketToForm(pkt, lcp);
	Log(LOGMEDIUM, "Ignoring first LCP Packet (type %s)\n",
		LCPCodeToString(lcp->code, codeStr));
	PrintCurrentPacket(n, ps);
	Free(lcp);
	rejState->sawOne = TRUE;
	return FALSE;
  }

  /*
	If we have sent a request, received a request and sent an ack, the DUT
	could have *just* sent an ack to us and consider themselves in the
	open state, so we shouldn't send a reject at that point
  */
  if((lcpState->reqSent > 0) &&
	 (lcpState->reqReceived > 0) &&
	 (lcpState->ackSent > 0)){
	return TRUE;
  }

  rej = LCPProtocolRejectFormCreate();
  FORM_SET_FIELD(rej, rejectedProtocol, rejState->protocol);
  /*+++art: Truncate to MRU */
  FORM_SET_VARFIELD(rej, rejectedInfo, pkt->data, pkt->len);
  buffLen = LCPProtocolRejectBuild(n, rej, buff);

  lcpRej = LCPFormCreate();
  FORM_SET_FIELD(lcpRej, code, lcpTypeProtocolReject);
  FORM_SET_DATA(lcpRej, buff, buffLen);

  Log(LOGMEDIUM, "Sending Protocol-Reject for %s\n",
	  PPPProtocolToString(rejState->protocol, codeStr));

  LCPSend(n, lcpRej, 0, 0);

  Free(rej);
  Free(lcpRej);
  return TRUE;
}

static boolean
PPPCheckCompressionHandler(NetConn_t *n, Packet_t *pkt,
						   HandlerPurpose_t purpose,
						   ProcessState_t *ps, void *context)
{
  PPPForm_t *ppp;
  struct PPPCheckCompressionState_s *state = 
	  (struct PPPCheckCompressionState_s *)context;

  ppp = PPPFormCreate();

  if (!state->receivedPkt) {

	if (n->pktSrc->type == PKTSRC_AHDLC) {
	  HDLCForm_t *hdlc;
	  ubyte2 fcs;

	  hdlc = HDLCFormCreate();

	  pkt = PktHistory(ps, HDLC);
	  if (!pkt) {
		Error(FATAL,
			  "Did not receive HDLC packet in PPPCheckCompressionHandler()\n");
	  }
	  
	  HDLCPacketToForm(pkt, hdlc, TRUE);
	  
	  fcs = hdlcfcs(HDLC_INIT_FCS, pkt->data, pkt->len-HDLC_FCS_LEN) ^ 0xFFFF;
	  if(fcs == hdlc->fcs){
		state->fcsCorrect = TRUE;
	  }
	  state->addressControlCompressed = hdlc->addressControlCompressed;

	  Free(hdlc);
	}

	pkt = PktHistory(ps, PPP);
	if (!pkt) {
	  Error(FATAL,
			"Did not receive PPP packet in PPPCheckCompressionHandler()\n");
	}
	PPPPacketToForm(pkt, ppp);
	state->protocolCompressed = ppp->protocolCompressed;

	state->receivedPkt = TRUE;
	PrintCurrentPacket(n, ps);
  }

  Free(ppp);

  return TRUE;
}

static boolean
NCPIgnoreNakHandler(NetConn_t *n, Packet_t *pkt,
					HandlerPurpose_t purpose,
					ProcessState_t *ps, void *context)
{
  NCPEstConnState_t *ncpState = (NCPEstConnState_t *)context;
  NCPForm_t *ncp;
  boolean status = TRUE;

  /* Convert to usable form */
  ncp = NCPFormCreate();
  NCPPacketToForm(pkt, ncp);

  if(ncp->code == ncpTypeConfigureNak){
	ncpState->nakReceived++;
	Log(LOGMEDIUM, "Ignoring Configure-Nak #%lu\n", ncpState->nakReceived);
	status = FALSE;
  }

  Free(ncp);
  return status;
}

/*>>

  (boolean) status = LCPWatchForTerminateOrConfigure(NetConn_t *n)

  REQUIRES:
 
  DESCRIPTION:
  Waits 35 seconds for either an LCP Terminate-Request or Configure-Request
  packet -- this is useful as an indication that an LCP connection has been
  closed and/or reset by the peer.

  ARGS:
  n                 the network connection on which to listen for LCP packets

  RETURNS:
    TRUE if an LCP Configure-Request or Terminate-Request was received.

<<*/

static boolean
LCPWatchForTerminateOrConfigure(NetConn_t *n)
{
  HandlerID_t hand;
  boolean sawOne = FALSE;
  Time_t timeOut = { 35, 0 };
  
  hand = ANVLHandlerInstall(n, LCP, 0, 0,
							LCPWatchForTerminateOrConfigureHandler, &sawOne);
  LogWaiting("for Terminate/Configure-Request packets", &timeOut);
  RunHandlers(&timeOut);
  HandlerRemove(n, hand, LCP);

  return sawOne;
}

static boolean
LCPWatchForTerminateOrConfigureHandler(NetConn_t *n,
									   Packet_t *pkt, HandlerPurpose_t purpose,
									   ProcessState_t *ps, void *context)
{
  LCPForm_t *lcp;
  boolean *sawOne;
  byte str[MAX_STRING];
  
  sawOne = context;
  
  lcp = LCPFormCreate();
  LCPPacketToForm(PktHistory(ps, LCP), lcp);
  
  if ((lcp->code == lcpTypeTerminateRequest) ||
	  (lcp->code == lcpTypeConfigureRequest)) {
	StopHandlers();
	*sawOne = TRUE;
  }

  if (lcp->code == lcpTypeEchoRequest){
	/* We do not want to answer Echo-Requests at this time because
       that will prevent the DUT from "timing out" and getting to the
       thing that ANVL wants to do: sending a TerminateRequest */
	Free(lcp);
	return FALSE;
  }

  Log(LOGMEDIUM, "Received %s packet\n", LCPCodeToString(lcp->code, str));
  PrintCurrentPacket(n, ps);
  Free(lcp);
  return TRUE;
}


