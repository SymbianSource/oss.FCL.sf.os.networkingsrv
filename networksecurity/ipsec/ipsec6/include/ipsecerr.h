// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ipsecerr.h - reason codes for IPSEC problems
// Define and document the IPSEC error codes.
//



/**
 @file ipsecerr.h
 @publishedPartner
 @released
*/
#ifndef __IPSECERR_H__
#define __IPSECERR_H__


enum TIpsecReasonCode
	{
	EIpsec_Ok,
	EIpsec_RMBUF = -5228,			//< -5228 RMBUF operation failed unexcpectedly
	//
	// AH and ESP
	//
	EIpsec_CorruptPacketIn,			//< -5227 Truncated or corrupt packet or header (in)
	EIpsec_CorruptPacketOut,		//< -5226 Corrupt packet after IPSEC operations
	EIpsec_EspInboundSA,			//< -5225 The inbound SA for ESP does not exist
	EIpsec_EspAuthentication,		//< -5224 Authentication check failed in ESP
	EIpsec_EspAuthAlg,				//< -5223 Required auth algorithm for ESP not available/installed
	EIpsec_EspEncrAlg,				//< -5222 Required encrypt algorithm for ESP not available/installed
	EIpsec_AhAuthAlg,				//< -5221 Required auth algorithm for AH not available/installed
	EIpsec_AhInboundSA,				//< -5220 The inbound SA for AH does not exist
	EIpsec_AhIcvLength,				//< -5219 ICV length in packet does not match algorithm
	EIpsec_AhAuthentication,		//< -5218 Authentication check failed in AH
	EIpsec_PacketLength,			//< -5217 Invalid lenght of the packet
	EIpsec_DataAlignment,			//< -5216 Data not aligned by block size
	EIpsec_EspPadByte,				//< -5215 The ESP pad byte content is invalid (probably wrong key)
	EIpsec_EspPadLength,			//< -5214 The ESP pad length is corrupt (probably wrong key)
	EIpsec_ReplayDuplicate,			//< -5213 Duplicate packet (replay window test)

	//
	// SECPOL
	//
	EIpsec_OutboundNotFound,		//< -5212 Outbound SA does not exist, ACQUIRE started
	EIpsec_OutboundPending,			//< -5211 Outbooud SA does not exits, ACQUIRE pending
	EIpsec_NoSelectorMatch,			//< -5210 None of the policy selectors matched
	EIpsec_MaxTransforms,			//< -5209 Incoming packet exceed configured max limit of transforms
	EIpsec_TooFewTransforms,		//< -5208 Incoming packet has less transforms than policy requires
	EIpsec_TunnelMismatch,			//< -5207 Tunnelmode does not match the policy
	EIpsec_MismatchedSA,			//< -5206 Applied SA does not match the policy
	EIpsec_UnrequiredSA,			//< -5205 Applied SA where policy has none
	EIpsec_TooManyTransforms,		//< -5204 Incoming packet had more transforms than policy requires
	EIpsec_NoBundle,				//< -5203 Incoming packet had transforms, but policy doesn't require any

	//
	// IPv6 additions
	//
	EIpsec_AhRMBufSplit,			//< -5202 Inbound AH processing failed (Memory?)
	EIpsec_AhPacketTooLong,			//< -5201 Outbound packet would exeed 2**16-1 with AH
	EIpsec_AhSequenceWrap,			//< -5200 Outbound sequence # wrapped around for this SA
	EIpsec_EspSequenceWrap,			//< -5199 Outbound sequence # wrapped around for this SA
	EIpsec_EspBadCipherBlockSize,	//< -5198 Configuration error, cipher block size must be < 256

	EIpsec_AcquireFailed,			//< -5197 Acquiring SA failed (no SA available or negotiated)
	//
	// Detail reasons for SA not matching the SA spec in the policy
	// (replace one EIpsec_MismatchedSA with multiple detail errors)
	//
	EIpsec_MismatchedDestination,	//< -5196 SA destination does not match (internal error?)
	EIpsec_MismatchedType,			//< -5195 SA Type (AH/ESP) does not match
	EIpsec_MismatchedPFS,			//< -5194 PFS bit is not same
	EIpsec_MismatchedAuthAlg,		//< -5193 Auth algorithm doesn't match
	EIpsec_MismatchedEncryptAlg,	//< -5192 Encrypt algorithm doesn't match
	EIpsec_MismatchReplayWindow,	//< -5191 ReplayWindow length is shorter than required
	EIpsec_MismatchSource,			//< -5190 source address does not match
	EIpsec_MismatchProxy,			//< -5189 proxy address does not match
	EIpsec_MismatchSourcePort,		//< -5188 source port does not match  
	EIpsec_MismatchDestinationPort,	//< -5187 destination port does not match
	EIpsec_MismatchProtocol,		//< -5186 protocol does not match
	EIpsec_MismatchSourceIdentity,	//< -5185 source identity does not match
	EIpsec_MismatchDestinationIdentity,//< -5184 destination identity does not match
	//
	// PFKEY and SAD specific errors
	//
	EIpsec_BadCipherKey,			//< -5183 Key in SA is too short (for the algorithm) or is weak
	EIpsec_UnknownCipherNumber,		//< -5182 Attempting to use algorithm number that is not known
	EIpsec_UnknownDigestNumber,		//< -5181 Attempting to use algorithm number that is not known
	EIpsec_UnavailableCipher,		//< -5180 No installed library implements the cipher
	EIpsec_UnavailableDigest,		//< -5179 No installed library implements the digest
	//
	// Policy Parsing Error codes
	//
	EIpsec_PolicyUnknownEncrypt,	//< -5178 algorithm not defined in algorithm map
	EIpsec_PolicyUnknownAuth,		//< -5177 algorithm not defined in algorithm map
	EIpsec_PolicyIdentityDefined,	//< -5176 identify already defined
	EIpsec_PolicyInvalidIdentity,	//< -5175 invalid identity syntax
	EIpsec_PolicyUnknownSpec,		//< -5174 unknown policy specification keyword
	EIpsec_PolicyNumberExpected,	//< -5173 number value expected
	EIpsec_PolicyCloseBraceExpected,//< -5172 closing brace expected
	EIpsec_PolicyNoType,			//< -5171 SA type (AH or ESP) omitted from specification
	EIpsec_PolicyTooManyTypes,		//< -5170 Type can be specified only once for specification
	EIpsec_PolicyNoAuthAlgorithm,	//< -5169 AH specification must include authentication algorithm
	EIpsec_PolicyNoEncryptAlgorithm,//< -5168 ESP specification must include encryptionb algorithm
	EIpsec_PolicySpecName,			//< -5167 SA specification name missing or invalid
	EIpsec_PolicySyntaxError,		//< -5166 Generic delimiter error in specification
	EIpsec_PolicySpecNotFound,		//< -5165 SA specification is not defined before reference in selector
	EIpsec_PolicyLeftParen,			//< -5164 Left parenthesis expected
	EIpsec_PolicyRightParen,		//< -5163 Right parenthesis expected
	EIpsec_PolicyInvalidIpAddress,	//< -5162 Invalid IP address
	EIpsec_PolicyIpAddressExpected,	//< -5161 Expected IP address here
	EIpsec_PolicyIpMaskExpected,	//< -5160 Expected IP address (as mask) here
	EIpsec_PolicyInboundOutbound,	//< -5159 Only one of the 'inbound' or 'outbound' is allowed
	EIpsec_PolicyUnknownSelector,	//< -5158 unknown selector keyword
	//
	// Temporary place for new errors
	//
	EIpsec_IcmpError,				//< -5157 An ICMP error report containing AH or ESP
	EIpsec_LostSA,					//< -5156 An SA has been lost between Apply and Verify, expired? (for SECPOL)
	EIpsec_NoInnerSource,			//< -5155 Cannot find inner-src for outbound packet when tunneling (for SECPOL)
    //
    // Special code for NAT Traversal  
    //
    EIpsec_NotANATTPacket,          //< -5154 UDP packet is NOT a NAT Taversal packet
    //
	EIpsec_FragmentMismatch			//< -5153 IPSEC on fragment is not same as before, packet dropped
	};

#endif
