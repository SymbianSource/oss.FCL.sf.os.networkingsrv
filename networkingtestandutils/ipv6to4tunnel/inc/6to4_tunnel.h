/*
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Name        : 6to4_tunnel.h
* Part of     : 6to4 plugin / 6to4.prt
* Implements 6to4 automatic and configured tunnels, see
* RFC 3056 & RFC 2893
* Version     : 0.2
*
*/




/**
 @internalComponent
*/

#ifndef __6TO4_TUNNEL_H
#define __6TO4_TUNNEL_H

//  INCLUDES
#include <e32base.h>
#include <in_sock.h>

// CONSTANTS
// MACROS
// DATA TYPES
class CTunnel; // This forward declaration is needed already here.
typedef TDblQue < CTunnel > TTunnelQueue;
typedef TDblQueIter < CTunnel > TTunnelQueueIter;

// FUNCTION PROTOTYPES
// FORWARD DECLARATIONS

// CLASS DECLARATION

/**
*  Parser class for configured tunnels initialization file
*  Parses and generates the tunnels requested in the initialization file.
*
*  @lib
*  @since
*/
class TTunnelParser : public TLex
	{
	public:  // Constructors and destructor
	TTunnelParser ();

	 // Destructor
	~TTunnelParser ();

	public: // New functions
		
	/**
	 * Parse tunnel configuration file
	 * @since 
	 * @param 
	 * @return TInt KErrNone if success
	 */
	TInt ParseL (const TDesC & aTunnelData);

	/**
	 * Private data access
	 * @since 
	 * @param 
	 * @return TTunnelQueue& Queue
	 */
	inline TTunnelQueue& TunnelQueue() { return iTunnelQueue; }

	private:

	typedef enum
		{
		ETokenTypeString,
		ETokenTypeInt,
		ETokenTypeEqual,
		ETokenTypeComma,
		ETokenTypeBraceLeft,
		ETokenTypeBraceRight,
		ETokenTypeParLeft,
		ETokenTypeParRight,
				ETokenTypeComment,
		ETokenTypeError,
		ETokenTypeEof
		}
	TTokenType;

	/**
	 * Get next token from the stream
	 * @since 
	 * @param 
	 * @return TTokenType Type of token encountered.
	 */
	TTokenType NextToken ();

	private:    // Data
	
	// Token value (string)
	TPtrC iToken;

	// Token value (Integer)
	TInt iTokenVal;

	// List for tunnels parsed from the configuration file
	TTunnelQueue iTunnelQueue;         
	};
	

// CLASS DECLARATION

/**
*  TIpAddress.
*  Raw IPv6 address with zone id.
*/
class TIpAddress: public TIp6Addr
	{
	public:	// Constructors

	TIpAddress() {}

	TIpAddress(const TIp6Addr &aAddr, const TUint32 aScope) : iScope(aScope) {(TIp6Addr &)*this = aAddr; }


	public: // New functions

	/**
	* Returns the address without the scope.
	*/
	inline const TIp6Addr &Address() const { return *this; }

	/**
	* Returns TRUE, if addresses and scope match.
	* The ZERO scope id acts as wild card.
	*/
	inline TInt operator==(const TIpAddress &aAddr) const
		{ return IsEqual(aAddr) && (iScope == aAddr.iScope || iScope == 0 || aAddr.iScope == 0); }

	/**
	* Returns TRUE, if address and scope does not match
	*/
	inline TInt operator!=(const TIpAddress &aAddr) const  { return ! (*this == aAddr); }
	
	/**
	* Set address and scope.
	* @param aAddr The bare address
	* @param aScope The zone id.
	*/
	inline void SetAddress(const TIp6Addr &aAddr, const TUint32 aScope)
		{
		(TIp6Addr &)*this = aAddr;
		iScope = aScope;
		}

	public:    // Data

	TUint32 iScope;
	};


// CLASS DECLARATION

/**
*  Configured tunnel
*  Keeps inside all information needed for a configured tunnel.
*
*  @lib
*  @since
*/
class CTunnel : public CBase
	{
	public:  // Constructors and destructor
	
	CTunnel (); 
	
	/**
	 * Destructor.
	 */
	virtual ~CTunnel();

	public: // New functions
		
	/**
	 * Sets a name for a tunnel. Used also as a virtual interface name.
	 * @since
	 * @param aName
	 * @return void
	 */
	void SetNameL (const TDesC & aName);

	/**
	 * Private member access
	 * @since
	 * @param
	 * @return Name
	 */
	inline const TDesC &Name() const     { return *iName; } 

	/**
	 * Private member access
	 * @since
	 * @param
	 * @return Endpoint address 
	 */
	inline TIpAddress &EndpointAddr()    { return iEndpointAddr; }

	/**
	 * Private member access
	 * @since
	 * @param
	 * @return TUint32 Interface index
	 */
	inline TUint32 InterfaceIndex()      { return iInterfaceIndex; }

	/**
	 * Private member access
	 * @since
	 * @param
	 * @return Virtual interface address
	 */
	inline TIpAddress& VirtualIfAddr()    { return iVirtualIfAddr; }   

	/**
	 * Private member access
	 * @since
	 * @param
	 * @return Route address (what is routed to this tunnel)
	 */
	inline TIp6Addr& RouteAddr()        { return iRouteAddr; }

	/**
	 * Private member access
	 * @since
	 * @param
	 * @return TUint Route address prefix length
	 */
	inline TUint RoutePrefixLength()     { return iRoutePrefixLength; }   

	/**
	 * Private member access
	 * @since
	 * @param aInterfaceIndex Interface index
	 * @return void
	 */
	inline void SetInterfaceIndex(TUint32 aInterfaceIndex) 
		  { iInterfaceIndex = aInterfaceIndex; }

	/**
	 * Private member access
	 * @since
	 * @param aRoutePrefixLength
	 * @return void
	 */
	inline void SetRoutePrefixLength(TUint aRoutePrefixLength) 
	  { iRoutePrefixLength = aRoutePrefixLength; }   


	public:     // Data
	// Link for double linked list
	TDblQueLink iDLink;    

	private:    // Data
	
	// IP header id


	// Name of the tunnel
	HBufC *iName;

	// Endpoint IPv4 address
	TIpAddress iEndpointAddr;

	// Local IPv4 address
//	TInetAddr iLocalAddr;

	// Virtual interface index
	TUint32 iInterfaceIndex;

	// IPv6 address for the tunnel virtual interface
	TIpAddress iVirtualIfAddr;   

	// IPv6 route address
	TIp6Addr iRouteAddr;

	// IPv6 route prefix length
	TUint iRoutePrefixLength;
	};

#endif      // __6TO4_TUNNEL_H   
			
// End of File
