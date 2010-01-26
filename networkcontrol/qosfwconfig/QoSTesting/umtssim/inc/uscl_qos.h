// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//


#ifndef _USCL_QOS_H__
#define _USCL_QOS_H__

#include <e32base.h>
#include "uscl_packet.h"

/*
//SBLP
// Constant definitions particular to the Generic Parameters
const TUint8 KMaxFQDNLength = 255;
// Typedef for the AuthToken Holder
typedef TBuf8<KMaxFQDNLength> TAuthToken;
*/


//**********************************
// RPacketQoS
//**********************************
// This class implements quality of service specific part of ETel packet data api.

class CPacketQoSInternalData;
class RPacketQoS : public RSubSessionBase
    {
    public:
      //
      // Nested enums & classes
      //
	/** Defines the QoS reliability settings for GRPS networks. */
	enum TQoSReliability	// GPRS Release 97/98
		{
		/** Best effort or subscribed value. */
		EUnspecifiedReliabilityClass = 0x01,
		/** Reliability Class 1. */
		EReliabilityClass1			 = 0x02,
		/** Reliability Class 2. */
		EReliabilityClass2			 = 0x04,
		/** Reliability Class 3. */
		EReliabilityClass3			 = 0x08,
		/** Reliability Class 4. */
		EReliabilityClass4			 = 0x10,
		/** Reliability Class 5. */
		EReliabilityClass5			 = 0x20
		};

	/** Defines the QoS precedence for GRPS networks. */
	enum TQoSPrecedence		// GPRS Release 97/98
		{
		/** Best effort or subscribed value */
		EUnspecifiedPrecedence		= 0x01, 
		/** High priority precedence. */
		EPriorityHighPrecedence     = 0x02,
		/** Medium priority precedence. */
		EPriorityMediumPrecedence  = 0x04,
		/** Low priority precedence. */
		EPriorityLowPrecedence		= 0x08
		};

	/** Defines the QoS delay for GPRS and CDMA200 networks.
	 */
	enum TQoSDelay			// GPRS Release 97/98, CDMA2000
		{
		/** Best effort or subscribed value. */
		EUnspecifiedDelayClass	= 0x01, 
		/** Delay class 1. */
		EDelayClass1			= 0x02,
		/** Delay class 2. */
		EDelayClass2			= 0x04,
		/** Delay class 3. */
		EDelayClass3			= 0x08,
		/** Delay class 4. */
		EDelayClass4			= 0x10,
		/** Delay 40 milli seconds.
	
		CDMA2000 specific */
		EDelay40ms				= 0x20,	
		/** Delay 120 milli seconds.
	
		CDMA2000 specific */
		EDelay120ms				= 0x40,	
		/** Delay 360 milli seconds.
	
		CDMA2000 specific */
		EDelay360ms				= 0x80	
		};

	/** Defines the QoS peak throughput rates for GRPS networks. */
	enum TQoSPeakThroughput // GPRS Release 97/98
		{
		/** Best effort or subscribed value. */
		EUnspecifiedPeakThroughput  = 0x001, 
		/** Peak throughput of 1,000. */
		EPeakThroughput1000			= 0x002,
		/** Peak throughput of 2,000. */
		EPeakThroughput2000			= 0x004,
		/** Peak throughput of 4,000. */
		EPeakThroughput4000			= 0x008,
		/** Peak throughput of 8,000. */
		EPeakThroughput8000			= 0x010,
		/** Peak throughput of 16,000. */
		EPeakThroughput16000		= 0x020,
		/** Peak throughput of 32,000. */
		EPeakThroughput32000		= 0x040,
		/** Peak throughput of 64,000. */
		EPeakThroughput64000		= 0x080,
		/** Peak throughput of 128,000. */
		EPeakThroughput128000		= 0x100,
		/** Peak throughput of 256,000. */
		EPeakThroughput256000		= 0x200
		};

	/** Defines the mean throughput for GRPS networks. */
	enum TQoSMeanThroughput	// GPRS Release 97/98 
		{
		/** Unsubscribed value. */
		EUnspecifiedMeanThroughput	= 0x00001,
		/** Mean throughput of 100. */
		EMeanThroughput100			= 0x00002,
		/** Mean throughput of 200. */
		EMeanThroughput200			= 0x00004,
		/** Mean throughput of 500. */
		EMeanThroughput500			= 0x00008,
		/** Mean throughput of 1,000. */
		EMeanThroughput1000			= 0x00010,
		/** Mean throughput of 2,000. */
		EMeanThroughput2000			= 0x00020,
		/** Mean throughput of 5,000. */
		EMeanThroughput5000			= 0x00040,
		/** Mean throughput of 10,000. */
		EMeanThroughput10000		= 0x00080,
		/** Mean throughput of 20,000. */
		EMeanThroughput20000		= 0x00100,
		/** Mean throughput of 50,000. */
		EMeanThroughput50000		= 0x00200,
		/** Mean throughput of 100,000. */
		EMeanThroughput100000		= 0x00400,
		/** Mean throughput of 200,000. */
		EMeanThroughput200000		= 0x00800,
		/** Mean throughput of 500,000. */
		EMeanThroughput500000		= 0x01000,
		/** Mean throughput of 1,000,000. */
		EMeanThroughput1000000		= 0x02000,
		/** Mean throughput of 2,000,000. */
		EMeanThroughput2000000		= 0x04000,
		/** Mean throughput of 5,000,000. */
		EMeanThroughput5000000		= 0x08000,
		/** Mean throughput of 10,000,000. */
		EMeanThroughput10000000		= 0x10000,
		/** Mean throughput of 20,000,000. */
		EMeanThroughput20000000		= 0x20000,
		/** Mean throughput of 50,000,000. */
		EMeanThroughput50000000		= 0x40000,
		/** Best effort. */
		EMeanThroughputBestEffort	= 0x80000
		};

	/** Defines the QoS link priority for CMDA2000 networks. */
	enum TQoSLinkPriority	
		{
		/** No link priority. */
		ELinkPriority00 = 0x0001, 
		/** 1/13th's of user's subscription priority. */
		ELinkPriority01 = 0x0002,
		/** 2/13th's of user's subscription priority. */
		ELinkPriority02 = 0x0004,
		/** 3/13th's of user's subscription priority. */
		ELinkPriority03	= 0x0008,
		/** 4/13th's of user's subscription priority. */
		ELinkPriority04	= 0x0010,
		/** 5/13th's of user's subscription priority. */
		ELinkPriority05	= 0x0020,
		/** 6/13th's of user's subscription priority. */
		ELinkPriority06	= 0x0040,	
		/** 7/13th's of user's subscription priority. */
		ELinkPriority07	= 0x0080,
		/** 8/13th's of user's subscription priority. */
		ELinkPriority08	= 0x0100,
		/** 9/13th's of user's subscription priority. */
		ELinkPriority09	= 0x0200,
		/** 10/13th's of user's subscription priority. */
		ELinkPriority10	= 0x0400,
		/** 11/13th's of user's subscription priority. */
		ELinkPriority11	= 0x0800,
		/** 12/13th's of user's subscription priority. */
		ELinkPriority12	= 0x1000,
		/** Subscription priority (13/13th's). */
		ELinkPriority13	= 0x2000	
		};

	/** Defines the QoS data loss rate. */
	enum TQoSDataLoss
		{
		/** 1% data loss rate. */
		EDataLoss1	 =	0x01,	
		/** 2% data loss rate. */
		EDataLoss2	 =	0x02,	
		/** 5% data loss rate. */
		EDataLoss5	 =	0x04,	
		/** 10% data loss rate. */
		EDataLoss10	 =	0x08	
		};

	/** Defines the QoS data rate. */
	enum TQoSDataRate
		{
		/** A data rate of 8 kb/s. */
		EDataRate8kbps	  =	0x01,
		/** A data rate of 32 kb/s. */
		EDataRate32kbps	  =	0x02,
		/** A data rate of 64 kb/s. */
		EDataRate64kbps	  =	0x04,
		/** A data rate of 144 kb/s. */
		EDataRate144kbps  =	0x08,
		/** A data rate of 384 kb/s */
		EDataRate384kbps  =	0x10
		};

	//
	// TRLPMode - allows the client to specify (if desired) one of the following:
	// transparent only, tranparent preferred, non-transparent only or non-transparent 
	// preferred Radio Link Protocol Mode
	//
	/** Defines the Radio Link Protocol (RPL) mode. */
	enum TRLPMode
		{
		/** RPL mode unknown. */
		KRLPUnknown				= 0x01,		
		/** Transparent mode only. */
		KRLPTransparent			= 0x02,
		/** Non-transparent mode only. */
		KRLPNonTransparent		= 0x04,
		/** Transparent mode preferred. */
		KRLPTransparentPref		= 0x08,
		/** Non-transparent mode preferred. */
		KRLPNonTransparentPref	= 0x10
		};

	// The enums TTrafficClass, TDeliveryOrder,TErroneousSDUDelivery, TBitErrorRatio,
	// TSDUErrorRatio, TTrafficHandlingPriority have been assigned values because
	// the same enums are used both in the TQoSR99_R4Requested / Negotiated classes and 
	// in the TQoSCapsR99_R4 class. The Caps class has to indicate which, for instance, 
	// traffic classes are supported in a bitfield, so the enums have been defined as 
	// different bits in a bit field.
	enum TTrafficClass			
		{
		ETrafficClassUnspecified	= 0x01,		//< Traffic class - Unspecified
		ETrafficClassConversational	= 0x02,		//< Traffic class - Conversational
		ETrafficClassStreaming		= 0x04,		//< Traffic class - Streaming
		ETrafficClassInteractive	= 0x08,		//< Traffic class - Interactive
		ETrafficClassBackground		= 0x10		//< Traffic class - Background
		};

	enum TDeliveryOrder		
		{
		EDeliveryOrderUnspecified	= 0x01,		//< SDU Delivery order - Unspecified
		EDeliveryOrderRequired		= 0x02,		//< SDU Delivery order - Required to be in sequence
		EDeliveryOrderNotRequired	= 0x04		//< SDU Delivery order - Not Required to be in sequence
		};

	enum TErroneousSDUDelivery		// Erroneous SDU Delivery
		{
		EErroneousSDUDeliveryUnspecified	= 0x01,	//< Unspecified
		EErroneousSDUNoDetection			= 0x02,	//< Erroneous SDUs delivered - Error detection not considered.
		EErroneousSDUDeliveryRequired		= 0x04,	//< Erroneous SDUs delivered + error indication - Error detection employed.
		EErroneousSDUDeliveryNotRequired	= 0x08	//< Erroneous SDUs discarded - Error detection is employed.
		};

	enum TBitErrorRatio				// Residual Bit Error Rate
		{
		EBERUnspecified				= 0x01,		//< Target residual undetected BER - Unspecified
		EBERFivePerHundred			= 0x02,		//< Target residual BER - 0.05
		EBEROnePerHundred			= 0x04,		//< Target residual BER - 0.01
		EBERFivePerThousand			= 0x08,		//< Target residual BER - 0.005
		EBERFourPerThousand			= 0x10,		//< Target residual BER - 0.004
		EBEROnePerThousand			= 0x20,		//< Target residual BER - 0.001
		EBEROnePerTenThousand		= 0x40,		//< Target residual BER - 0.0001
		EBEROnePerHundredThousand	= 0x80,		//< Target residual BER - 0.00001
		EBEROnePerMillion			= 0x100,	//< Target residual BER - 0.000001
		EBERSixPerHundredMillion	= 0x200		//< Target residual BER - 0.00000006
		};

	enum TSDUErrorRatio				// SDU Error Ratio
		{
		ESDUErrorRatioUnspecified			= 0x01,	//< Target value of Erroneous SDUs - Unspecified
		ESDUErrorRatioOnePerTen				= 0x02,	//< Target SDU error ratio - 0.1
		ESDUErrorRatioOnePerHundred			= 0x04,	//< Target SDU error ratio - 0.01
		ESDUErrorRatioSevenPerThousand		= 0x08,	//< Target SDU error ratio - 0.007
		ESDUErrorRatioOnePerThousand		= 0x10,	//< Target SDU error ratio - 0.001
		ESDUErrorRatioOnePerTenThousand		= 0x20,	//< Target SDU error ratio - 0.0001
		ESDUErrorRatioOnePerHundredThousand	= 0x40,	//< Target SDU error ratio - 0.00001
		ESDUErrorRatioOnePerMillion			= 0x80	//< Target SDU error ratio - 0.000001
		};

	enum TTrafficHandlingPriority	// Traffic handling priority
		{
		ETrafficPriorityUnspecified	= 0x01,		//< Unspecified Priority level
		ETrafficPriority1			= 0x02,		//< Priority level 1
		ETrafficPriority2			= 0x04,		//< Priority level 2
		ETrafficPriority3			= 0x08		//< Priority level 3
		};

	struct TBitRate					// Bit rates for uplink and downlink
		{
		TInt iUplinkRate;			//< Uplink bitrate in kbps. Range 0 - 8640
		TInt iDownlinkRate;			//< Downlink bitrate in kbps. Range 0 - 8640
		};
			
	/**
	Source statistics descriptor - as defined in 3GPP TS 23.107 and TS 24.008.
	
	@publishedPartner
	@released
	*/
	enum TSourceStatisticsDescriptor	
		{
		/** Unknown source statistics descriptor. */
		ESourceStatisticsDescriptorUnknown	= 0x0,		
		/** Speech source statistics descriptor. */
		ESourceStatisticsDescriptorSpeech	= 0x01,		 
		};
		
	//
	// QoS capabilities classes
	//
	class TQoSCapsGPRS : public TPacketDataConfigBase
	/**
	Supported GPRS QoS capabilities.

	@publishedPartner
	@released
	*/
		{
	public:
		IMPORT_C TQoSCapsGPRS(); // iExtensionId = KConfigGPRS
	public:
		/** Bit-wise sum of the TQoSPrecedence attributes. 
		
		The default value is EUnspecifiedPrecedence. */
		TUint iPrecedence;
		/** Bit-wise sum of the TQoSDelay attributes. 
		
		The default value is EUnspecifiedDelay. */
		TUint iDelay;
		/** Bit-wise sum of the TQoSReliability attributes. 
		
		The default value is EUnspecifiedReliability. */
		TUint iReliability;
		/** Bit-wise sum of the TQoSPeakThroughput attributes. 
		
		The default value is EUnspecifiedPeakThroughput. */
		TUint iPeak;
		/** Bit-wise sum of the TQoSMeanThroughput attributes. 
		
		The default value is EUnspecifiedMeanThroughput. */
		TUint iMean;
		};

	class TQoSCapsCDMA2000 : public TPacketDataConfigBase
	/**
	Supported CDMA2000 QoS capabilities.

	@publishedPartner
	@released
	*/
		{
	public:
		IMPORT_C TQoSCapsCDMA2000(); // iExtensionId = KConfigCDMA
	public:
		/** Bit-wise sum of the TQoSLinkPriority attributes. */
		TUint	iPriority;
		/** Bit-wise sum of the TQoSDataRate attributes for the uplink. */
		TUint	iUplinkRate;
		/** Bit-wise sum of the TQoSDataRate attributes for the downlink. */
		TUint	iDownlinkRate;
		/** Bit-wise sum of the TQoSDataLoss attributes. */
		TUint	iFwdLossRate;
		/** Bit-wise sum of the TQoSDataLoss attributes. */
		TUint	iRevLossRate;
		/** Bit-wise sum of the TQoSDelay attributes. */
		TUint	iFwdMaxDelay;
		/** Bit-wise sum of the TQoSDelay attributes. */
		TUint	iRevMaxDelay;
		};

	
	class TQoSCapsR99_R4 : public TPacketDataConfigBase
	/**
	GPRS/UMTS Rel99 and UMTS Rel4 QoS capabilities class.
	
	@publishedPartner
	@released
	*/
		{
	public:
		IMPORT_C TQoSCapsR99_R4(); 
	public:
		TUint iTrafficClass;			//< Supported traffic class of the MT
		TUint iDeliveryOrderReqd;		//< SDU sequential delivery
		TUint iDeliverErroneousSDU;		//< Delivery of erroneous SDUs
		TUint iBER;						//< Target Bit Error Ratio (BER)
		TUint iSDUErrorRatio;			//< Target SDU Error Ratio
		TUint iTrafficHandlingPriority; //< Traffic handling priority
		};
		

      //
      // QoS configuration classes
      //
      class TQoSGPRSRequested : public TPacketDataConfigBase // GPRS
          {
          public:
            IMPORT_C TQoSGPRSRequested();
          public:
            TQoSPrecedence		iReqPrecedence;
            TQoSPrecedence		iMinPrecedence;
            TQoSDelay			iReqDelay;
            TQoSDelay			iMinDelay;
            TQoSReliability		iReqReliability;
            TQoSReliability		iMinReliability;
            TQoSPeakThroughput	iReqPeakThroughput;
            TQoSPeakThroughput	iMinPeakThroughput;
            TQoSMeanThroughput	iReqMeanThroughput;
            TQoSMeanThroughput	iMinMeanThroughput;
          };

      class TQoSGPRSNegotiated : public TPacketDataConfigBase // GPRS
          {
          public:
            TQoSGPRSNegotiated() { iExtensionId = TPacketDataConfigBase::KConfigGPRS; };
          public:
            TQoSPrecedence		iPrecedence;
            TQoSDelay			iDelay;
            TQoSReliability		iReliability;
            TQoSPeakThroughput	iPeakThroughput;
            TQoSMeanThroughput	iMeanThroughput;
          };

      class TQoSCDMA2000Requested : public TPacketDataConfigBase // CDMA2000
          {
          public:
            IMPORT_C TQoSCDMA2000Requested();
          public:
            TBool				iAssuredMode;	// assured vs. non-assured mode
            TRLPMode			iRLPMode;
            TQoSLinkPriority	iPriorityMode;	// fwd & reverse link will be kept to the same values!
            TQoSDataRate		iReqUplinkRate;
            TQoSDataRate		iMinUplinkRate;
            TQoSDataRate		iReqDownlinkRate;
            TQoSDataRate		iMinDownlinkRate;
            TQoSDataLoss		iFwdRequestedLossRate;
            TQoSDataLoss		iRevRequetsedLossRate;
            TQoSDataLoss		iFwdAcceptedLossRate;
            TQoSDataLoss		iRevAcceptedLossRate;
            TQoSDelay			iFwdMaxRequestedDelay;
            TQoSDelay			iRevMaxRequestedDelay;
            TQoSDelay			iFwdMaxAcceptedDelay;
            TQoSDelay			iRevMaxAcceptedDelay;
          };

      class TQoSCDMA2000Negotiated : public TPacketDataConfigBase // CDMA2000
          {
          public:
            IMPORT_C TQoSCDMA2000Negotiated();
          public:
            TBool				iAssuredMode;	// assured vs. non-assured mode. 
            TRLPMode			iRLPMode;
            TQoSLinkPriority	iPriorityMode;	// fwd & reverse link will be kept to the same values!
            TQoSDataRate		iUplinkRate;
            TQoSDataRate		iDownlinkRate;
            TQoSDataLoss		iFwdDataLossRate;
            TQoSDataLoss		iRevDataLossRate;
            TQoSDelay			iFwdMaxDelay;
            TQoSDelay			iRevMaxDelay;
          };

      class TQoSR99_R4Requested : public TPacketDataConfigBase // UMTS
          {
          public:
            IMPORT_C TQoSR99_R4Requested();
          public:
            TTrafficClass				iReqTrafficClass;			//< Requested traffic class	
            TTrafficClass				iMinTrafficClass;			//< Minimum acceptable traffic class
            TDeliveryOrder				iReqDeliveryOrderReqd;		//< Requested value for sequential SDU delivery
            TDeliveryOrder				iMinDeliveryOrderReqd;		//< Minimum acceptable value for sequential SDU delivery
            TErroneousSDUDelivery		iReqDeliverErroneousSDU;	//< Requested value for erroneous SDU delivery
            TErroneousSDUDelivery		iMinDeliverErroneousSDU;	//< Minimum acceptable value for erroneous SDU delivery
            TInt						iReqMaxSDUSize;				//< Request maximum SDU size
            TInt						iMinAcceptableMaxSDUSize;	//< Minimum acceptable SDU size
            TBitRate					iReqMaxRate;				//< Requested maximum bit rates on uplink and downlink
            TBitRate					iMinAcceptableMaxRate;		//< Minimum acceptable bit rates on uplink and downlink
            TBitErrorRatio				iReqBER;					//< Requested target BER
            TBitErrorRatio				iMaxBER;					//< Maximum acceptable target BER
            TSDUErrorRatio				iReqSDUErrorRatio;			//< Requested target SDU error ratio
            TSDUErrorRatio				iMaxSDUErrorRatio;			//< Maximum acceptable target SDU error ratio
            TTrafficHandlingPriority	iReqTrafficHandlingPriority;//< Requested traffic handling priority
            TTrafficHandlingPriority	iMinTrafficHandlingPriority;//< Minimum acceptable traffic handling priority
            TInt						iReqTransferDelay;			//< Requested transfer delay (in milliseconds)
            TInt						iMaxTransferDelay;			//< Maximum acceptable  transfer delay (in milliseconds)
            TBitRate					iReqGuaranteedRate;			//< Requested guaranteed bit rates on uplink and downlink
            TBitRate					iMinGuaranteedRate;			//< Minimum acceptable guaranteed bit rates on uplink and downlink
          };

      class TQoSR99_R4Negotiated : public TPacketDataConfigBase // UMTS
          {
          public:
            TQoSR99_R4Negotiated() { iExtensionId = TPacketDataConfigBase::KConfigRel99Rel4; };
            //IMPORT_C TQoSR99_R4Negotiated();
          public:
            TTrafficClass iTrafficClass;
            TDeliveryOrder iDeliveryOrderReqd;
            TErroneousSDUDelivery iDeliverErroneousSDU;
            TInt iMaxSDUSize; // in octets, range [10, 1520], steps 10, also 1502
            TBitRate iMaxRate;
            TBitErrorRatio iBER;
            TSDUErrorRatio iSDUErrorRatio;
            TTrafficHandlingPriority iTrafficHandlingPriority; // only interactive traffic class
            TInt iTransferDelay; // [10, 4100]ms, various inc; only for conversational or streaming
            TBitRate iGuaranteedRate; // only for conversational or streaming
          };

    

 /*     // SBLP
      class TSBLPR5FlowExtensionParams
        {
        public:
           
        public:
            TAuthToken iAuthToken;            
            TUint16 iMediaComponentNumber;
	        TUint16 iIPFlowNumber;
            
    
        };
        */
      class TQoSR5Requested : public TQoSR99_R4Requested	
	/**
	Contains the requested and minimum values for the 
	UMTS/IMS 3GPP Rel5 QoS profile attributes.
	
	@publishedPartner
	@released
	*/
		{
	public:
		IMPORT_C TQoSR5Requested();
	public:	
		/** Requested signalling indication. */
		TBool						iSignallingIndication;			 
		/** Requested source statistics descriptor. */
		TSourceStatisticsDescriptor	iSourceStatisticsDescriptor;			
		};


	
	class TQoSR5Negotiated : public TQoSR99_R4Negotiated	
	/**
	Contains the negotiated values for the UMTS/IMS 3GPP Rel5 QoS profile.

	@publishedPartner
	@released
	*/
		{
	public:
		//IMPORT_C TQoSR5Negotiated();
		TQoSR5Negotiated()
		    {
		    iSignallingIndication = EFalse;
		    iSourceStatisticsDescriptor = ESourceStatisticsDescriptorUnknown;
		    
		    iExtensionId = TPacketDataConfigBase::KConfigRel5;
		    }
		    
	
		    
		    
		    
		    
	public:	
		/** Negotiated signalling indication. */	
		TBool						iSignallingIndication;			
		/** Negotiated source statistics descriptor. */
		TSourceStatisticsDescriptor	iSourceStatisticsDescriptor;	 		
		};

	
      IMPORT_C RPacketQoS();
      IMPORT_C ~RPacketQoS();
      // open new QoS functionality subsession extension on existing RPacketContext
      IMPORT_C TInt OpenNewQoS(RPacketContext& aPacketContext, TDes& aProfileName);
      // open existing QoS functionality subsession RPacketQoS
      IMPORT_C TInt OpenExistingQoS(RPacketContext& aPacketContext, TDesC& aProfileName);
      IMPORT_C void Close();	// close

      IMPORT_C void SetProfileParameters(TRequestStatus& aStatus, TDes8& aProfile) const; // set requested profile parameters
      IMPORT_C void GetProfileParameters(TRequestStatus& aStatus, TDes8& aProfile) const; // get negotiated profile parameters
      IMPORT_C void GetProfileCapabilities(TRequestStatus& aStatus, TDes8& aProfileCaps) const; // get QoS capabilities

      // notify clients whenever a change in parameters occurs
      IMPORT_C void NotifyProfileChanged(TRequestStatus& aStatus, TDes8& aProfile) const;

      IMPORT_C void CancelAsyncRequest(TInt aReqToCancel) const;

    private:
      CPacketQoSInternalData* iData;
    };


#endif
