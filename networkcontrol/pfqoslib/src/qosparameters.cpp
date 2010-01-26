// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qosparameters.h"
#include "pfqosparser.h"
#include "qosvariables.h"

//lint -e{1927}
EXPORT_C TQoSParameters::TQoSParameters()
/** Default constructor. 
* 
* Sets all internal members to zero, priorities are 
* set to the lowest priority and the adapt flag is cleared. */
	{
	// Uplink
	iUplinkBandwidth = 0;
	iUpLinkMaximumBurstSize = 0;
	iUpLinkMaximumPacketSize = 0;
	iUpLinkAveragePacketSize = 0;
	iUpLinkDelay = 0;
	iUpLinkPriority = KQoSLowestPriority;
	
	// Downlink
	iDownlinkBandwidth = 0;
	iDownLinkMaximumBurstSize = 0;
	iDownLinkMaximumPacketSize = 0;
	iDownLinkAveragePacketSize = 0;
	iDownLinkDelay = 0;
	iDownLinkPriority = KQoSLowestPriority;

	// Others
	iName.FillZ();
	iFlags = 2; // Headers included in bitrate calcs
	}

EXPORT_C TInt TQoSParameters::operator==(const TQoSParameters& aParameters) 
	const
/** Equality operator. 
*
* @param aParameters The object to be compared with *this.
* @return			1, if the supplied argument is equal to *this; 
*					0, otherwise.*/
	{
	if (iUplinkBandwidth == aParameters.GetUplinkBandwidth() &&
		iUpLinkMaximumBurstSize == aParameters.GetUpLinkMaximumBurstSize() &&
		iUpLinkMaximumPacketSize == aParameters.GetUpLinkMaximumPacketSize() 
		&&
		iUpLinkAveragePacketSize == aParameters.GetUpLinkAveragePacketSize() 
		&&
		iUpLinkDelay == aParameters.GetUpLinkDelay() &&
		iUpLinkPriority == aParameters.GetUpLinkPriority() &&

		iDownlinkBandwidth == aParameters.GetDownlinkBandwidth() &&
		iDownLinkMaximumBurstSize == aParameters.GetDownLinkMaximumBurstSize() 
		&&
		iDownLinkMaximumPacketSize == 
			aParameters.GetDownLinkMaximumPacketSize() &&
		iDownLinkAveragePacketSize == 
			aParameters.GetDownLinkAveragePacketSize() &&
		iDownLinkDelay == aParameters.GetDownLinkDelay() &&
		iDownLinkPriority == aParameters.GetDownLinkPriority() &&
			
		iName.Compare(aParameters.GetName()) == 0 &&
		iFlags == aParameters.Flags())
		{
		return 1;
		}
	else
		{
		return 0;
		}
	}


EXPORT_C TQoSParameters& TQoSParameters::operator=
	(const TQoSParameters& aSpec)
/** Assignment operator.
* 
* @param aSpec	The object to make *this equal to.
* @return		A reference to *this. */
	{
	if (this != &aSpec)
		{
		SetUplinkBandwidth(aSpec.GetUplinkBandwidth());
		SetUpLinkMaximumBurstSize(aSpec.GetUpLinkMaximumBurstSize());
		SetUpLinkMaximumPacketSize(aSpec.GetUpLinkMaximumPacketSize());
		SetUpLinkAveragePacketSize(aSpec.GetUpLinkAveragePacketSize());
		SetUpLinkDelay(aSpec.GetUpLinkDelay());
		SetUpLinkPriority(aSpec.GetUpLinkPriority());

		SetDownlinkBandwidth(aSpec.GetDownlinkBandwidth());
		SetDownLinkMaximumBurstSize(aSpec.GetDownLinkMaximumBurstSize());
		SetDownLinkMaximumPacketSize(aSpec.GetDownLinkMaximumPacketSize());
		SetDownLinkAveragePacketSize(aSpec.GetDownLinkAveragePacketSize());
		SetDownLinkDelay(aSpec.GetDownLinkDelay());
		SetDownLinkPriority(aSpec.GetDownLinkPriority());

		SetFlags(aSpec.Flags());
		SetName(aSpec.GetName());
		SetAdaptMode(aSpec.AdaptMode());
		}
	return (*this);
	}

