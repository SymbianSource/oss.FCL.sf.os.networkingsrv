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
// AlgorithmConf - Algorithm data 
//



/**
 @internalComponent
*/
#ifndef __ALGORITHMCONF__
#define __ALGORITHMCONF__

_LIT8(KAlgorithmConf, 
                   "encrypt    descbc(2,64,64,64,64)\r\n"
                   "encrypt    3descbc(3,64,192,192,192)\r\n"
                   "encrypt    rc5(4,64,128,40,2040)\r\n"
                   "encrypt    null(11)\r\n"
                   "encrypt    aescbc(12,128,128,128,256)\r\n"
                   "encrypt	   aesctr(13,64)\r\n"
                   "auth md5(2,96)\r\n"
                   "auth sha1(3,96)\r\n");
                   
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
_LIT8(KNewAlgorithmConf, 
                   "auth    	aesxcbcmac(9,96)\r\n");
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT


#endif
