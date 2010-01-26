// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is Test Interface header. Testing application uses test interface to
// communicate with NIF and giving instructions of the test cases.
//

#ifndef __TESTIF_H__
#define __TESTIF_H__

#include <UmtsNifControlIf.h>
#include <qos_if.h>

// Constants for message types 
const TUint KTest                           = 2501;
const TUint KNetworkFailure                 = 2502;
const TUint KSetQoSFail                     = 2503;
const TUint KReNegotiateFail                = 2504;
const TUint KDowngrade                      = 2505;
const TUint KNetworkContextDelete           = 2506;
const TUint KNotifySecondaryCreated         = 2507;
const TUint KLoadTestModule                 = 2508;
const TUint KDropContext                    = 2509;
                                            
const TUint KSetDefaultQoSFail              = 2510;
const TUint KContextCreateFail              = 2511;
const TUint KContextActivateFail            = 2513;
const TUint KContextModifyActiveFail        = 2514;
const TUint KContextQoSSetFail              = 2515;
const TUint KContextTFTModifyFail           = 2516;

const TUint KContextCreateFailAsync         = 2521;
const TUint KContextActivateFailAsync       = 2523;
const TUint KContextModifyActiveFailAsync   = 2524;
const TUint KContextQoSSetFailAsync         = 2525;
const TUint KContextTFTModifyFailAsync      = 2526;
const TUint KPrintRel99                     = 2527;

// To reset the message values back 
const TUint KResetMessage                   = 2528;
const TUint KNetworkDowngrade               = 2529;
const TUint KNetworkUpgrade                 = 2530;
const TUint KNetworkDowngradeAfter          = 2531;

// To create the SBLP rejection error code
const TUint KContextActivateRejectSBLP      = 2532;

// To create the IMS unsupported network scenerio
const TUint KNetworkUnsupportedIms          = 2533;

// To downgrade the umts r5 parameters
const TUint KNetworkDowngradeR5             = 2534;

// To create the IMS unsupported network scenerio
const TUint KNetworkUnsupportedUmtsR5       = 2535;

// General error code for test errors
const TInt KErrTest                         = -6001;

_LIT(KTestDir,   "c:\\QoSTest\\");
_LIT(KTestFile1, "c:\\QoSTest\\testfile_1");
_LIT(KTestFile2, "c:\\QoSTest\\testfile_2");

// This file informs nif to use testmodule instead of guqos
_LIT(KUseTestModule, "c:\\QoSTest\\testmodule"); 

// This file informs nif to load non-exist testmodule
_LIT(KUseTestModuleNonExist, "c:\\QoSTest\\testmodulenonexist"); 

// This file informs nif not to load any module 
_LIT(KUseNoModule, "c:\\QoSTest\\nomodule"); 

// This file informs that testmodule is loaded
_LIT(KTestModuleLoaded, "c:\\QoSTest\\testmoduleloaded"); 

// This file informs that testmodule is unloaded
_LIT(KTestModuleUnloaded, "c:\\QoSTest\\testmoduleunloaded"); 

// This file informs nif to fail SetDefaultQoS
_LIT(KTestSetDefaultQoSFail, "c:\\QoSTest\\setdefaultqosfail"); 

// This file informs nif to fail SetDefaultQoS
_LIT(KRel99, "c:\\QoSTest\\Rel99"); 

// This file informs that the sblp parameter has been added in the
// context during tft operation
_LIT(KTestSblpAdd, "c:\\QoSTest\\sblp_add_file");

// This file informs that the ims parameter has been added in the
// context during tft operation
_LIT(KTestIMS, "c:\\QoSTest\\ims_file");

// This file informs that the umtsr5 parameter has been added in the
// context during tft operation
_LIT(KTestUmtsR5, "c:\\QoSTest\\umtsr5_file");

class TTestNifControlIf
    {
    public:
        // Has to be defined right at the beginning of this class; the stack 
        // needs this identifier to lookup the testNif and forward the 
        // setopt() function call in the test application.
        TSoIfInfo iInterfaceIdentifier;        
        
        // KContextActivate, KContextQoSSet; KRegisterEventHandler ...
        TUint iControlOption; 
        
        // contains the QoS plug-in (e.g. guqos) and a protocol ID
        TSoIfControllerInfo iControllerInfo; 
    };

#endif
