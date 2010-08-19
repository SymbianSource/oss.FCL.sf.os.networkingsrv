// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 
#ifndef __T_AUTOSSL_CONST_H__
#define __T_AUTOSSL_CONST_H__

// Constants
const TInt KMaxSSLConnections = 1; //***********
const TInt KControllerDelayTime = 5000000;
const TInt KCipherBufSize = 128;

// parameter file related constants
_LIT( KSectionName, "Tlstest" );

_LIT( KCfgIPAddress, "IPAddress" );
_LIT( KCfgIPPort, "IPPort" );
_LIT( KCfgCipher, "Cipher" );
_LIT( KCfgCipherSuites, "CipherSuites" );
_LIT( KCfgPage, "Page" );
_LIT( KCfgDNSName, "DNSName" );
_LIT( KCfgSimpleGet, "SimpleGet" );
_LIT( KCfgTestEndDelay, "TestEndDelay" );
_LIT( KCfgFailureThreshold, "FailureThreshold" );
_LIT( KCfgMaxThreshold, "MaxThreshold" );
_LIT( KCfgProtocol, "Protocol" );
_LIT( KCfgUseGenericSocket, "UseGenericSocket");
_LIT( KCfgEAPKeyDerivation, "EAPKeyDerivation");
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
_LIT( KCfgSSLDialogMode, "SSLDialogMode");
_LIT( KCfgSSLDialogModeValue, "SSLDialogModeValue");
_LIT( KCfgExpectedErrorCode, "ExpectedErrorCode");
#endif // HTTP_ALLOW_UNTRUSTED_CERTIFICATES

const TInt KDefCfgCipher = 0;
const TInt KDefCfgIPPort = 0;
const TInt KDefCfgSimpleGet = 0;
const TInt KDefCfgTestEndDelay = 32;
const TInt KDefCfgFailureThreshold = 1;
const TInt KDefCfgMaxThreshold = 1000;
const TBool KDefUseGenericSocket = EFalse;
const TBool KDefEAPKeyDerivation = EFalse;
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
const TBool KDefSSLDialogMode = EFalse;
const TInt KDefSSLDialogModeValue = 0;
const TInt KDefErrorValue = 0;
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES

_LIT( KDefCfgIPAddress, "default" );
_LIT( KDefCfgDNSName, "www.default.com" );
_LIT( KDefCfgPage, "default.htm" );
_LIT( KDefCfgCipherSuites, "0" );
_LIT( KDefCfgProtocol, "TLS1.0" );


#endif // __T_AUTOSSL_CONST_H__