EXPORT_C void TQoSParameters::SetUplinkBandwidth(TInt aBandwidth)
/** Sets the bandwidth for the uplink.
* 
* @param aBandwidth	The bandwidth for the uplink. */
	{
	iUplinkBandwidth = aBandwidth;
	}

EXPORT_C void TQoSParameters::SetUpLinkMaximumBurstSize(TInt aSize)
/** Sets the maximum burst size for the uplink.
* 
* @param aSize	The maximum burst size for the uplink. */
	{
	iUpLinkMaximumBurstSize = aSize;
	}

EXPORT_C void TQoSParameters::SetUpLinkMaximumPacketSize(TInt aMaxSize)
/** Sets the maximum packet size for the uplink.
* 
* @param aMaxSize	The maximum packet size for the uplink. */
	{
	iUpLinkMaximumPacketSize = aMaxSize;
	}

EXPORT_C void TQoSParameters::SetUpLinkAveragePacketSize(TInt aSize)
/** Sets the average packet size for uplink direction. 
* 
* Average packet size is used as the smallest packet size when doing traffic 
* policing and estimating effect of protocol headers. The average packet size 
* must be <= maximum packet size.
* 
* @param aSize	The average packet size for uplink direction in bytes. */
	{
	iUpLinkAveragePacketSize = aSize;
	}

EXPORT_C void TQoSParameters::SetUpLinkDelay(TInt aDelay)
/** Sets the delay for the uplink.
* 
* @param aDelay	The uplink delay (in milliseconds). */
	{
	iUpLinkDelay = aDelay;
	}

EXPORT_C void TQoSParameters::SetUpLinkPriority(TInt aPriority)
/** Sets the priority for uplink direction. 
* 
* Priority can be used to prioritise between traffic flows inside the 
* terminal.
* 
* @param aPriority The priority for uplink direction, 0 indicates the highest 
*				  priority. */
	{
	if (aPriority > KQoSLowestPriority)
		{
		iUpLinkPriority = KQoSLowestPriority;
		}
	else
		{
		iUpLinkPriority = aPriority;
		}
	}

EXPORT_C void TQoSParameters::SetDownlinkBandwidth(TInt aBandwidth)
/** Sets the bandwidth for downlink direction. 
* 
* Bandwidth defines the requested transfer rate that the application 
* requests. bandwidth must be > 0.
* 
* @param aBandwidth The value to which to set the bandwidth for downlink 
*				   direction in bytes/second. */
	{
	iDownlinkBandwidth = aBandwidth;
	}

EXPORT_C void TQoSParameters::SetDownLinkMaximumBurstSize(TInt aSize)
/** Sets the maximum burst size for downlink direction. 
* 
* Maximum burst size defines the burst size that the application might send. 
* Maximum burst size must be > 0.
* 
* @param aSize The value to which to set the maximum burst size for downlink 
*			  direction in bytes. */
	{
	iDownLinkMaximumBurstSize = aSize;
	}

EXPORT_C void TQoSParameters::SetDownLinkMaximumPacketSize(TInt aMaxSize)
/** Sets the maximum packet sixe for the downlink.
* 
* @param aMaxSize	The maximum packet size for the downlink. */
	{
	iDownLinkMaximumPacketSize = aMaxSize;
	}

EXPORT_C void TQoSParameters::SetDownLinkAveragePacketSize(TInt aSize)
/** Sets the average packet size for downlink direction.
* 
* @param aSize	The value to which to set the maximum packet size for 
*				 downlink direction in bytes. */
	{
	iDownLinkAveragePacketSize = aSize;
	}

EXPORT_C void TQoSParameters::SetDownLinkDelay(TInt aDelay)
/** Sets the downlink delay
* 
* @param aDelay	The downlink delay (in milliseconds). */
	{
	iDownLinkDelay = aDelay;
	}

EXPORT_C void TQoSParameters::SetDownLinkPriority(TInt aPriority)
/** Sets the priority for downlink direction. 
* 
* Priority can be used to prioritise between traffic flows inside the 
* terminal.
* 
* @param aPriority The priority for downlink direction, 0 indicates the 
*				  highest priority. */
	{
	if (aPriority > KQoSLowestPriority)
		{
		iDownLinkPriority = KQoSLowestPriority;
		}
	else
		{
		iDownLinkPriority = aPriority;
		}
	}

EXPORT_C TBool TQoSParameters::AdaptMode() const
/** Tests whether the application can adapt.
* 
* @return ETrue, if the application can adapt; otherwise, EFalse. */
// @return The adaption mode. */
	{
	if (iFlags & KPfqosOptionCanAdapt)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

EXPORT_C TBool TQoSParameters::GetHeaderMode() const
	{
	if (iFlags & KPfqosOptionHeadersIncluded)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}
	}

EXPORT_C TInt TQoSParameters::SetName(const TName& aName)
/** Sets the name to a copy of the supplied argument.
* 
* @param aName The name.
* @return	  KErrNone, if successful; KErrTooBig, if the supplied argument 
*			  is too large (256 characters maximum). */
	{
	if (aName.Length() > iName.MaxLength())
		{
		return KErrTooBig;
		}
	iName.Copy(aName);
	return KErrNone;
	}

EXPORT_C void TQoSParameters::SetAdaptMode(TBool aCanAdapt)
/** Sets the adaption mode.
* 
* @param aCanAdapt	The adaption mode. */
	{
	if (aCanAdapt)
		{
		iFlags |= KPfqosOptionCanAdapt;
		}
	else
		{
		iFlags &= ~KPfqosOptionCanAdapt;
		}
	}

EXPORT_C void TQoSParameters::SetHeaderMode(TBool aHeadersIncluded)
	{
	if (aHeadersIncluded)
		{
		iFlags |= KPfqosOptionHeadersIncluded;
		}
	else
		{
		iFlags &= ~KPfqosOptionHeadersIncluded;
		}
	}

EXPORT_C void TQoSParameters::Copy(CExtension& aExtension)
/** Makes the members of this object equal to the equivalent settings of the 
* extension policy (i.e. a module specific policy) of the supplied parameter. 
* 
* If any of the extension policy settings do not exist then the appropriate 
* member of this object is unchanged.
* 
* @param aExtension	The extension policy. */
	{
	TVariableBase* var;

	// bandwidth up
	var = aExtension.FindVariable(KDescUplinkBandwidth);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetUplinkBandwidth(((TIntVariable*)var)->Value());
		}

	// maximum burst size up
	var = aExtension.FindVariable(KDescUpLinkMaximumBurstSize);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetUpLinkMaximumBurstSize(((TIntVariable*)var)->Value());
		}

	// Priority up
	var = aExtension.FindVariable(KDescUpLinkPriority);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetUpLinkPriority((TUint8)((TIntVariable*)var)->Value());
		}

	// Max packet size up
	var = aExtension.FindVariable(KDescUpLinkMaximumPacketSize);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetUpLinkMaximumPacketSize(((TIntVariable*)var)->Value());
		}

	// average packet size up
	var = aExtension.FindVariable(KDescUpLinkAveragePacketSize);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetUpLinkAveragePacketSize(((TIntVariable*)var)->Value());
		}

	// Delay up
	var = aExtension.FindVariable(KDescUpLinkDelay);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetUpLinkDelay(((TIntVariable*)var)->Value());
		}

	// bandwidth down
	var = aExtension.FindVariable(KDescDownlinkBandwidth);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetDownlinkBandwidth(((TIntVariable*)var)->Value());
		}

	// maximum burst size down
	var = aExtension.FindVariable(KDescDownLinkMaximumBurstSize);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetDownLinkMaximumBurstSize(((TIntVariable*)var)->Value());
		}

	// Priority down
	var = aExtension.FindVariable(KDescDownLinkPriority);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetDownLinkPriority((TUint8)((TIntVariable*)var)->Value());
		}

	// Max packet size down
	var = aExtension.FindVariable(KDescDownLinkMaximumPacketSize);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetDownLinkMaximumPacketSize(((TIntVariable*)var)->Value());
		}

	// average packet size down
	var = aExtension.FindVariable(KDescDownLinkAveragePacketSize);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetDownLinkAveragePacketSize(((TIntVariable*)var)->Value());
		}

	// Delay down
	var = aExtension.FindVariable(KDescDownLinkDelay);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetDownLinkDelay(((TIntVariable*)var)->Value());
		}

	// Flags
	var = aExtension.FindVariable(KDescFlags);
	if (var && var->Type() == KPfqosTypeInteger)
		{
		SetFlags((TUint)((TIntVariable*)var)->Value());
		}

	// Name
	var = aExtension.FindVariable(KDescName);
	if (var && var->Type() == KPfqosTypeString)
		{
		SetName(((TStringVariable*)var)->Value());
		}
	}

